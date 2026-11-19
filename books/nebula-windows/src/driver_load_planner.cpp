// MIT License - (c) Anthony Matarazzo 2026
//
// driver_load_planner.cpp - conflict detection, topological ordering and waves.

#include "driver_load_planner.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <set>
#include <sstream>

namespace nebula::drivers {
namespace {

const LoadRequest* find_req(const std::vector<LoadRequest>& reqs, const std::string& id) {
    for (const auto& r : reqs) {
        if (r.driver_id == id) return &r;
    }
    return nullptr;
}

} // namespace

const char* to_string(Conflict::Kind k) {
    switch (k) {
        case Conflict::Kind::DuplicateId:      return "duplicate-id";
        case Conflict::Kind::MissingDependency: return "missing-dependency";
        case Conflict::Kind::Cycle:            return "cycle";
        case Conflict::Kind::Resource:         return "resource";
        case Conflict::Kind::Explicit:         return "explicit";
        case Conflict::Kind::SelfDependency:   return "self-dependency";
    }
    return "unknown";
}

DriverLoadPlanner::DriverLoadPlanner(std::vector<LoadRequest> requests)
    : requests_(std::move(requests)) {}

double DriverLoadPlanner::estimate_ms(const LoadRequest& r, const PlannerOptions& opts) {
    double transfer = opts.throughput_mb_per_ms > 0.0 ? r.size_mb / opts.throughput_mb_per_ms
                                                     : r.size_mb;
    return opts.fixed_overhead_ms + transfer;
}

std::vector<Conflict> DriverLoadPlanner::detect_conflicts() const {
    std::vector<Conflict> out;
    std::set<std::string> seen;

    for (const auto& r : requests_) {
        if (!seen.insert(r.driver_id).second) {
            out.push_back({Conflict::Kind::DuplicateId, r.driver_id, r.driver_id,
                           "driver requested more than once"});
        }
        for (const auto& dep : r.requires) {
            if (dep == r.driver_id) {
                out.push_back({Conflict::Kind::SelfDependency, r.driver_id, dep,
                               "driver requires itself"});
            } else if (!find_req(requests_, dep)) {
                out.push_back({Conflict::Kind::MissingDependency, r.driver_id, dep,
                               "required driver is not in the request list"});
            }
        }
        for (const auto& other : r.conflicts_with) {
            if (other == r.driver_id) continue;
            // Report each explicit pair once, using sorted order.
            const LoadRequest* o = find_req(requests_, other);
            if (!o) continue;
            std::string key = std::min(r.driver_id, other) + "|" + std::max(r.driver_id, other);
            if (seen.count("pair:" + key)) continue;
            seen.insert("pair:" + key);
            out.push_back({Conflict::Kind::Explicit, r.driver_id, other,
                           "declared mutually exclusive"});
        }
    }

    // Shared exclusive resources.
    std::map<std::string, std::vector<std::string>> owners;
    for (const auto& r : requests_) {
        for (const auto& res : r.claims) {
            owners[res].push_back(r.driver_id);
        }
    }
    for (const auto& kv : owners) {
        if (kv.second.size() < 2) continue;
        for (std::size_t i = 0; i < kv.second.size(); ++i) {
            for (std::size_t j = i + 1; j < kv.second.size(); ++j) {
                if (kv.second[i] == kv.second[j]) continue;
                out.push_back({Conflict::Kind::Resource, kv.second[i], kv.second[j],
                               "both claim resource " + kv.first});
            }
        }
    }

    // Cycles, reported once per participating driver set.
    std::map<std::string, int> state; // 0 unvisited, 1 on stack, 2 done
    std::set<std::string> reported;
    std::vector<std::string> stack;
    std::function<void(const std::string&)> visit = [&](const std::string& id) {
        int& s = state[id];
        if (s == 1) {
            auto it = std::find(stack.begin(), stack.end(), id);
            if (it != stack.end()) {
                std::string cycle;
                for (auto p = it; p != stack.end(); ++p) {
                    if (!cycle.empty()) cycle += " -> ";
                    cycle += *p;
                }
                cycle += " -> " + id;
                if (reported.insert(cycle).second) {
                    out.push_back({Conflict::Kind::Cycle, id, *it, cycle});
                }
            }
            return;
        }
        if (s == 2) return;
        s = 1;
        stack.push_back(id);
        if (const LoadRequest* r = find_req(requests_, id)) {
            for (const auto& dep : r->requires) {
                if (dep != id && find_req(requests_, dep)) visit(dep);
            }
        }
        stack.pop_back();
        s = 2;
    };
    for (const auto& r : requests_) visit(r.driver_id);
    return out;
}

LoadPlan DriverLoadPlanner::plan(const PlannerOptions& opts) const {
    LoadPlan plan;

    std::vector<LoadRequest> active;
    std::set<std::string> active_ids;
    for (const auto& r : requests_) {
        if (active_ids.count(r.driver_id)) {
            plan.dropped.push_back(r.driver_id + " (duplicate request)");
            continue;
        }
        active_ids.insert(r.driver_id);
        active.push_back(r);
    }

    auto conflicts = detect_conflicts();

    // Resource and explicit conflicts: optionally resolve by priority.
    std::set<std::string> blocked_ids;
    std::map<std::string, std::vector<std::string>> owners;
    for (const auto& r : active) {
        for (const auto& res : r.claims) owners[res].push_back(r.driver_id);
    }
    auto drop_or_block = [&](const std::string& loser, const std::string& why) {
        if (opts.resolve_conflicts) {
            blocked_ids.insert(loser);
            plan.dropped.push_back(loser + " (" + why + ")");
        }
    };
    for (const auto& kv : owners) {
        if (kv.second.size() < 2) continue;
        // Highest priority claimant keeps the resource; ties break on id order.
        std::string winner = kv.second.front();
        for (const auto& id : kv.second) {
            const LoadRequest* a = find_req(active, id);
            const LoadRequest* b = find_req(active, winner);
            if (!a || !b) continue;
            if (a->priority > b->priority || (a->priority == b->priority && id < winner)) {
                winner = id;
            }
        }
        for (const auto& id : kv.second) {
            if (id != winner) drop_or_block(id, "lost resource " + kv.first + " to " + winner);
        }
    }
    for (const auto& c : conflicts) {
        if (c.kind == Conflict::Kind::Explicit) {
            const LoadRequest* a = find_req(active, c.a);
            const LoadRequest* b = find_req(active, c.b);
            if (!a || !b) continue;
            const LoadRequest* loser = a->priority < b->priority ? a : (b->priority < a->priority ? b : nullptr);
            if (loser) drop_or_block(loser->driver_id, "declared conflict with " +
                                                          (loser->driver_id == a->driver_id ? b->driver_id
                                                                                           : a->driver_id));
        }
    }

    // Remove blocked drivers, plus anything that depends on them.
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& r : active) {
            if (blocked_ids.count(r.driver_id)) continue;
            for (const auto& dep : r.requires) {
                if (blocked_ids.count(dep)) {
                    if (r.critical || !opts.drop_non_critical) {
                        // A critical driver cannot silently lose its dependency.
                        plan.error = "critical driver " + r.driver_id + " depends on dropped " + dep;
                    }
                    blocked_ids.insert(r.driver_id);
                    changed = true;
                    break;
                }
            }
        }
    }

    std::vector<LoadRequest> ordered_input;
    for (const auto& r : active) {
        if (blocked_ids.count(r.driver_id)) continue;
        ordered_input.push_back(r);
    }

    // Topological sort. Ties resolve on (priority desc, id asc) for determinism.
    std::set<std::string> done;
    std::vector<LoadStep> steps;
    std::map<std::string, int> wave_of;

    while (steps.size() < ordered_input.size()) {
        std::vector<const LoadRequest*> ready;
        for (const auto& r : ordered_input) {
            if (done.count(r.driver_id)) continue;
            bool ok = true;
            for (const auto& dep : r.requires) {
                if (dep == r.driver_id) { ok = false; break; }
                if (!find_req(ordered_input, dep)) { ok = false; break; }
                if (!done.count(dep)) { ok = false; break; }
            }
            if (ok) ready.push_back(&r);
        }
        if (ready.empty()) {
            // Cycle among the remaining drivers.
            std::string stuck;
            for (const auto& r : ordered_input) {
                if (done.count(r.driver_id)) continue;
                if (!stuck.empty()) stuck += ",";
                stuck += r.driver_id;
            }
            plan.error = "cannot order drivers (cycle or unresolved dependency): " + stuck;
            for (const auto& r : ordered_input) {
                if (done.count(r.driver_id)) continue;
                if (r.critical) plan.loadable = false;
                else plan.dropped.push_back(r.driver_id + " (unorderable, non critical)");
            }
            break;
        }
        std::sort(ready.begin(), ready.end(), [](const LoadRequest* a, const LoadRequest* b) {
            if (a->priority != b->priority) return a->priority > b->priority;
            return a->driver_id < b->driver_id;
        });

        for (const LoadRequest* r : ready) {
            LoadStep s;
            s.driver_id = r->driver_id;
            s.binary = r->binary;
            s.size_mb = r->size_mb;
            s.priority = r->priority;
            s.requires = r->requires;
            s.est_ms = estimate_ms(*r, opts);
            int w = 0;
            for (const auto& dep : r->requires) {
                auto it = wave_of.find(dep);
                if (it != wave_of.end()) w = std::max(w, it->second + 1);
            }
            s.wave = w;
            wave_of[r->driver_id] = w;
            done.insert(r->driver_id);
            steps.push_back(s);
        }
    }

    plan.steps = steps;
    plan.conflicts = conflicts;
    plan.loadable = plan.error.empty();

    for (const auto& s : plan.steps) {
        plan.est_total_ms += s.est_ms;
        plan.total_mb += s.size_mb;
        plan.waves = std::max(plan.waves, s.wave + 1);
    }

    // Critical path: longest chain of estimated load costs through requires.
    std::map<std::string, double> memo;
    std::function<double(const std::string&)> chain = [&](const std::string& id) -> double {
        auto hit = memo.find(id);
        if (hit != memo.end()) return hit->second;
        const LoadStep* step = nullptr;
        for (const auto& s : plan.steps) {
            if (s.driver_id == id) { step = &s; break; }
        }
        if (!step) return 0.0;
        double best = 0.0;
        for (const auto& dep : step->requires) best = std::max(best, chain(dep));
        double v = best + step->est_ms;
        memo[id] = v;
        return v;
    };
    for (const auto& s : plan.steps) {
        plan.est_critical_path_ms = std::max(plan.est_critical_path_ms, chain(s.driver_id));
    }

    // Wall time: with parallel waves, a wave costs its slowest member; serial is the sum.
    if (opts.allow_parallel_waves && plan.waves > 0) {
        std::vector<double> wave_cost(static_cast<std::size_t>(plan.waves), 0.0);
        for (const auto& s : plan.steps) {
            if (s.wave >= 0 && s.wave < plan.waves) {
                wave_cost[static_cast<std::size_t>(s.wave)] =
                    std::max(wave_cost[static_cast<std::size_t>(s.wave)], s.est_ms);
            }
        }
        double wall = 0.0;
        for (double c : wave_cost) wall += c;
        plan.est_critical_path_ms = std::max(plan.est_critical_path_ms, wall);
    }

    if (!plan.loadable) {
        plan.steps.clear();
        plan.waves = 0;
    }
    return plan;
}

