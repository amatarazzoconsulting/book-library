// MIT License - (c) Anthony Matarazzo 2026
#include "sig.hpp"

#include <algorithm>
#include <cstring>

namespace lowl {

const char* to_string(Delivery d) {
    switch (d) {
        case Delivery::Ignored: return "IGNORED";
        case Delivery::Blocked: return "BLOCKED";
        case Delivery::Handled: return "HANDLED";
        case Delivery::Terminated: return "TERMINATED";
        case Delivery::Stopped: return "STOPPED";
        case Delivery::Continued: return "CONTINUED";
        case Delivery::Discarded: return "DISCARDED";
        case Delivery::BadSignal: return "EBADF";
    }
    return "?";
}

SignalManager::SignalManager(ProcessTable& table) : table_(table) {}

int SignalManager::register_handler(SignalHandlerFn fn) {
    handlers_.push_back(std::move(fn));
    return static_cast<int>(handlers_.size()) - 1;
}

bool SignalManager::sigaction(std::int32_t pid, int sig, const SignalAction& act,
                              SignalAction* old) {
    PCB* p = table_.get(pid);
    if (!p) return false;
    if (sig < 1 || sig > signo::COUNT) return false;
    // SIGKILL and SIGSTOP cannot be caught or ignored, as in POSIX.
    if ((sig == signo::KILL || sig == signo::STOP) && act.handler >= 0) return false;

    if (old) old->handler = p->sig_handlers[static_cast<std::size_t>(sig)];
    p->sig_handlers[static_cast<std::size_t>(sig)] = act.handler;
    return true;
}

SignalAction SignalManager::action_of(std::int32_t pid, int sig) const {
    SignalAction a;
    const PCB* p = table_.get(pid);
    if (!p || sig < 1 || sig > signo::COUNT) { a.handler = signo::SIG_DFL; return a; }
    a.handler = p->sig_handlers[static_cast<std::size_t>(sig)];
    return a;
}

bool SignalManager::kill(std::int32_t pid, int sig) {
    PCB* p = table_.get(pid);
    if (!p) return false;
    if (sig < 1 || sig > signo::COUNT) return false;
    // A zombie cannot receive signals any more.
    if (p->state == ProcState::Zombie || p->state == ProcState::Terminated) return false;
    p->pending_signals |= signo::bit(sig);
    p->flags |= pflags::SIGNALED;
    return true;
}

Delivery SignalManager::deliver(std::int32_t pid, int sig) {
    PCB* p = table_.get(pid);
    if (!p) return Delivery::BadSignal;
    if (sig < 1 || sig > signo::COUNT) return Delivery::BadSignal;

    // SIGKILL and SIGSTOP bypass the mask and any handler.
    if (sig == signo::KILL) {
        p->pending_signals &= ~signo::bit(sig);
        table_.exit(pid, 128 + sig, static_cast<std::uint8_t>(sig));
        ++delivered_;
        return Delivery::Terminated;
    }
    if (sig == signo::STOP) {
        p->pending_signals &= ~signo::bit(sig);
        p->state = ProcState::Stopped;
        ++delivered_;
        return Delivery::Stopped;
    }

    if (p->sigmask & signo::bit(sig)) {
        ++blocked_;
        return Delivery::Blocked;  // stays pending
    }

    std::int32_t h = p->sig_handlers[static_cast<std::size_t>(sig)];
    p->pending_signals &= ~signo::bit(sig);

    if (h == signo::SIG_IGN) {
        ++delivered_;
        return Delivery::Ignored;
    }
    if (h >= 0) {
        if (static_cast<std::size_t>(h) < handlers_.size() && handlers_[static_cast<std::size_t>(h)]) {
            handlers_[static_cast<std::size_t>(h)](sig);
        }
        ++delivered_;
        return Delivery::Handled;
    }

    // SIG_DFL
    ProcState st = p->state;
    Delivery d = default_action(pid, sig, st);
    p->state = st;
    if (d == Delivery::Terminated) {
        table_.exit(pid, 128 + sig, static_cast<std::uint8_t>(sig));
    }
    ++delivered_;
    return d;
}

Delivery SignalManager::default_action(std::int32_t pid, int sig, ProcState& state_out) {
    (void)pid;
    switch (sig) {
        case signo::INT:
        case signo::QUIT:
        case signo::ILL:
        case signo::TRAP:
        case signo::ABRT:
        case signo::BUS:
        case signo::FPE:
        case signo::SEGV:
        case signo::TERM:
        case signo::PIPE:
        case signo::ALRM:
        case signo::HUP:
            state_out = ProcState::Zombie;
            return Delivery::Terminated;
        case signo::CHLD:
            return Delivery::Discarded;  // default is ignore
        case signo::STOP:
        case signo::TSTP:
            state_out = ProcState::Stopped;
            return Delivery::Stopped;
        case signo::CONT:
            if (state_out == ProcState::Stopped) state_out = ProcState::Ready;
            return Delivery::Continued;
        case signo::USR1:
        case signo::USR2:
            state_out = ProcState::Zombie;
            return Delivery::Terminated;
        default:
            return Delivery::Discarded;
    }
}

bool SignalManager::sigprocmask(std::int32_t pid, int how, std::uint64_t set,
                                std::uint64_t* oldset) {
    PCB* p = table_.get(pid);
    if (!p) return false;
    if (oldset) *oldset = p->sigmask;
    switch (how) {
        case sigmask_op::BLOCK: p->sigmask |= set; break;
        case sigmask_op::UNBLOCK: p->sigmask &= ~set; break;
        case sigmask_op::SETMASK: p->sigmask = set; break;
        default: return false;
    }
    // SIGKILL and SIGSTOP are never blockable.
    p->sigmask &= ~(signo::bit(signo::KILL) | signo::bit(signo::STOP));
    return true;
}

std::uint64_t SignalManager::pending(std::int32_t pid) const {
    const PCB* p = table_.get(pid);
    return p ? p->pending_signals : 0;
}

bool SignalManager::is_pending(std::int32_t pid, int sig) const {
    return (pending(pid) & signo::bit(sig)) != 0;
}

int SignalManager::deliver_pending(std::int32_t pid) {
    PCB* p = table_.get(pid);
    if (!p) return 0;
    for (int sig = 1; sig <= signo::COUNT; ++sig) {
        if (!(p->pending_signals & signo::bit(sig))) continue;
        if (p->sigmask & signo::bit(sig)) continue;
        Delivery d = deliver(pid, sig);
        if (d != Delivery::Blocked) return sig;
    }
    return 0;
}

int SignalManager::drain(std::int32_t pid) {
    int n = 0;
    while (deliver_pending(pid) != 0) ++n;
    return n;
}

// ------------------------------------------------------------------- Pipe

Pipe::Pipe(std::size_t capacity) : cap_(capacity ? capacity : 1) {}

void Pipe::reset() {
    buf_.clear();
    read_refs_ = 1;
    write_refs_ = 1;
    written_ = read_ = full_events_ = empty_events_ = 0;
}

std::int64_t Pipe::write(const char* data, std::size_t n) {
    if (write_refs_ <= 0) return EPIPE_CODE;
    if (read_refs_ <= 0) return EPIPE_CODE;  // nobody left to read
    if (n == 0) return 0;
    if (buf_.size() >= cap_) { ++full_events_; return EAGAIN_CODE; }

    std::size_t space = cap_ - buf_.size();
    std::size_t put = std::min(n, space);
    for (std::size_t i = 0; i < put; ++i) buf_.push_back(data[i]);
    written_ += put;
    if (put < n) ++full_events_;  // partial write, pipe is now full
    return static_cast<std::int64_t>(put);
}

std::int64_t Pipe::read(char* out, std::size_t n) {
    if (read_refs_ <= 0) return EPIPE_CODE;
    if (n == 0) return 0;
    if (buf_.empty()) {
        if (write_refs_ <= 0) return 0;  // EOF
        ++empty_events_;
        return EAGAIN_CODE;
    }
    std::size_t take = std::min(n, buf_.size());
    for (std::size_t i = 0; i < take; ++i) {
        out[i] = buf_.front();
        buf_.pop_front();
    }
    read_ += take;
    return static_cast<std::int64_t>(take);
}

void Pipe::close_read_end() {
    if (read_refs_ > 0) --read_refs_;
}

void Pipe::close_write_end() {
    if (write_refs_ > 0) --write_refs_;
}

// -------------------------------------------------------------- PipeTable

void PipeTable::reset() {
    pipes_.clear();
    alive_.clear();
    ends_.clear();
    next_fd_ = 3;
}

std::int32_t PipeTable::create(std::size_t capacity, std::int32_t* write_fd) {
    if (!write_fd) return -1;
    pipes_.emplace_back(capacity);
    alive_.push_back(true);
    std::size_t idx = pipes_.size() - 1;
    std::int32_t rfd = next_fd_++;
    std::int32_t wfd = next_fd_++;
    ends_[rfd] = End{idx, false};
    ends_[wfd] = End{idx, true};
    *write_fd = wfd;
    return rfd;
}

Pipe* PipeTable::get(std::int32_t fd) {
    auto it = ends_.find(fd);
    if (it == ends_.end()) return nullptr;
    return &pipes_[it->second.index];
}

const Pipe* PipeTable::get(std::int32_t fd) const {
    auto it = ends_.find(fd);
    if (it == ends_.end()) return nullptr;
    return &pipes_[it->second.index];
}

bool PipeTable::is_write_end(std::int32_t fd) const {
    auto it = ends_.find(fd);
    return it != ends_.end() && it->second.write;
}

bool PipeTable::close(std::int32_t fd) {
    auto it = ends_.find(fd);
    if (it == ends_.end()) return false;
    Pipe& p = pipes_[it->second.index];
    if (it->second.write) p.close_write_end(); else p.close_read_end();
    ends_.erase(it);
    return true;
}

std::size_t PipeTable::live_pipes() const {
    std::size_t n = 0;
    for (std::size_t i = 0; i < pipes_.size(); ++i) {
        bool any = false;
        for (const auto& kv : ends_) if (kv.second.index == i) { any = true; break; }
        if (any) ++n;
    }
    return n;
}

}  // namespace lowl
