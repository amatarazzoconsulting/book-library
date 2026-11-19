// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : signal delivery and pipe IPC.
// Mirrors posix/signal.lp (SIG_DFL / SIG_IGN / user handler, a per-signal
// action table, a blocked mask, a pending bitmask, and the default actions)
// and sys_pipe() from posix/syscall.lp (a bounded FIFO with a read end and a
// write end, blocking when full, EPIPE when the last writer closes).
#ifndef LOWL_SIG_HPP
#define LOWL_SIG_HPP

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "pcb.hpp"

namespace lowl {

// SA_* flags from posix/signal.lp.
namespace sa {
constexpr std::uint32_t NOCLDSTOP = 0x00000001;
constexpr std::uint32_t NOCLDWAIT = 0x00000002;
constexpr std::uint32_t SIGINFO = 0x00000004;
constexpr std::uint32_t ONSTACK = 0x00000008;
constexpr std::uint32_t RESTART = 0x00000010;
}  // namespace sa

// SIG_BLOCK / SIG_UNBLOCK / SIG_SETMASK for sigprocmask.
namespace sigmask_op {
constexpr int BLOCK = 0;
constexpr int UNBLOCK = 1;
constexpr int SETMASK = 2;
}  // namespace sigmask_op

struct SignalAction {
    std::int32_t handler = signo::SIG_DFL;  // SIG_DFL, SIG_IGN or handler id >= 0
    std::uint64_t mask = 0;
    std::uint32_t flags = 0;
};

// What happened when a signal was delivered.
enum class Delivery : std::uint8_t {
    Ignored,      // handler was SIG_IGN
    Blocked,      // signal was in the mask; it stays pending
    Handled,      // user handler ran
    Terminated,   // default action killed the process
    Stopped,      // default action stopped the process
    Continued,    // SIGCONT resumed a stopped process
    Discarded,    // default action is ignore (e.g. SIGCHLD)
    BadSignal
};

const char* to_string(Delivery d);

// Handler signature: signo in, user context pointer through.
using SignalHandlerFn = std::function<void(int)>;

// SignalManager implements delivery for a ProcessTable. Handler ids are
// small integers mapped to C++ callables, which stands in for the function
// pointer in the book.
class SignalManager {
public:
    explicit SignalManager(ProcessTable& table);

    // sigaction(). handler is SIG_DFL, SIG_IGN or an id registered with
    // register_handler(). old receives the previous action when non-null.
    bool sigaction(std::int32_t pid, int sig, const SignalAction& act, SignalAction* old);
    SignalAction action_of(std::int32_t pid, int sig) const;

    int register_handler(SignalHandlerFn fn);
    void clear_handlers() { handlers_.clear(); }
    std::size_t handler_count() const { return handlers_.size(); }

    // kill(2): mark the signal pending, then try to deliver it.
    bool kill(std::int32_t pid, int sig);
    // Deliver a pending signal now, honouring the block mask.
    Delivery deliver(std::int32_t pid, int sig);

    // sigprocmask(how, set, oldset).
    bool sigprocmask(std::int32_t pid, int how, std::uint64_t set, std::uint64_t* oldset);
    std::uint64_t pending(std::int32_t pid) const;
    bool is_pending(std::int32_t pid, int sig) const;

    // Deliver the lowest numbered pending, unblocked signal. Returns the
    // signal number or 0 if nothing was deliverable.
    int deliver_pending(std::int32_t pid);

    // After a handler returns, a blocked signal that arrived meanwhile must
    // be reconsidered. Call this from the run loop.
    int drain(std::int32_t pid);

    // Default action table (default_action() in the book).
    static Delivery default_action(std::int32_t pid, int sig, ProcState& state_out);

    std::uint64_t delivered_count() const { return delivered_; }
    std::uint64_t blocked_count() const { return blocked_; }

private:
    ProcessTable& table_;
    std::vector<SignalHandlerFn> handlers_;
    std::uint64_t delivered_ = 0;
    std::uint64_t blocked_ = 0;
};

// A pipe: bounded byte FIFO with one read end and one write end. The ends
// are reference counted so closing all writers wakes blocked readers with 0
// bytes (EOF) and closing all readers gives writers EPIPE.
class Pipe {
public:
    explicit Pipe(std::size_t capacity = 4096);

    // Returns bytes written, or a negative errno style code (-EPIPE, -EAGAIN).
    std::int64_t write(const char* data, std::size_t n);
    // Returns bytes read, 0 on EOF (no writers left), -EAGAIN when empty and
    // a writer is still open, -EPIPE when nothing can ever arrive.
    std::int64_t read(char* out, std::size_t n);

    void close_read_end();
    void close_write_end();
    int read_refs() const { return read_refs_; }
    int write_refs() const { return write_refs_; }

    std::size_t size() const { return buf_.size(); }
    std::size_t capacity() const { return cap_; }
    bool empty() const { return buf_.empty(); }
    bool full() const { return buf_.size() >= cap_; }
    bool eof() const { return write_refs_ == 0 && buf_.empty(); }
    void reset();

    std::uint64_t bytes_written() const { return written_; }
    std::uint64_t bytes_read() const { return read_; }
    std::uint64_t full_events() const { return full_events_; }
    std::uint64_t empty_events() const { return empty_events_; }

    static constexpr std::int64_t EAGAIN_CODE = -11;
    static constexpr std::int64_t EPIPE_CODE = -32;

private:
    std::size_t cap_;
    std::deque<char> buf_;
    int read_refs_ = 1;
    int write_refs_ = 1;
    std::uint64_t written_ = 0;
    std::uint64_t read_ = 0;
    std::uint64_t full_events_ = 0;
    std::uint64_t empty_events_ = 0;
};

// A pipe table that hands out fd pairs, standing in for the kernel's pipe
// registry. Two pipes can be crossed to make a two-way channel.
class PipeTable {
public:
    // Creates a pipe and returns the read fd; write_fd receives the write end.
    // Returns -1 when no fd can be allocated.
    std::int32_t create(std::size_t capacity, std::int32_t* write_fd);
    Pipe* get(std::int32_t fd);
    const Pipe* get(std::int32_t fd) const;
    bool is_write_end(std::int32_t fd) const;
    bool close(std::int32_t fd);
    std::size_t live_pipes() const;
    std::size_t fd_count() const { return ends_.size(); }
    void reset();

private:
    struct End { std::size_t index; bool write; };
    std::vector<Pipe> pipes_;
    std::vector<bool> alive_;  // per pipe: still referenced by any end
    std::map<std::int32_t, End> ends_;
    std::int32_t next_fd_ = 3;
};

}  // namespace lowl

#endif  // LOWL_SIG_HPP
