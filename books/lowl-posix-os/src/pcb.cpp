// MIT License - (c) Anthony Matarazzo 2026
#include "pcb.hpp"

#include <algorithm>
#include <cstring>

namespace lowl {

const char* to_string(ProcState s) {
    switch (s) {
        case ProcState::New: return "NEW";
        case ProcState::Ready: return "READY";
        case ProcState::Running: return "RUNNING";
        case ProcState::Waiting: return "WAITING";
        case ProcState::Terminated: return "TERMINATED";
        case ProcState::Zombie: return "ZOMBIE";
        case ProcState::Stopped: return "STOPPED";
    }
    return "?";
}

const char* to_string(Policy p) {
    switch (p) {
        case Policy::Fifo: return "FIFO";
        case Policy::Rr: return "RR";
        case Policy::Cfs: return "CFS";
    }
    return "?";
}

bool parse_state(const std::string& text, ProcState& out) {
    static const ProcState all[] = {ProcState::New,     ProcState::Ready, ProcState::Running,
                                    ProcState::Waiting, ProcState::Terminated,
                                    ProcState::Zombie,  ProcState::Stopped};
    for (ProcState s : all) {
        if (text == to_string(s)) { out = s; return true; }
    }
    return false;
}

bool parse_policy(const std::string& text, Policy& out) {
    for (Policy p : {Policy::Fifo, Policy::Rr, Policy::Cfs}) {
        if (text == to_string(p)) { out = p; return true; }
    }
    return false;
}

namespace prio {
std::int8_t clamp(int value) {
    if (value < MIN) return MIN;
    if (value > MAX) return MAX;
    return static_cast<std::int8_t>(value);
}

const char* label(std::int8_t p) {
    if (p <= IDLE) return "IDLE";
    if (p < NORMAL) return "LOW";
    if (p < HIGH) return "NORMAL";
    if (p < REALTIME) return "HIGH";
    return "REALTIME";
}
}  // namespace prio

namespace signo {
const char* name(int sig) {
    switch (sig) {
        case HUP: return "SIGHUP";
        case INT: return "SIGINT";
        case QUIT: return "SIGQUIT";
        case ILL: return "SIGILL";
        case TRAP: return "SIGTRAP";
        case ABRT: return "SIGABRT";
        case BUS: return "SIGBUS";
        case FPE: return "SIGFPE";
        case KILL: return "SIGKILL";
        case USR1: return "SIGUSR1";
        case SEGV: return "SIGSEGV";
        case USR2: return "SIGUSR2";
        case PIPE: return "SIGPIPE";
        case ALRM: return "SIGALRM";
        case TERM: return "SIGTERM";
        case CHLD: return "SIGCHLD";
        case CONT: return "SIGCONT";
        case STOP: return "SIGSTOP";
        case TSTP: return "SIGTSTP";
        default: return "SIGUNKNOWN";
    }
}

std::uint64_t bit(int sig) {
    if (sig < 1 || sig > COUNT) return 0;
    return std::uint64_t(1) << (sig - 1);
}
}  // namespace signo

PCB::PCB() { sig_handlers.fill(signo::SIG_DFL); }

ProcessTable::ProcessTable() {
    // init is created by the caller (boot.lp creates it) so the table starts
    // empty; this keeps pid numbering explicit and testable.
}

std::int32_t ProcessTable::create(const std::string& name, std::int32_t ppid,
                                  std::int8_t priority, Policy policy, std::uint32_t uid) {
    auto pcb = std::make_unique<PCB>();
    std::int32_t pid = next_pid_++;
    pcb->pid = pid;
    pcb->tgid = pid;
    pcb->ppid = ppid;
    pcb->uid = uid;
    pcb->gid = uid;
    pcb->name = name;
    pcb->priority = prio::clamp(priority);
    pcb->policy = policy;
    pcb->state = ProcState::New;
    pcb->start_time = clock_;

    if (ppid > 0) {
        PCB* parent = get(ppid);
        if (parent) parent->children.push_back(pid);
    }
    procs_[pid] = std::move(pcb);
    return pid;
}

std::int32_t ProcessTable::fork(std::int32_t parent_pid) {
    PCB* parent = get(parent_pid);
    if (!parent) return -1;
    std::int32_t child = create(parent->name, parent_pid, parent->priority, parent->policy,
                                parent->uid);
    PCB* c = get(child);
    if (c) {
        c->flags = parent->flags | pflags::FORKED;
        c->nice = parent->nice;
        c->regs = parent->regs;          // registers are copied
        c->sig_handlers = parent->sig_handlers;
        c->sigmask = parent->sigmask;
        c->state = ProcState::Ready;
        c->burst = parent->burst;
    }
    return child;
}

PCB* ProcessTable::get(std::int32_t pid) {
    auto it = procs_.find(pid);
    return it == procs_.end() ? nullptr : it->second.get();
}

const PCB* ProcessTable::get(std::int32_t pid) const {
    auto it = procs_.find(pid);
    return it == procs_.end() ? nullptr : it->second.get();
}

bool ProcessTable::has(std::int32_t pid) const { return procs_.count(pid) != 0; }

bool ProcessTable::exit(std::int32_t pid, std::int32_t code, std::uint8_t sig) {
    PCB* p = get(pid);
    if (!p) return false;
    if (p->state == ProcState::Zombie || p->state == ProcState::Terminated) return false;

    p->state = ProcState::Zombie;
    p->exit_code = code;
    p->exit_signal = sig;
    p->flags |= pflags::EXITING;

    // Orphans are reparented to init (pid 1), as the kernel does.
    for (std::int32_t child : p->children) {
        PCB* c = get(child);
        if (!c) continue;
        c->ppid = INIT_PID;
        PCB* init = get(INIT_PID);
        if (init) init->children.push_back(child);
    }

    PCB* parent = get(p->ppid);
    if (parent) {
        parent->pending_signals |= signo::bit(signo::CHLD);
        parent->flags |= pflags::SIGNALED;
        // Wake a parent blocked in wait() if this was the child it wanted.
        if (parent->state == ProcState::Waiting &&
            (parent->wait_target == pid || parent->wait_target == -1)) {
            parent->state = ProcState::Ready;
        }
    }
    return true;
}

WaitStatus ProcessTable::wait(std::int32_t parent_pid, std::int32_t target,
                              std::int32_t* status, std::int32_t* reaped_pid) {
    PCB* parent = get(parent_pid);
    if (!parent) return WaitStatus::NoChild;

    // -1 means "any child"; target > 0 means that specific pid.
    std::int32_t found = -1;
    for (std::int32_t child : parent->children) {
        PCB* c = get(child);
        if (!c) continue;
        if (target > 0 && child != target) continue;
        if (c->state == ProcState::Zombie) { found = child; break; }
    }

    if (found < 0) {
        bool any_live = false;
        for (std::int32_t child : parent->children) {
            const PCB* c = get(child);
            if (c && c->state != ProcState::Zombie && c->state != ProcState::Terminated) {
                any_live = true;
                break;
            }
        }
        if (!any_live && target > 0 && !has(target)) return WaitStatus::NoChild;
        if (!any_live) return WaitStatus::NoChild;
        // A live child exists but has not exited: block.
        parent->state = ProcState::Waiting;
        parent->wait_target = target;
        return WaitStatus::WouldBlock;
    }

    PCB* z = get(found);
    std::int32_t code = z->exit_code;
    if (status) *status = code;
    if (reaped_pid) *reaped_pid = found;
    parent->state = ProcState::Running;
    parent->wait_target = -1;
    reap(found);
    return WaitStatus::Reaped;
}

bool ProcessTable::reap(std::int32_t pid) {
    PCB* p = get(pid);
    if (!p) return false;
    std::int32_t ppid = p->ppid;
    PCB* parent = get(ppid);
    if (parent) {
        auto& ch = parent->children;
        ch.erase(std::remove(ch.begin(), ch.end(), pid), ch.end());
    }
    procs_.erase(pid);
    return true;
}

bool ProcessTable::set_state(std::int32_t pid, ProcState st) {
    PCB* p = get(pid);
    if (!p) return false;
    p->state = st;
    return true;
}

bool ProcessTable::set_priority(std::int32_t pid, int priority) {
    PCB* p = get(pid);
    if (!p) return false;
    p->priority = prio::clamp(priority);
    return true;
}

bool ProcessTable::set_policy(std::int32_t pid, Policy pol) {
    PCB* p = get(pid);
    if (!p) return false;
    p->policy = pol;
    return true;
}

std::size_t ProcessTable::count(ProcState st) const {
    std::size_t n = 0;
    for (const auto& kv : procs_) {
        if (kv.second->state == st) ++n;
    }
    return n;
}

std::size_t ProcessTable::live_count() const {
    std::size_t n = 0;
    for (const auto& kv : procs_) {
        if (kv.second->state != ProcState::Zombie && kv.second->state != ProcState::Terminated)
            ++n;
    }
    return n;
}

std::size_t ProcessTable::zombie_count() const { return count(ProcState::Zombie); }

std::vector<std::int32_t> ProcessTable::pids() const {
    std::vector<std::int32_t> out;
    out.reserve(procs_.size());
    for (const auto& kv : procs_) out.push_back(kv.first);
    return out;
}

std::vector<PCB*> ProcessTable::all() {
    std::vector<PCB*> out;
    out.reserve(procs_.size());
    for (auto& kv : procs_) out.push_back(kv.second.get());
    return out;
}

std::vector<const PCB*> ProcessTable::all() const {
    std::vector<const PCB*> out;
    out.reserve(procs_.size());
    for (const auto& kv : procs_) out.push_back(kv.second.get());
    return out;
}

std::vector<std::int32_t> ProcessTable::children_of(std::int32_t pid) const {
    const PCB* p = get(pid);
    return p ? p->children : std::vector<std::int32_t>{};
}

}  // namespace lowl
