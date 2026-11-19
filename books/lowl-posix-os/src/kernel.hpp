// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : user-space kernel simulator.
// Ties the four algorithms together the way posix/boot.lp does: a process
// table, a scheduler, an inode filesystem with per-process fd tables, a
// syscall dispatch table and signal delivery with pipes. Everything is
// single threaded and deterministic, driven by an explicit tick counter.
#ifndef LOWL_KERNEL_HPP
#define LOWL_KERNEL_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "pcb.hpp"
#include "sched.hpp"
#include "sig.hpp"
#include "syscall.hpp"
#include "vfs.hpp"

namespace lowl {

struct SyscallTrace {
    std::uint64_t number = 0;
    std::string name;
    std::int64_t result = 0;
    std::int32_t pid = -1;
};

class Kernel {
public:
    Kernel();

    // boot(): format the filesystem, create init (pid 1) and register every
    // syscall handler. Idempotent enough to be called once per test.
    void boot();
    void reset();

    ProcessTable& procs() { return procs_; }
    Scheduler& sched() { return sched_; }
    vfs::FileSystem& fs() { return fs_; }
    SignalManager& signals() { return sigs_; }
    SyscallTable& syscalls() { return syscalls_; }
    PipeTable& pipes() { return pipes_; }

    // Per-process fd tables and cwd, keyed by pid. Created on demand.
    vfs::FdTable& fds(std::int32_t pid);
    std::uint32_t cwd(std::int32_t pid) const;
    void set_cwd(std::int32_t pid, std::uint32_t ino);

    // create_process(): new PCB, Ready, admitted to the scheduler.
    std::int32_t spawn(const std::string& name, std::int8_t priority = prio::NORMAL,
                       Policy policy = Policy::Rr, std::uint64_t burst = 10);
    // fork() of the running process, per sys_fork().
    std::int32_t fork_current();
    // exit() of the running process.
    void exit_current(std::int32_t code);

    // The syscall entry point: executes a number with six arguments as the
    // current process and writes the result back. Records a trace.
    std::int64_t syscall(std::uint64_t number, const SyscallArgs& args);
    std::int64_t syscall(std::uint64_t number, std::uint64_t a0 = 0, std::uint64_t a1 = 0,
                         std::uint64_t a2 = 0, std::uint64_t a3 = 0);

    const std::vector<SyscallTrace>& trace() const { return trace_; }
    void clear_trace() { trace_.clear(); }
    bool trace_enabled = false;

    // Advance the simulated clock; each tick runs the scheduler and delivers
    // pending signals for the running process.
    void tick();
    void run(std::uint64_t ticks);

    std::int32_t current() const { return sched_.current(); }
    std::uint64_t clock() const { return sched_.clock(); }

    // Console output captured from fd 1/2 writes, per process.
    std::string console(std::int32_t pid) const;

    // Install the 36 handlers the book registers, plus pipe/dup helpers.
    void install_syscalls();

    // User-space memory stand-in. A pathname argument is a handle returned by
    // intern(): the kernel side of a pointer the book would dereference.
    std::uint64_t intern(const std::string& text);
    const std::string* user_string(std::uint64_t handle) const;

private:
    std::int32_t current_pid() const;
    void validate_pid(std::int32_t pid);

    ProcessTable procs_;
    Scheduler sched_;
    vfs::FileSystem fs_;
    SignalManager sigs_;
    SyscallTable syscalls_;
    PipeTable pipes_;
    std::map<std::int32_t, vfs::FdTable> fd_tables_;
    std::map<std::int32_t, std::uint32_t> cwd_;
    std::vector<SyscallTrace> trace_;
    std::vector<std::string> user_strings_;
    std::int32_t console_pid_ = -1;
};

}  // namespace lowl

#endif  // LOWL_KERNEL_HPP
