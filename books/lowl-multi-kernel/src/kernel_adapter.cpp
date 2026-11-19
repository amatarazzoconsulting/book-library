// MIT License - (c) Anthony Matarazzo 2026
#include "kernel_adapter.hpp"

#include <algorithm>

namespace lowl {
namespace kal {

namespace {
// Baseline costs in nanoseconds for the Linux reference adapter. The book
// quotes 100-200 ns for a simple Linux system call, so the simple ops sit in
// that band and the heavier ones scale from there.
constexpr double kLinuxOpen = 850.0;
constexpr double kLinuxClose = 320.0;
constexpr double kLinuxRead = 620.0;
constexpr double kLinuxWrite = 640.0;
constexpr double kLinuxFork = 62000.0;
constexpr double kLinuxThread = 14000.0;
constexpr double kLinuxMmap = 1900.0;
constexpr double kLinuxSem = 260.0;
constexpr double kLinuxSignal = 2400.0;
constexpr double kLinuxAsync = 900.0;
constexpr double kLinuxTcp = 41000.0;
constexpr double kLinuxStat = 480.0;
}  // namespace

const char* kernel_name(KernelId id) {
    switch (id) {
        case KernelId::Linux: return "Linux";
        case KernelId::FreeBsd: return "FreeBSD";
        case KernelId::WindowsNt: return "Windows NT";
        case KernelId::Os2: return "OS/2";
        case KernelId::Unknown: break;
    }
    return "Unknown";
}

const char* op_name(Op op) {
    switch (op) {
        case Op::Open: return "open";
        case Op::Close: return "close";
        case Op::Read: return "read";
        case Op::Write: return "write";
        case Op::Fork: return "fork";
        case Op::ThreadCreate: return "thread_create";
        case Op::Mmap: return "mmap";
        case Op::SemAcquire: return "sem_acquire";
        case Op::SignalDeliver: return "signal_deliver";
        case Op::AsyncIo: return "async_io";
        case Op::TcpConnect: return "tcp_connect";
        case Op::Stat: return "stat";
        case Op::Count: break;
    }
    return "unknown";
}

const char* status_name(Status s) {
    switch (s) {
        case Status::Ok: return "ok";
        case Status::Unsupported: return "unsupported";
        case Status::BadHandle: return "bad_handle";
        case Status::LimitReached: return "limit_reached";
    }
    return "unknown";
}

KernelAdapter::KernelAdapter(KernelId id, std::size_t handle_limit)
    : id_(id), handle_limit_(handle_limit) {}

void KernelAdapter::set(Op op, double cost_ns, unsigned native_nr, bool supported,
                        bool emulated) {
    OpProfile& p = table_[static_cast<std::size_t>(op)];
    p.cost_ns = cost_ns;
    p.native_nr = native_nr;
    p.supported = supported;
    p.emulated = emulated;
}

bool KernelAdapter::supports(Op op) const { return profile(op).supported; }

bool KernelAdapter::is_emulated(Op op) const { return profile(op).emulated; }

const OpProfile& KernelAdapter::profile(Op op) const {
    return table_[static_cast<std::size_t>(op)];
}

CallResult KernelAdapter::call(Op op, long arg) {
    (void)arg;
    CallResult r;
    const OpProfile& p = profile(op);
    ++stats_.calls;
    r.emulated = p.emulated;

    if (!p.supported) {
        r.status = Status::Unsupported;
        ++stats_.unsupported;
        ++stats_.failures;
        return r;
    }

    if (op == Op::Open) {
        if (open_handles_ >= handle_limit_) {
            r.status = Status::LimitReached;
            ++stats_.failures;
            return r;
        }
        ++open_handles_;
        r.value = static_cast<long>(open_handles_);
    } else if (op == Op::Close) {
        if (open_handles_ == 0) {
            r.status = Status::BadHandle;
            ++stats_.failures;
            return r;
        }
        r.value = static_cast<long>(open_handles_);
        --open_handles_;
    } else if (op == Op::Read || op == Op::Write || op == Op::Stat) {
        if (open_handles_ == 0) {
            r.status = Status::BadHandle;
            ++stats_.failures;
            return r;
        }
        r.value = 4096;
    } else {
        r.value = 1;
    }

    r.cost_ns = p.cost_ns;
    stats_.total_cost_ns += p.cost_ns;
    if (p.emulated) ++stats_.emulated;
    return r;
}

void KernelAdapter::reset_stats() {
    stats_ = Stats{};
    open_handles_ = 0;
}

double KernelAdapter::ratio_vs(const KernelAdapter& baseline, Op op) const {
    const double base = baseline.profile(op).cost_ns;
    if (base <= 0.0) return 0.0;
    return profile(op).cost_ns / base;
}

std::string KernelAdapter::describe() const {
    std::string s = name();
    s += " [";
    s += abi();
    s += "] probe=";
    s += std::to_string(probe_score());
    return s;
}

LinuxAdapter::LinuxAdapter() : KernelAdapter(KernelId::Linux) {
    set(Op::Open, kLinuxOpen, 2);
    set(Op::Close, kLinuxClose, 3);
    set(Op::Read, kLinuxRead, 0);
    set(Op::Write, kLinuxWrite, 1);
    set(Op::Fork, kLinuxFork, 57);
    set(Op::ThreadCreate, kLinuxThread, 56);
    set(Op::Mmap, kLinuxMmap, 9);
    set(Op::SemAcquire, kLinuxSem, 202);
    set(Op::SignalDeliver, kLinuxSignal, 62);
    set(Op::AsyncIo, kLinuxAsync, 425);
    set(Op::TcpConnect, kLinuxTcp, 42);
    set(Op::Stat, kLinuxStat, 4);
}

FreeBsdAdapter::FreeBsdAdapter() : KernelAdapter(KernelId::FreeBsd) {
    set(Op::Open, kLinuxOpen * 1.1, 5);
    set(Op::Close, kLinuxClose * 1.1, 6);
    set(Op::Read, kLinuxRead * 1.0, 3);
    set(Op::Write, kLinuxWrite * 1.0, 4);
    set(Op::Fork, kLinuxFork * 1.3, 2);
    set(Op::ThreadCreate, kLinuxThread * 1.1, 455);
    set(Op::Mmap, kLinuxMmap * 1.0, 197);
    set(Op::SemAcquire, kLinuxSem * 1.2, 468);
    set(Op::SignalDeliver, kLinuxSignal * 1.3, 103);
    // FreeBSD has no io_uring; kqueue emulation costs about 2.5x.
    set(Op::AsyncIo, kLinuxAsync * 2.5, 363, true, true);
    set(Op::TcpConnect, kLinuxTcp * 1.1, 98);
    set(Op::Stat, kLinuxStat * 1.1, 188);
}

WindowsNtAdapter::WindowsNtAdapter() : KernelAdapter(KernelId::WindowsNt) {
    set(Op::Open, kLinuxOpen * 1.4, 51);
    set(Op::Close, kLinuxClose * 1.4, 12);
    set(Op::Read, kLinuxRead * 1.1, 6);
    set(Op::Write, kLinuxWrite * 1.1, 8);
    // No fork primitive: NtCreateProcess plus address space copy.
    set(Op::Fork, kLinuxFork * 850.0, 165, true, true);
    set(Op::ThreadCreate, kLinuxThread * 2.2, 194);
    set(Op::Mmap, kLinuxMmap * 1.5, 39);
    set(Op::SemAcquire, kLinuxSem * 2.0, 86);
    set(Op::SignalDeliver, kLinuxSignal * 15.0, 177, true, true);
    set(Op::AsyncIo, kLinuxAsync * 5.0, 59);
    set(Op::TcpConnect, kLinuxTcp * 1.3, 49);
    set(Op::Stat, kLinuxStat * 1.4, 56);
}

Os2Adapter::Os2Adapter() : KernelAdapter(KernelId::Os2) {
    set(Op::Open, kLinuxOpen * 3.2, 0x76);
    set(Op::Close, kLinuxClose * 3.2, 0x77);
    set(Op::Read, kLinuxRead * 2.8, 0x72);
    set(Op::Write, kLinuxWrite * 2.9, 0x73);
    set(Op::Fork, kLinuxFork * 2100.0, 0x5B, true, true);
    set(Op::ThreadCreate, kLinuxThread * 5.8, 0x0A);
    set(Op::Mmap, kLinuxMmap * 3.0, 0x51);
    set(Op::SemAcquire, kLinuxSem * 8.0, 0x2A);
    set(Op::SignalDeliver, kLinuxSignal * 25.0, 0x38, true, true);
    // OS/2 has no native async I/O; the adapter reports it unsupported.
    set(Op::AsyncIo, 0.0, 0, false, false);
    set(Op::TcpConnect, kLinuxTcp * 4.2, 0x60);
    set(Op::Stat, kLinuxStat * 3.2, 0x4E);
}

std::unique_ptr<KernelAdapter> make_adapter(KernelId id) {
    switch (id) {
        case KernelId::Linux: return std::unique_ptr<KernelAdapter>(new LinuxAdapter());
        case KernelId::FreeBsd: return std::unique_ptr<KernelAdapter>(new FreeBsdAdapter());
        case KernelId::WindowsNt:
            return std::unique_ptr<KernelAdapter>(new WindowsNtAdapter());
        case KernelId::Os2: return std::unique_ptr<KernelAdapter>(new Os2Adapter());
        case KernelId::Unknown: break;
    }
    return nullptr;
}

KernelAdapter* select_best(const std::vector<KernelAdapter*>& candidates) {
    KernelAdapter* best = nullptr;
    for (KernelAdapter* c : candidates) {
        if (c == nullptr) continue;
        if (best == nullptr || c->probe_score() > best->probe_score()) best = c;
    }
    return best;
}

std::unique_ptr<KernelAdapter> select_adapter(const std::vector<KernelId>& candidates) {
    std::vector<std::unique_ptr<KernelAdapter>> built;
    std::vector<KernelAdapter*> raw;
    for (KernelId id : candidates) {
        std::unique_ptr<KernelAdapter> a = make_adapter(id);
        if (!a) continue;
        raw.push_back(a.get());
        built.push_back(std::move(a));
    }
    KernelAdapter* best = select_best(raw);
    if (best == nullptr) return nullptr;
    for (std::size_t i = 0; i < built.size(); ++i) {
        if (built[i].get() == best) return std::move(built[i]);
    }
    return nullptr;
}

}  // namespace kal
}  // namespace lowl
