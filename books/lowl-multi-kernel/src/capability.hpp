// MIT License - (c) Anthony Matarazzo 2026
//
// Capability query and fallback selection. Each kernel advertises a support
// level per capability; a request can be satisfied directly or through an
// ordered fallback chain, and the planner records every hop it takes.
#ifndef LOWL_KAL_CAPABILITY_HPP
#define LOWL_KAL_CAPABILITY_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "kernel_adapter.hpp"

namespace lowl {
namespace kal {

enum class Capability {
    Sandbox,        // filesystem confinement
    SyscallFilter,  // per-call filtering
    Namespace,      // per-process view isolation
    Container,      // whole-system partitioning
    JobGroup,       // process grouping with limits
    AsyncIo,        // completion based I/O
    HugePages,      // large page backing
    ZeroCopyCopy,   // kernel side file copy
    Semaphore,      // fast user visible lock
    Signal,         // asynchronous notification
    Count
};

constexpr std::size_t kCapabilityCount = static_cast<std::size_t>(Capability::Count);
const char* capability_name(Capability c);

enum class Support { Native, Emulated, Unavailable };
const char* support_name(Support s);
int support_rank(Support s);  // Native 2, Emulated 1, Unavailable 0

// What one kernel says about one capability.
struct CapabilityEntry {
    Support level{Support::Unavailable};
    const char* primitive{""};
};

class CapabilityProfile {
public:
    explicit CapabilityProfile(KernelId kernel);
    KernelId kernel() const { return kernel_; }

    void set(Capability c, Support level, const char* primitive);
    Support query(Capability c) const;
    const CapabilityEntry& entry(Capability c) const;
    bool has(Capability c) const { return query(c) != Support::Unavailable; }

    std::size_t native_count() const;
    std::size_t emulated_count() const;
    std::size_t missing_count() const;

    std::string report() const;

private:
    KernelId kernel_;
    CapabilityEntry entries_[kCapabilityCount];
};

// Built in profiles for the four simulated kernels, matching the security
// comparison in the book.
CapabilityProfile profile_for(KernelId kernel);

// An ordered fallback chain. The head is the requested capability, the tail
// holds progressively cheaper substitutes.
class FallbackChain {
public:
    FallbackChain& then(Capability c);
    const std::vector<Capability>& steps() const { return steps_; }
    std::size_t size() const { return steps_.size(); }
    bool contains(Capability c) const;

private:
    std::vector<Capability> steps_;
};

// Planner configuration, every field is exercised by teaching.cpp.
struct PlanOptions {
    bool allow_emulation{true};  // accept Emulated as a usable answer
    bool prefer_native{true};    // keep walking for a Native later in chain
    std::size_t max_hops{8};     // stop after this many chain entries
    bool record_rejections{true};
};

struct PlanStep {
    Capability capability{Capability::Sandbox};
    Support level{Support::Unavailable};
    const char* primitive{""};
    bool accepted{false};
};

struct Plan {
    bool satisfied{false};
    Capability chosen{Capability::Sandbox};
    Support level{Support::Unavailable};
    std::vector<PlanStep> trail;
    std::string reason;

    std::size_t hops() const { return trail.size(); }
};

class CapabilityPlanner {
public:
    CapabilityPlanner(const CapabilityProfile& profile, PlanOptions options = {})
        : profile_(profile), options_(options) {}

    // Register or replace the fallback chain for a capability.
    void set_chain(Capability requested, const FallbackChain& chain);
    const FallbackChain* chain(Capability requested) const;

    // Resolve a request. With no chain registered the capability itself is the
    // only candidate.
    Plan plan(Capability requested) const;

    // Convenience: resolve a whole list and report how many were satisfied.
    std::size_t plan_all(const std::vector<Capability>& requested,
                         std::vector<Plan>& out) const;

    const PlanOptions& options() const { return options_; }
    void set_options(const PlanOptions& o) { options_ = o; }

private:
    const CapabilityProfile& profile_;
    PlanOptions options_;
    std::map<Capability, FallbackChain> chains_;
};

// Standard chains used by the LOWL runtime: sandboxing degrades from
// landlock to seccomp to nothing, and async I/O degrades to a thread pool.
FallbackChain default_sandbox_chain();
FallbackChain default_async_chain();
FallbackChain default_isolation_chain();

}  // namespace kal
}  // namespace lowl

#endif  // LOWL_KAL_CAPABILITY_HPP