std::vector<LoadRequest> DriverLoadPlanner::profile_a_requests() {
    // Sizes are the per driver figures from the book's storage table.
    return {
        {"intelpep", "intelpep.sys", 5.0, {}, {}, {}, 1, true},
        {"usbxhci", "usbxhci.sys", 3.0, {}, {"pci:bus0"}, {}, 5, true},
        {"iaStorVD", "iaStorVD.sys", 4.5, {"usbxhci"}, {"vmd:0"}, {}, 8, true},
        {"nvlddmkm", "nvlddmkm.sys", 1200.0, {"usbxhci", "intelpep"}, {"pci:01:00.0"}, {}, 9, true},
        {"netwtw10", "netwtw10.sys", 45.0, {"usbxhci"}, {"pci:00:14.3"}, {}, 6, true},
        {"e2xw10x64", "e2xw10x64.sys", 30.0, {"usbxhci"}, {"pci:00:1f.6"}, {}, 6, true},
        {"RTKVHD64", "RTKVHD64.sys", 22.0, {"intelpep"}, {"hda:0"}, {}, 7, true},
        {"razerkbd", "razerkbd.sys", 9.0, {"usbxhci"}, {"usb:1532"}, {}, 3, false},
        {"TbtBusDrv", "TbtBusDrv.sys", 11.0, {"usbxhci"}, {"pci:00:07.0"}, {}, 4, false},
    };
}

