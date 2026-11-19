// MIT License - (c) Anthony Matarazzo 2026
#include "capability.hpp"

#include <algorithm>
#include <cstdio>

namespace lowl {
namespace kal {

const char* capability_name(Capability c) {
    switch (c) {
        case Capability::Sandbox: return "sandbox";
        case Capability::SyscallFilter: return "syscall_filter";
        case Capability::Namespace: return "namespace";
        case Capability::Container: return "container";
        case Capability::JobGroup: return "job_group";
        case Capability::AsyncIo: return "async_io";
        case Capability::HugePages: return "huge_pages";
        case Capability::ZeroCopyCopy: return "zero_copy_copy";
        case Capability::Semaphore: return "semaphore";
        case Capability::Signal: return "signal";
        case Capability::Count: break;
    }
    return "unknown";
}

const char* support_name(Support s) {
    switch (s) {
        case Support::Native: return "native";
        case Support::Emulated: return "emulated";
        case Support::Unavailable: return "unavailable";
    }
    return "unknown";
}

int support_rank(Support s) {
    switch (s) {
        case Support::Native: return 2;
        case Support::Emulated: return 1;
        case Support::Unavailable: return 0;
    }
    return 0;
}

CapabilityProfile::CapabilityProfile(KernelId kernel) : kernel_(kernel) {
    for (std::size_t i = 0; i < kCapabilityCount; ++i) {
        entries_[i].level = Support::Unavailable;
        entries_[i].primitive = "";
    }
}

void CapabilityProfile::set(Capability c, Support level, const char* primitive) {
    CapabilityEntry& e = entries_[static_cast<std::size_t>(c)];
    e.level = level;
    e.primitive = primitive ? primitive : "";
}

Support CapabilityProfile::query(Capability c) const {
    return entries_[static_cast<std::size_t>(c)].level;
}

const CapabilityEntry& CapabilityProfile::entry(Capability c) const {
    return entries_[static_cast<std::size_t>(c)];
}

std::size_t CapabilityProfile::native_count() const {
    std::size_t n = 0;
    for (std::size_t i = 0; i < kCapabilityCount; ++i) {
        if (entries_[i].level == Support::Native) ++n;
    }
    return n;
}

std::size_t CapabilityProfile::emulated_count() const {
    std::size_t n = 0;
    for (std::size_t i = 0; i < kCapabilityCount; ++i) {
        if (entries_[i].level == Support::Emulated) ++n;
    }
    return n;
}

std::size_t CapabilityProfile::missing_count() const {
    return kCapabilityCount - native_count() - emulated_count();
}

std::string CapabilityProfile::report() const {
    char line[128];
    std::string out = std::string(kernel_name(kernel_)) + " capabilities:\n";
    for (std::size_t i = 0; i < kCapabilityCount; ++i) {
        const Capability c = static_cast<Capability>(i);
        std::snprintf(line, sizeof line, "  %-16s %-12s %s\n", capability_name(c),
                      support_name(entries_[i].level), entries_[i].primitive);
        out += line;
    }
    std::snprintf(line, sizeof line, "  native=%zu emulated=%zu missing=%zu\n",
                  native_count(), emulated_count(), missing_count());
    out += line;
    return out;
}

CapabilityProfile profile_for(KernelId kernel) {
    CapabilityProfile p(kernel);
    switch (kernel) {
        case KernelId::Linux:
            p.set(Capability::Sandbox, Support::Native, "landlock");
            p.set(Capability::SyscallFilter, Support::Native, "seccomp-bpf");
            p.set(Capability::Namespace, Support::Native, "unshare");
            p.set(Capability::Container, Support::Native, "namespaces+cgroups");
            p.set(Capability::JobGroup, Support::Native, "cgroup v2");
            p.set(Capability::AsyncIo, Support::Native, "io_uring");
            p.set(Capability::HugePages, Support::Native, "madvise HUGEPAGE");
            p.set(Capability::ZeroCopyCopy, Support::Native, "copy_file_range");
            p.set(Capability::Semaphore, Support::Native, "futex");
            p.set(Capability::Signal, Support::Native, "signalfd");
            break;
        case KernelId::FreeBsd:
            p.set(Capability::Sandbox, Support::Native, "capsicum");
            p.set(Capability::SyscallFilter, Support::Emulated, "capsicum+syscall");
            p.set(Capability::Namespace, Support::Emulated, "jail");
            p.set(Capability::Container, Support::Native, "jails");
            p.set(Capability::JobGroup, Support::Emulated, "rctl");
            p.set(Capability::AsyncIo, Support::Emulated, "kqueue");
            p.set(Capability::HugePages, Support::Native, "superpages");
            p.set(Capability::ZeroCopyCopy, Support::Emulated, "sendfile");
            p.set(Capability::Semaphore, Support::Native, "umtx");
            p.set(Capability::Signal, Support::Emulated, "pipe+sigaction");
            break;
        case KernelId::WindowsNt:
            p.set(Capability::Sandbox, Support::Emulated, "job object");
            p.set(Capability::SyscallFilter, Support::Emulated, "mitigation policy");
            p.set(Capability::Namespace, Support::Unavailable, "");
            p.set(Capability::Container, Support::Emulated, "job object");
            p.set(Capability::JobGroup, Support::Native, "job object");
            p.set(Capability::AsyncIo, Support::Native, "IOCP");
            p.set(Capability::HugePages, Support::Native, "large pages");
            p.set(Capability::ZeroCopyCopy, Support::Native, "TransmitFile");
            p.set(Capability::Semaphore, Support::Native, "keyed event");
            p.set(Capability::Signal, Support::Emulated, "APC");
            break;
        case KernelId::Os2:
            p.set(Capability::Sandbox, Support::Unavailable, "");
            p.set(Capability::SyscallFilter, Support::Unavailable, "");
            p.set(Capability::Namespace, Support::Unavailable, "");
            p.set(Capability::Container, Support::Unavailable, "");
            p.set(Capability::JobGroup, Support::Unavailable, "");
            p.set(Capability::AsyncIo, Support::Unavailable, "");
            p.set(Capability::HugePages, Support::Unavailable, "");
            p.set(Capability::ZeroCopyCopy, Support::Unavailable, "");
            p.set(Capability::Semaphore, Support::Native, "DosRequestMutexSem");
            p.set(Capability::Signal, Support::Emulated, "DosKillProcess");
            break;
        case KernelId::Unknown:
            break;
    }
    return p;
}

FallbackChain& FallbackChain::then(Capability c) {
    steps_.push_back(c);
    return *this;
}

bool FallbackChain::contains(Capability c) const {
    return std::find(steps_.begin(), steps_.end(), c) != steps_.end();
}

void CapabilityPlanner::set_chain(Capability requested, const FallbackChain& chain) {
    chains_[requested] = chain;
}

const FallbackChain* CapabilityPlanner::chain(Capability requested) const {
    auto it = chains_.find(requested);
    if (it == chains_.end()) return nullptr;
    return &it->second;
}

Plan CapabilityPlanner::plan(Capability requested) const {
    Plan p;
    p.chosen = requested;

    FallbackChain single;
    const FallbackChain* c = chain(requested);
    const std::vector<Capability>* steps = nullptr;
    if (c != nullptr) {
        steps = &c->steps();
    } else {
        single.then(requested);
        steps = &single.steps();
    }

    const std::size_t limit = std::min(options_.max_hops, steps->size());
    bool have_acceptable = false;
    PlanStep first_acceptable;
    bool truncated = false;

    for (std::size_t i = 0; i < steps->size(); ++i) {
        if (i >= limit) {
            truncated = true;
            break;
        }
        const Capability cap = (*steps)[i];
        const CapabilityEntry& e = profile_.entry(cap);
        PlanStep s;
        s.capability = cap;
        s.level = e.level;
        s.primitive = e.primitive;

        const bool usable = e.level == Support::Native ||
                            (e.level == Support::Emulated && options_.allow_emulation);
        s.accepted = usable;

        if (usable && e.level == Support::Native) {
            p.trail.push_back(s);
            p.satisfied = true;
            p.chosen = cap;
            p.level = e.level;
            p.reason = "native support";
            return p;
        }
        if (usable && !have_acceptable) {
            have_acceptable = true;
            first_acceptable = s;
            first_acceptable.accepted = false;
        }
        if (options_.record_rejections || usable) p.trail.push_back(s);
    }

    if (have_acceptable && !options_.prefer_native) {
        p.trail.push_back(first_acceptable);
        p.satisfied = true;
        p.chosen = first_acceptable.capability;
        p.level = first_acceptable.level;
        p.reason = "emulated support accepted";
        return p;
    }
    if (have_acceptable && options_.prefer_native) {
        p.trail.push_back(first_acceptable);
        p.satisfied = true;
        p.chosen = first_acceptable.capability;
        p.level = first_acceptable.level;
        p.reason = "emulated fallback, no native step in chain";
        return p;
    }

    p.satisfied = false;
    p.chosen = requested;
    p.level = Support::Unavailable;
    p.reason = truncated ? "hop limit reached before a usable step" : "no usable step in chain";
    return p;
}

std::size_t CapabilityPlanner::plan_all(const std::vector<Capability>& requested,
                                        std::vector<Plan>& out) const {
    std::size_t ok = 0;
    out.clear();
    for (Capability c : requested) {
        Plan p = plan(c);
        if (p.satisfied) ++ok;
        out.push_back(p);
    }
    return ok;
}

FallbackChain default_sandbox_chain() {
    FallbackChain c;
    c.then(Capability::Sandbox).then(Capability::SyscallFilter).then(Capability::JobGroup);
    return c;
}

FallbackChain default_async_chain() {
    FallbackChain c;
    c.then(Capability::AsyncIo).then(Capability::Semaphore);
    return c;
}

FallbackChain default_isolation_chain() {
    FallbackChain c;
    c.then(Capability::Namespace).then(Capability::Container).then(Capability::JobGroup);
    return c;
}

}  // namespace kal
}  // namespace lowl
