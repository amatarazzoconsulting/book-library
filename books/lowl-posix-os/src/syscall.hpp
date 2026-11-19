// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : POSIX-style system call dispatch table.
// Mirrors posix/syscall.lp: a 512 slot table, every slot defaulting to
// syscall_unknown, syscall numbers in %rax with arguments in rdi/rsi/rdx/
// r10/r8/r9, and a return value written back to %rax. Adds argument
// validation, an errno style negative return, and per-call latency sampling
// so the benchmark can report real syscall cost.
#ifndef LOWL_SYSCALL_HPP
#define LOWL_SYSCALL_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "pcb.hpp"

namespace lowl {

// Syscall numbers exactly as listed in posix/syscall.lp.
namespace sysno {
constexpr std::uint64_t READ = 0;
constexpr std::uint64_t WRITE = 1;
constexpr std::uint64_t OPEN = 2;
constexpr std::uint64_t CLOSE = 3;
constexpr std::uint64_t STAT = 4;
constexpr std::uint64_t FSTAT = 5;
constexpr std::uint64_t LSEEK = 8;
constexpr std::uint64_t MMAP = 9;
constexpr std::uint64_t MUNMAP = 11;
constexpr std::uint64_t BRK = 12;
constexpr std::uint64_t SIGACTION = 13;
constexpr std::uint64_t SIGPROCMASK = 14;
constexpr std::uint64_t SIGRETURN = 15;
constexpr std::uint64_t IOCTL = 16;
constexpr std::uint64_t GETPID = 20;
constexpr std::uint64_t PIPE = 22;
constexpr std::uint64_t GETUID = 24;
constexpr std::uint64_t GETGID = 25;
constexpr std::uint64_t DUP = 32;
constexpr std::uint64_t DUP2 = 33;
constexpr std::uint64_t NICE = 34;
constexpr std::uint64_t GETPPID = 39;
constexpr std::uint64_t SOCKET = 41;
constexpr std::uint64_t FORK = 57;
constexpr std::uint64_t EXECVE = 59;
constexpr std::uint64_t EXIT = 60;
constexpr std::uint64_t WAIT4 = 61;
constexpr std::uint64_t KILL = 62;
constexpr std::uint64_t SETSID = 66;
constexpr std::uint64_t GETCWD = 79;
constexpr std::uint64_t CHDIR = 80;
constexpr std::uint64_t MKDIR = 83;
constexpr std::uint64_t RMDIR = 84;
constexpr std::uint64_t UNLINK = 87;
constexpr std::uint64_t SCHED_GET_PRIORITY = 140;
constexpr std::uint64_t SCHED_SET_PRIORITY = 141;
constexpr std::uint64_t NR = 512;
}  // namespace sysno

// Errno values returned as negative numbers, the convention the book uses.
namespace errno_ {
constexpr std::int64_t E_EPERM = 1;
constexpr std::int64_t E_ENOENT = 2;
constexpr std::int64_t E_EBADF = 9;
constexpr std::int64_t E_EAGAIN = 11;
constexpr std::int64_t E_ENOMEM = 12;
constexpr std::int64_t E_EFAULT = 14;
constexpr std::int64_t E_EEXIST = 17;
constexpr std::int64_t E_ENOTDIR = 20;
constexpr std::int64_t E_EISDIR = 21;
constexpr std::int64_t E_EINVAL = 22;
constexpr std::int64_t E_EMFILE = 24;
constexpr std::int64_t E_EPIPE = 32;
constexpr std::int64_t E_ENOSYS = 38;
constexpr std::int64_t E_ENOTEMPTY = 39;
constexpr std::int64_t E_ESRCH = 3;
}  // namespace errno_

// Six arguments as the kernel receives them from the interrupt frame.
struct SyscallArgs {
    std::uint64_t a0 = 0, a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0;
    std::uint64_t& operator[](int i);
    std::uint64_t operator[](int i) const;
};

struct SyscallResult {
    std::int64_t value = 0;   // >= 0 success, < 0 is -errno
    bool handled = false;     // false means the slot was empty
    bool is_error() const { return value < 0; }
};

// A syscall table slot. Returning SyscallResult lets a handler signal an
// error without overloading the return value's meaning.
using Handler = std::function<SyscallResult(const SyscallArgs&)>;

struct LatencyStats {
    std::uint64_t calls = 0;
    std::uint64_t errors = 0;
    std::uint64_t min_ns = 0;
    std::uint64_t max_ns = 0;
    std::uint64_t total_ns = 0;
    double mean_ns() const { return calls ? static_cast<double>(total_ns) / calls : 0.0; }
};

class SyscallTable {
public:
    SyscallTable();

    // Registering an unknown number is rejected, exactly like a table index
    // check in the kernel.
    bool install(std::uint64_t number, const std::string& name, Handler fn);
    bool uninstall(std::uint64_t number);
    bool installed(std::uint64_t number) const;
    const std::string& name_of(std::uint64_t number) const;
    std::vector<std::uint64_t> installed_numbers() const;
    std::size_t size() const { return installed_count_; }

    // The dispatcher: bounds check, empty-slot check, timing, accounting.
    SyscallResult dispatch(std::uint64_t number, const SyscallArgs& args);
    SyscallResult operator()(std::uint64_t number, const SyscallArgs& args) {
        return dispatch(number, args);
    }

    const LatencyStats& stats() const { return stats_; }
    const std::map<std::uint64_t, LatencyStats>& per_call() const { return per_; }
    void reset_stats();
    std::uint64_t unknown_calls() const { return unknown_calls_; }

    // Simulated user/kernel boundary crossing cost, in nanoseconds. The
    // benchmark sets this to model the real entry cost the book describes.
    void set_entry_cost(std::uint64_t ns) { entry_cost_ns_ = ns; }
    std::uint64_t entry_cost() const { return entry_cost_ns_; }

private:
    std::array<Handler, sysno::NR> table_{};
    std::array<std::string, sysno::NR> names_{};
    std::array<bool, sysno::NR> present_{};
    std::size_t installed_count_ = 0;
    std::uint64_t unknown_calls_ = 0;
    std::uint64_t entry_cost_ns_ = 0;
    LatencyStats stats_;
    std::map<std::uint64_t, LatencyStats> per_;
};

}  // namespace lowl

#endif  // LOWL_SYSCALL_HPP