std::string format_conflicts(const std::vector<Conflict>& conflicts) {
    std::ostringstream os;
    if (conflicts.empty()) {
        os << "no conflicts\n";
        return os.str();
    }
    for (const auto& c : conflicts) {
        os << "  [" << to_string(c.kind) << "] " << c.a << " / " << c.b << " - " << c.detail << "\n";
    }
    return os.str();
}

std::string format_plan(const LoadPlan& plan) {
    std::ostringstream os;
    if (!plan.error.empty()) {
        os << "plan error: " << plan.error << "\n";
    }
    os << "wave  driver        binary           size_mb   est_ms  requires\n";
    for (const auto& s : plan.steps) {
        std::string req;
        for (std::size_t i = 0; i < s.requires.size(); ++i) {
            if (i) req += ",";
            req += s.requires[i];
        }
        if (req.empty()) req = "-";
        char line[200];
        std::snprintf(line, sizeof(line), "%4d  %-13s %-16s %7.1f  %7.3f  %s\n",
                      s.wave, s.driver_id.c_str(), s.binary.c_str(), s.size_mb, s.est_ms,
                      req.c_str());
        os << line;
    }
    char tail[256];
    std::snprintf(tail, sizeof(tail),
                  "waves=%d drivers=%zu total_mb=%.1f est_total=%.3f ms est_critical=%.3f ms "
                  "loadable=%s dropped=%zu conflicts=%zu\n",
                  plan.waves, plan.steps.size(), plan.total_mb, plan.est_total_ms,
                  plan.est_critical_path_ms, plan.loadable ? "yes" : "no",
                  plan.dropped.size(), plan.conflicts.size());
    os << tail;
    return os.str();
}

} // namespace nebula::drivers
