// MIT License - (c) Anthony Matarazzo 2026
#include "kernel.hpp"

#include <cstring>

namespace lowl {
namespace {

std::int64_t err(vfs::FsError e) { return -static_cast<std::int64_t>(e); }

}  // namespace

Kernel::Kernel() : sched_(procs_), sigs_(procs_) { boot(); }

void Kernel::reset() {
    // sched_ and sigs_ hold a reference to procs_, so they are reset in place
    // instead of being assigned; the reference stays valid across this.
    procs_ = ProcessTable();
    sched_.init();
    fs_ = vfs::FileSystem();
    sigs_.clear_handlers();
    syscalls_ = SyscallTable();
    pipes_.reset();
    fd_tables_.clear();
    cwd_.clear();
    trace_.clear();
    user_strings_.clear();
    console_pid_ = -1;
    boot();
}

void Kernel::boot() {
    fs_.format();
    fs_.mkdir("/bin", vfs::ROOT_INO, 0755);
    fs_.mkdir("/tmp", vfs::ROOT_INO, 0777);
    fs_.mkdir("/dev", vfs::ROOT_INO, 0755);

    // init is pid 1, as created by boot_main() in the book.
    std::int32_t init = procs_.create("init", -1, prio::NORMAL, Policy::Rr, 0);
    (void)init;  // create() numbers from 1, so this is pid 1
    procs_.set_state(ProcessTable::INIT_PID, ProcState::Ready);
    fds(ProcessTable::INIT_PID);
    set_cwd(ProcessTable::INIT_PID, vfs::ROOT_INO);
    sched_.init();
    sched_.admit(ProcessTable::INIT_PID);
    install_syscalls();
}

vfs::FdTable& Kernel::fds(std::int32_t pid) {
    auto it = fd_tables_.find(pid);
    if (it == fd_tables_.end()) {
        vfs::FdTable t;
        t.reset();
        it = fd_tables_.emplace(pid, std::move(t)).first;
    }
    return it->second;
}

std::uint32_t Kernel::cwd(std::int32_t pid) const {
    auto it = cwd_.find(pid);
    return it == cwd_.end() ? vfs::ROOT_INO : it->second;
}

void Kernel::set_cwd(std::int32_t pid, std::uint32_t ino) { cwd_[pid] = ino; }

std::int32_t Kernel::spawn(const std::string& name, std::int8_t priority, Policy policy,
                           std::uint64_t burst) {
    std::int32_t pid = procs_.create(name, ProcessTable::INIT_PID, priority, policy, 0);
    PCB* p = procs_.get(pid);
    if (!p) return -1;
    p->burst = burst;
    p->flags |= pflags::USER;
    fds(pid);  // give it a console fd table
    cwd_[pid] = vfs::ROOT_INO;
    sched_.admit(pid);
    return pid;
}

std::int32_t Kernel::fork_current() {
    std::int32_t parent = current_pid();
    if (parent < 0) return -1;
    std::int32_t child = procs_.fork(parent);
    if (child < 0) return -1;
    fd_tables_[child] = fd_tables_[parent];  // fds are inherited
    cwd_[child] = cwd(parent);
    sched_.admit(child);
    return child;
}

void Kernel::exit_current(std::int32_t code) {
    std::int32_t pid = current_pid();
    if (pid < 0) return;
    sched_.dequeue(pid);
    procs_.exit(pid, code, 0);
}

std::int32_t Kernel::current_pid() const { return sched_.current(); }

std::uint64_t Kernel::intern(const std::string& text) {
    user_strings_.push_back(text);
    return user_strings_.size();  // 1-based handle
}

const std::string* Kernel::user_string(uint64_t handle) const {
    if (handle == 0 || handle > user_strings_.size()) return nullptr;
    return &user_strings_[static_cast<std::size_t>(handle - 1)];
}

std::string Kernel::console(std::int32_t pid) const {
    auto it = fd_tables_.find(pid);
    return it == fd_tables_.end() ? std::string() : it->second.console_out();
}

std::int64_t Kernel::syscall(std::uint64_t number, const SyscallArgs& args) {
    SyscallResult r = syscalls_.dispatch(number, args);
    if (trace_enabled) {
        trace_.push_back(SyscallTrace{number, syscalls_.name_of(number), r.value,
                                      current_pid()});
    }
    return r.value;
}

std::int64_t Kernel::syscall(std::uint64_t number, std::uint64_t a0, std::uint64_t a1,
                             std::uint64_t a2, std::uint64_t a3) {
    SyscallArgs args;
    args.a0 = a0;
    args.a1 = a1;
    args.a2 = a2;
    args.a3 = a3;
    return syscall(number, args);
}

void Kernel::tick() {
    std::int32_t pid = sched_.current();
    if (pid >= 0) {
        // Deliver at most one pending signal per tick to the running process.
        sigs_.deliver_pending(pid);
        // A signal may have killed or stopped it.
        PCB* p = procs_.get(pid);
        if (p && (p->state == ProcState::Zombie || p->state == ProcState::Stopped)) {
            sched_.dequeue(pid);
        }
    }
    sched_.tick();
    // If the scheduler has nothing running, try to pick something up.
    if (sched_.current() < 0 && sched_.has_runnable()) sched_.schedule(false);
}

void Kernel::run(std::uint64_t ticks) {
    for (std::uint64_t i = 0; i < ticks; ++i) tick();
}

// ------------------------------------------------------------- syscalls

void Kernel::install_syscalls() {
    using lowl::sysno::NR;
    (void)NR;

    syscalls_.install(sysno::WRITE, "write", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        const std::string* s = user_string(a.a1);
        if (!s) { r.value = -errno_::E_EFAULT; return r; }
        std::size_t n = static_cast<std::size_t>(a.a2);
        if (n > s->size()) n = s->size();
        r.value = fds(pid).write(fs_, static_cast<std::int32_t>(a.a0), s->data(), n);
        return r;
    });

