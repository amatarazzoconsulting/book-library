// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : process control block and process table.
// Mirrors posix/process.lp from the book: states, scheduling policy,
// priority levels, flags, a register context and the parent/child links
// needed for fork, exit, wait and zombie reaping.
#ifndef LOWL_PCB_HPP
#define LOWL_PCB_HPP

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace lowl {

// ProcessState from posix/process.lp. Terminated is the final state of a
// reaped process; Zombie is the state between exit and wait.
enum class ProcState : std::uint8_t {
    New = 0, Ready = 1, Running = 2, Waiting = 3, Terminated = 4, Zombie = 5, Stopped = 6
};

// SchedulingPolicy from posix/process.lp.
enum class Policy : std::uint8_t { Fifo = 0, Rr = 1, Cfs = 2 };

const char* to_string(ProcState s);
const char* to_string(Policy p);
bool parse_state(const std::string& text, ProcState& out);
bool parse_policy(const std::string& text, Policy& out);

// ProcessFlags
namespace pflags {
constexpr std::uint32_t KTHREAD = 0x00000001;
constexpr std::uint32_t USER = 0x00000002;
constexpr std::uint32_t SIGNALED = 0x00000004;
constexpr std::uint32_t EXITING = 0x00000008;
constexpr std::uint32_t FORKED = 0x00000010;
constexpr std::uint32_t TRACED = 0x00000020;
}  // namespace pflags

// ProcessPriority. The book lists IDLE..CRITICAL as -20..40 but its run queue
// array only has 41 slots covering -20..20, so every priority is clamped to
// [MIN, MAX] on the way into the scheduler.
namespace prio {
constexpr std::int8_t IDLE = -20;
constexpr std::int8_t LOW = 0;
constexpr std::int8_t NORMAL = 10;
constexpr std::int8_t HIGH = 20;
constexpr std::int8_t REALTIME = 30;
constexpr std::int8_t CRITICAL = 40;
constexpr std::int8_t MIN = -20;
constexpr std::int8_t MAX = 20;
std::int8_t clamp(int value);
const char* label(std::int8_t p);
}  // namespace prio

// Standard signals (posix/signal.lp), 1..64 as in the book.
namespace signo {
constexpr std::uint8_t HUP = 1;
constexpr std::uint8_t INT = 2;
constexpr std::uint8_t QUIT = 3;
constexpr std::uint8_t ILL = 4;
constexpr std::uint8_t TRAP = 5;
constexpr std::uint8_t ABRT = 6;
constexpr std::uint8_t BUS = 7;
constexpr std::uint8_t FPE = 8;
constexpr std::uint8_t KILL = 9;
constexpr std::uint8_t USR1 = 10;
constexpr std::uint8_t SEGV = 11;
constexpr std::uint8_t USR2 = 12;
constexpr std::uint8_t PIPE = 13;
constexpr std::uint8_t ALRM = 14;
constexpr std::uint8_t TERM = 15;
constexpr std::uint8_t CHLD = 17;
constexpr std::uint8_t CONT = 18;
constexpr std::uint8_t STOP = 19;
constexpr std::uint8_t TSTP = 20;
constexpr int COUNT = 64;
constexpr std::int32_t SIG_DFL = -1;
constexpr std::int32_t SIG_IGN = -2;
const char* name(int sig);
std::uint64_t bit(int sig);
}  // namespace signo

// ContextRegisters, trimmed to the registers a user-space simulator can show.
struct Registers {
    std::uint64_t rax = 0, rbx = 0, rcx = 0, rdx = 0;
    std::uint64_t rsi = 0, rdi = 0, rbp = 0, rsp = 0;
    std::uint64_t rip = 0, rflags = 0;
};

struct PCB {
    // Identification
    std::int32_t pid = -1;
    std::int32_t ppid = -1;
    std::int32_t tgid = -1;
    std::uint32_t uid = 0;
    std::uint32_t gid = 0;

    // State
    ProcState state = ProcState::New;
    std::uint32_t flags = 0;
    std::int32_t exit_code = 0;
    std::uint8_t exit_signal = 0;

    // Scheduling
    std::int8_t priority = prio::NORMAL;
    std::int8_t nice = 0;
    Policy policy = Policy::Rr;
    std::uint32_t time_slice = 0;
    std::uint64_t vruntime = 0;
    std::uint64_t last_ran = 0;

    // Context
    Registers regs;

    // Timing (ticks of the simulated 1 kHz clock)
    std::uint64_t start_time = 0;
    std::uint64_t user_time = 0;
    std::uint64_t system_time = 0;

    // Signals: one handler slot per signal, bitmask of pending signals.
    std::array<std::int32_t, signo::COUNT + 1> sig_handlers{};
    std::uint64_t pending_signals = 0;
    std::uint64_t sigmask = 0;

    // Simulated CPU demand of this job, in ticks. Used by the scheduler model.
    std::uint64_t burst = 0;

    // Children and wait bookkeeping
    std::vector<std::int32_t> children;
    std::int32_t wait_target = -1;

    std::string name;

    PCB();
};

enum class WaitStatus : std::uint8_t { Reaped, WouldBlock, NoChild, BadPid };

// ProcessTable owns every PCB. Pointers returned by get() stay valid until
// reap() is called for that pid, so the scheduler can hold PCB* safely.
class ProcessTable {
public:
    ProcessTable();

    std::int32_t create(const std::string& name, std::int32_t ppid = -1,
                        std::int8_t priority = prio::NORMAL, Policy policy = Policy::Rr,
                        std::uint32_t uid = 0);
    std::int32_t fork(std::int32_t parent);

    PCB* get(std::int32_t pid);
    const PCB* get(std::int32_t pid) const;
    bool has(std::int32_t pid) const;

    // Moves the process to Zombie, records the code, reparents orphans to
    // init and raises SIGCHLD in the parent. Returns false for unknown or
    // already dead pids.
    bool exit(std::int32_t pid, std::int32_t code, std::uint8_t sig = 0);

    WaitStatus wait(std::int32_t parent, std::int32_t target, std::int32_t* status,
                    std::int32_t* reaped_pid = nullptr);
    bool reap(std::int32_t pid);

    bool set_state(std::int32_t pid, ProcState st);
    bool set_priority(std::int32_t pid, int priority);
    bool set_policy(std::int32_t pid, Policy p);

    std::size_t count(ProcState st) const;
    std::size_t live_count() const;
    std::size_t zombie_count() const;
    std::size_t size() const { return procs_.size(); }

    std::vector<std::int32_t> pids() const;
    std::vector<PCB*> all();
    std::vector<const PCB*> all() const;
    std::vector<std::int32_t> children_of(std::int32_t pid) const;

    std::uint64_t clock() const { return clock_; }
    void advance(std::uint64_t ticks) { clock_ += ticks; }
    void set_clock(std::uint64_t t) { clock_ = t; }

    static constexpr std::int32_t INIT_PID = 1;

private:
    std::map<std::int32_t, std::unique_ptr<PCB>> procs_;
    std::int32_t next_pid_ = 1;
    std::uint64_t clock_ = 0;
};

}  // namespace lowl

#endif  // LOWL_PCB_HPP
