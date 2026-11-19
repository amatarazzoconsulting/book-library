// MIT License - (c) Anthony Matarazzo 2026
//
// Kernel abstraction layer: one adapter interface, four simulated kernels.
// The costs are modelled on the ratio table in "LOWL Multi Kernel"
// (Linux baseline 1.0) so that cross-kernel behaviour is reproducible.
#ifndef LOWL_KAL_KERNEL_ADAPTER_HPP
#define LOWL_KAL_KERNEL_ADAPTER_HPP

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace lowl {
namespace kal {

enum class KernelId { Linux, FreeBsd, WindowsNt, Os2, Unknown };
const char* kernel_name(KernelId id);

// Abstract operations the upper layers of LOWL are allowed to perform.
enum class Op {
    Open,
    Close,
    Read,
    Write,
    Fork,
    ThreadCreate,
    Mmap,
    SemAcquire,
    SignalDeliver,
    AsyncIo,
    TcpConnect,
    Stat,
    Count
};

constexpr std::size_t kOpCount = static_cast<std::size_t>(Op::Count);
const char* op_name(Op op);

enum class Status { Ok, Unsupported, BadHandle, LimitReached };
const char* status_name(Status s);

struct CallResult {
    Status status{Status::Ok};
    long value{0};
    double cost_ns{0.0};
    bool emulated{false};
};

// One row of the per-kernel dispatch table.
struct OpProfile {
    double cost_ns{0.0};
    unsigned native_nr{0};
    bool supported{true};
    bool emulated{false};
};

struct Stats {
    unsigned long calls{0};
    unsigned long unsupported{0};
    unsigned long emulated{0};
    unsigned long failures{0};
    double total_cost_ns{0.0};
};

// Abstract base class. Concrete adapters fill the dispatch table in their
// constructor and supply name, ABI string and probe score.
class KernelAdapter {
public:
    explicit KernelAdapter(KernelId id, std::size_t handle_limit = 1024);
    virtual ~KernelAdapter() = default;

    KernelId id() const { return id_; }
    virtual const char* name() const = 0;
    virtual const char* abi() const = 0;
    virtual int probe_score() const = 0;

    bool supports(Op op) const;
    bool is_emulated(Op op) const;
    const OpProfile& profile(Op op) const;

    // Dispatch one abstract operation. Tracks handles so that Close without
    // Open and reads without an open handle fail deterministically.
    CallResult call(Op op, long arg = 0);

    const Stats& stats() const { return stats_; }
    void reset_stats();
    std::size_t open_handles() const { return open_handles_; }
    std::size_t handle_limit() const { return handle_limit_; }

    // Cost ratio against another adapter, 0.0 when the baseline is free.
    double ratio_vs(const KernelAdapter& baseline, Op op) const;

    virtual std::string describe() const;

protected:
    void set(Op op, double cost_ns, unsigned native_nr, bool supported = true,
             bool emulated = false);
    std::array<OpProfile, kOpCount> table_{};

private:
    KernelId id_;
    std::size_t handle_limit_;
    std::size_t open_handles_{0};
    Stats stats_{};
};

class LinuxAdapter : public KernelAdapter {
public:
    LinuxAdapter();
    const char* name() const override { return "Linux"; }
    const char* abi() const override { return "syscall insn, rax number, rdi/rsi/rdx args"; }
    int probe_score() const override { return 100; }
};

class FreeBsdAdapter : public KernelAdapter {
public:
    FreeBsdAdapter();
    const char* name() const override { return "FreeBSD"; }
    const char* abi() const override { return "syscall insn, rax number, rdi/rsi/rdx args"; }
    int probe_score() const override { return 70; }
};

class WindowsNtAdapter : public KernelAdapter {
public:
    WindowsNtAdapter();
    const char* name() const override { return "Windows NT"; }
    const char* abi() const override { return "Nt* stub, eax number, r10/r8/rdx args"; }
    int probe_score() const override { return 50; }
};

class Os2Adapter : public KernelAdapter {
public:
    Os2Adapter();
    const char* name() const override { return "OS/2"; }
    const char* abi() const override { return "Dos* call gate, ordinal in ax"; }
    int probe_score() const override { return 10; }
};

// Factory used by the runtime selection step.
std::unique_ptr<KernelAdapter> make_adapter(KernelId id);

// Startup probing: candidates are scored and the highest score wins.
// Returns nullptr only for an empty candidate list.
std::unique_ptr<KernelAdapter> select_adapter(const std::vector<KernelId>& candidates);

// Same selection over already constructed adapters, without taking ownership.
KernelAdapter* select_best(const std::vector<KernelAdapter*>& candidates);

}  // namespace kal
}  // namespace lowl

#endif  // LOWL_KAL_KERNEL_ADAPTER_HPP