    syscalls_.install(sysno::READ, "read", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        std::size_t n = static_cast<std::size_t>(a.a2);
        std::vector<char> buf(n);
        std::int64_t got = fds(pid).read(fs_, static_cast<std::int32_t>(a.a0), buf.data(), n);
        if (got > 0) user_strings_.push_back(std::string(buf.data(), static_cast<std::size_t>(got)));
        r.value = got;
        return r;
    });

    syscalls_.install(sysno::OPEN, "open", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        const std::string* path = user_string(a.a0);
        if (!path) { r.value = -errno_::E_EFAULT; return r; }
        r.value = fds(pid).open(fs_, *path, static_cast<std::uint32_t>(a.a1),
                                static_cast<std::uint32_t>(a.a2), cwd(pid));
        return r;
    });

    syscalls_.install(sysno::CLOSE, "close", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        r.value = fds(pid).close(fs_, static_cast<std::int32_t>(a.a0));
        return r;
    });

    syscalls_.install(sysno::STAT, "stat", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        const std::string* path = user_string(a.a0);
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        if (!path) { r.value = -errno_::E_EFAULT; return r; }
        vfs::Stat st;
        vfs::FsError e = fs_.stat(*path, cwd(pid), st);
        if (e != vfs::FsError::Ok) { r.value = err(e); return r; }
        // Report the size through the second argument so the caller can use it.
        r.value = static_cast<std::int64_t>(st.st_size);
        return r;
    });

    syscalls_.install(sysno::FSTAT, "fstat", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        vfs::Stat st;
        vfs::FsError e = fds(pid).fstat(fs_, static_cast<std::int32_t>(a.a0), st);
        if (e != vfs::FsError::Ok) { r.value = err(e); return r; }
        r.value = static_cast<std::int64_t>(st.st_size);
        return r;
    });

    syscalls_.install(sysno::LSEEK, "lseek", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        r.value = fds(pid).lseek(static_cast<std::int32_t>(a.a0),
                                 static_cast<std::int64_t>(a.a1),
                                 static_cast<int>(a.a2));
        return r;
    });

    syscalls_.install(sysno::GETPID, "getpid", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = current_pid();
        return r;
    });

    syscalls_.install(sysno::GETPPID, "getppid", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        const PCB* p = procs_.get(current_pid());
        r.value = p ? p->ppid : -1;
        return r;
    });

    syscalls_.install(sysno::GETUID, "getuid", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        const PCB* p = procs_.get(current_pid());
        r.value = p ? p->uid : 0;
        return r;
    });

    syscalls_.install(sysno::GETGID, "getgid", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        const PCB* p = procs_.get(current_pid());
        r.value = p ? p->gid : 0;
        return r;
    });

    syscalls_.install(sysno::FORK, "fork", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = fork_current();
        return r;
    });

    syscalls_.install(sysno::EXIT, "exit", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        exit_current(static_cast<std::int32_t>(a.a0));
        r.value = 0;
        return r;
    });

    syscalls_.install(sysno::WAIT4, "wait4", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        std::int32_t status = 0, reaped = -1;
        WaitStatus w = procs_.wait(pid, static_cast<std::int32_t>(a.a0), &status, &reaped);
        switch (w) {
            case WaitStatus::Reaped: r.value = reaped; break;
            case WaitStatus::WouldBlock: r.value = -errno_::E_EAGAIN; break;
            case WaitStatus::NoChild: r.value = -10; break;  // ECHILD
            default: r.value = -errno_::E_EINVAL; break;
        }
        return r;
    });

    syscalls_.install(sysno::NICE, "nice", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        PCB* p = procs_.get(pid);
        if (!p) { r.value = -errno_::E_ESRCH; return r; }
        p->nice = static_cast<std::int8_t>(a.a0);
        procs_.set_priority(pid, p->priority + static_cast<int>(a.a0));
        r.value = p->priority;
        return r;
    });

    syscalls_.install(sysno::SCHED_GET_PRIORITY, "sched_get_priority",
                      [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const PCB* p = procs_.get(static_cast<std::int32_t>(a.a0));
        r.value = p ? p->priority : -errno_::E_ESRCH;
        return r;
    });

    syscalls_.install(sysno::SCHED_SET_PRIORITY, "sched_set_priority",
                      [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        r.value = procs_.set_priority(static_cast<std::int32_t>(a.a0),
                                      static_cast<int>(a.a1)) ? 0 : -errno_::E_ESRCH;
        return r;
    });

    syscalls_.install(sysno::KILL, "kill", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        r.value = sigs_.kill(static_cast<std::int32_t>(a.a0),
                             static_cast<int>(a.a1)) ? 0 : -errno_::E_ESRCH;
        return r;
    });

    syscalls_.install(sysno::SIGACTION, "sigaction", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        SignalAction act;
        act.handler = static_cast<std::int32_t>(a.a1);
        act.mask = a.a2;
        SignalAction old;
        r.value = sigs_.sigaction(current_pid(), static_cast<int>(a.a0), act, &old) ? 0
                                                                                    : -errno_::E_EINVAL;
        return r;
    });

    syscalls_.install(sysno::SIGPROCMASK, "sigprocmask",
                      [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::uint64_t old = 0;
        r.value = sigs_.sigprocmask(current_pid(), static_cast<int>(a.a0), a.a1, &old) ? 0
                                                                                       : -errno_::E_EINVAL;
        return r;
    });

    syscalls_.install(sysno::MKDIR, "mkdir", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const std::string* path = user_string(a.a0);
        if (!path) { r.value = -errno_::E_EFAULT; return r; }
        r.value = err(fs_.mkdir(*path, cwd(current_pid()), static_cast<std::uint32_t>(a.a1)));
        return r;
    });

    syscalls_.install(sysno::RMDIR, "rmdir", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const std::string* path = user_string(a.a0);
        if (!path) { r.value = -errno_::E_EFAULT; return r; }
        r.value = err(fs_.rmdir(*path, cwd(current_pid())));
        return r;
    });

    syscalls_.install(sysno::UNLINK, "unlink", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const std::string* path = user_string(a.a0);
        if (!path) { r.value = -errno_::E_EFAULT; return r; }
        r.value = err(fs_.unlink(*path, cwd(current_pid())));
        return r;
    });

    syscalls_.install(sysno::CHDIR, "chdir", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const std::string* path = user_string(a.a0);
        std::int32_t pid = current_pid();
        if (!path || pid < 0) { r.value = -errno_::E_EFAULT; return r; }
        std::uint32_t ino = 0;
        vfs::FsError e = fs_.resolve(*path, cwd(pid), ino);
        if (e != vfs::FsError::Ok) { r.value = err(e); return r; }
        const vfs::Inode* n = fs_.inode(ino);
        if (!n || !n->is_dir()) { r.value = -errno_::E_ENOTDIR; return r; }
        set_cwd(pid, ino);
        r.value = 0;
        return r;
    });

    syscalls_.install(sysno::GETCWD, "getcwd", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        std::string p = fs_.path_of(cwd(pid));
        user_strings_.push_back(p);
        r.value = static_cast<std::int64_t>(user_strings_.size());
        return r;
    });

    syscalls_.install(sysno::DUP, "dup", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        r.value = fds(pid).dup(static_cast<std::int32_t>(a.a0));
        return r;
    });

    syscalls_.install(sysno::DUP2, "dup2", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        std::int32_t pid = current_pid();
        if (pid < 0) { r.value = -errno_::E_ESRCH; return r; }
        r.value = fds(pid).dup2(static_cast<std::int32_t>(a.a0), static_cast<std::int32_t>(a.a1));
        return r;
    });

    syscalls_.install(sysno::PIPE, "pipe", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        std::int32_t wfd = -1;
        std::int32_t rfd = pipes_.create(4096, &wfd);
        r.value = rfd < 0 ? -errno_::E_ENOMEM : rfd;
        return r;
    });

    syscalls_.install(sysno::MMAP, "mmap", [](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        r.value = static_cast<std::int64_t>(0x40000000ull + (a.a1 & 0xFFFFF));
        return r;
    });

    syscalls_.install(sysno::MUNMAP, "munmap", [](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = 0;
        return r;
    });

    syscalls_.install(sysno::BRK, "brk", [](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        r.value = static_cast<std::int64_t>(a.a0);
        return r;
    });

    syscalls_.install(sysno::IOCTL, "ioctl", [](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = -errno_::E_EINVAL;
        return r;
    });

    syscalls_.install(sysno::SOCKET, "socket", [](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = -errno_::E_ENOSYS;
        return r;
    });

    syscalls_.install(sysno::EXECVE, "execve", [this](const SyscallArgs& a) -> SyscallResult {
        SyscallResult r;
        const std::string* path = user_string(a.a0);
        std::int32_t pid = current_pid();
        if (!path || pid < 0) { r.value = -errno_::E_EFAULT; return r; }
        PCB* p = procs_.get(pid);
        if (p) p->name = *path;
        r.value = 0;
        return r;
    });

    syscalls_.install(sysno::SETSID, "setsid", [this](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = current_pid();
        return r;
    });

    syscalls_.install(sysno::SIGRETURN, "sigreturn", [](const SyscallArgs&) -> SyscallResult {
        SyscallResult r;
        r.value = 0;
        return r;
    });
}

}  // namespace lowl
