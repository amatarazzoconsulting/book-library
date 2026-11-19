// MIT License - (c) Anthony Matarazzo 2026
#include "syscall.hpp"

#include <chrono>

namespace lowl {

std::uint64_t& SyscallArgs::operator[](int i) {
    switch (i) {
        case 0: return a0;
        case 1: return a1;
        case 2: return a2;
        case 3: return a3;
        case 4: return a4;
        default: return a5;
    }
}

std::uint64_t SyscallArgs::operator[](int i) const {
    switch (i) {
        case 0: return a0;
        case 1: return a1;
        case 2: return a2;
        case 3: return a3;
        case 4: return a4;
        default: return a5;
    }
}

namespace {
const std::string kEmpty;

// A tiny busy loop that models the work of crossing the protection boundary.
// It is never optimised away because the result feeds the timing sample.
std::uint64_t burn(std::uint64_t ns, std::uint64_t seed) {
    if (ns == 0) return seed;
    auto start = std::chrono::steady_clock::now();
    std::uint64_t acc = seed;
    do {
        acc = acc * 6364136223846793005ull + 1442695040888963407ull;
    } while (std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::steady_clock::now() - start)
                 .count() < static_cast<long long>(ns));
    return acc;
}
}  // namespace

SyscallTable::SyscallTable() { names_.fill(std::string()); }

bool SyscallTable::install(std::uint64_t number, const std::string& name, Handler fn) {
    if (number >= sysno::NR) return false;
    if (!fn) return false;
    if (!present_[number]) ++installed_count_;
    table_[number] = std::move(fn);
    names_[number] = name;
    present_[number] = true;
    return true;
}

bool SyscallTable::uninstall(std::uint64_t number) {
    if (number >= sysno::NR || !present_[number]) return false;
    table_[number] = nullptr;
    names_[number].clear();
    present_[number] = false;
    --installed_count_;
    return true;
}

bool SyscallTable::installed(std::uint64_t number) const {
    return number < sysno::NR && present_[number];
}

const std::string& SyscallTable::name_of(std::uint64_t number) const {
    if (number >= sysno::NR) return kEmpty;
    return names_[number];
}

std::vector<std::uint64_t> SyscallTable::installed_numbers() const {
    std::vector<std::uint64_t> out;
    out.reserve(installed_count_);
    for (std::uint64_t i = 0; i < sysno::NR; ++i) {
        if (present_[i]) out.push_back(i);
    }
    return out;
}

void SyscallTable::reset_stats() {
    stats_ = LatencyStats();
    per_.clear();
    unknown_calls_ = 0;
}

SyscallResult SyscallTable::dispatch(std::uint64_t number, const SyscallArgs& args) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    SyscallResult r;
    if (number >= sysno::NR) {
        // Out of range: the kernel's `if sysno < 512` check fails.
        ++unknown_calls_;
        r.value = -errno_::E_ENOSYS;
        r.handled = false;
    } else if (!present_[number]) {
        ++unknown_calls_;
        r.value = -errno_::E_ENOSYS;
        r.handled = false;
    } else {
        // Model the boundary crossing cost before running the handler.
        if (entry_cost_ns_) burn(entry_cost_ns_, number ^ args.a0);
        r = table_[number](args);
        r.handled = true;
    }

    auto t1 = clock::now();
    std::uint64_t ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

    ++stats_.calls;
    if (r.value < 0) ++stats_.errors;
    stats_.total_ns += ns;
    if (stats_.min_ns == 0 || ns < stats_.min_ns) stats_.min_ns = ns;
    if (ns > stats_.max_ns) stats_.max_ns = ns;

    LatencyStats& p = per_[number];
    ++p.calls;
    if (r.value < 0) ++p.errors;
    p.total_ns += ns;
    if (p.min_ns == 0 || ns < p.min_ns) p.min_ns = ns;
    if (ns > p.max_ns) p.max_ns = ns;

    return r;
}

}  // namespace lowl
