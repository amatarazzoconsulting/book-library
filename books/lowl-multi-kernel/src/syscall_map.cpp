// MIT License - (c) Anthony Matarazzo 2026
#include "syscall_map.hpp"

#include <algorithm>
#include <cstdio>

namespace lowl {
namespace kal {

namespace {

const AbiLayout kLinuxAbi{"syscall", "rax", "rax", {"rdi", "rsi", "rdx", "r10"}, 6};
const AbiLayout kFreeBsdAbi{"syscall", "rax", "rax", {"rdi", "rsi", "rdx", "r10"}, 8};
const AbiLayout kWindowsAbi{"syscall stub", "eax", "eax", {"r10", "rdx", "r8", "r9"}, 12};
const AbiLayout kOs2Abi{"call gate", "eax", "eax", {"ebx", "ecx", "edx", "esi"}, 4};
const AbiLayout kUnknownAbi{"none", "-", "-", {"-", "-", "-", "-"}, 0};

}  // namespace

SyscallMap::SyscallMap() {
    // Linux x86_64.
    add(Op::Read, KernelId::Linux, 0, "read");
    add(Op::Write, KernelId::Linux, 1, "write");
    add(Op::Open, KernelId::Linux, 2, "open");
    add(Op::Close, KernelId::Linux, 3, "close");
    add(Op::Stat, KernelId::Linux, 4, "stat");
    add(Op::Mmap, KernelId::Linux, 9, "mmap");
    add(Op::ThreadCreate, KernelId::Linux, 56, "clone");
    add(Op::Fork, KernelId::Linux, 57, "fork");
    add(Op::SignalDeliver, KernelId::Linux, 62, "kill");
    add(Op::SemAcquire, KernelId::Linux, 202, "futex");
    add(Op::AsyncIo, KernelId::Linux, 425, "io_uring_setup");
    add(Op::TcpConnect, KernelId::Linux, 42, "connect");

    // FreeBSD 13 x86_64.
    add(Op::Fork, KernelId::FreeBsd, 2, "fork");
    add(Op::Read, KernelId::FreeBsd, 3, "read");
    add(Op::Write, KernelId::FreeBsd, 4, "write");
    add(Op::Open, KernelId::FreeBsd, 5, "open");
    add(Op::Close, KernelId::FreeBsd, 6, "close");
    add(Op::TcpConnect, KernelId::FreeBsd, 98, "connect");
    add(Op::SignalDeliver, KernelId::FreeBsd, 103, "sigaction");
    add(Op::Stat, KernelId::FreeBsd, 188, "stat");
    add(Op::Mmap, KernelId::FreeBsd, 197, "mmap");
    add(Op::AsyncIo, KernelId::FreeBsd, 363, "kevent", true);
    add(Op::ThreadCreate, KernelId::FreeBsd, 455, "thr_new");
    add(Op::SemAcquire, KernelId::FreeBsd, 468, "__semwait_signal");

    // Windows NT native API (Nt* service numbers).
    add(Op::SemAcquire, KernelId::WindowsNt, 4, "NtWaitForSingleObject");
    add(Op::Read, KernelId::WindowsNt, 6, "NtReadFile");
    add(Op::AsyncIo, KernelId::WindowsNt, 7, "NtDeviceIoControlFile");
    add(Op::TcpConnect, KernelId::WindowsNt, 7, "NtDeviceIoControlFile", true);
    add(Op::Write, KernelId::WindowsNt, 8, "NtWriteFile");
    add(Op::Close, KernelId::WindowsNt, 15, "NtClose");
    add(Op::Stat, KernelId::WindowsNt, 17, "NtQueryInformationFile");
    add(Op::Mmap, KernelId::WindowsNt, 24, "NtAllocateVirtualMemory");
    add(Op::SignalDeliver, KernelId::WindowsNt, 69, "NtQueueApcThread", true);
    add(Op::Open, KernelId::WindowsNt, 85, "NtCreateFile");
    add(Op::Fork, KernelId::WindowsNt, 165, "NtCreateProcess", true);
    add(Op::ThreadCreate, KernelId::WindowsNt, 194, "NtCreateThreadEx");

    // OS/2 Warp Dos* ordinals.
    add(Op::ThreadCreate, KernelId::Os2, 0x0A, "DosCreateThread");
    add(Op::SemAcquire, KernelId::Os2, 0x2A, "DosRequestMutexSem");
    add(Op::SignalDeliver, KernelId::Os2, 0x38, "DosKillProcess", true);
    add(Op::Stat, KernelId::Os2, 0x4E, "DosQueryFileInfo");
    add(Op::Mmap, KernelId::Os2, 0x51, "DosAllocMem");
    add(Op::Fork, KernelId::Os2, 0x5B, "DosExecPgm", true);
    add(Op::TcpConnect, KernelId::Os2, 0x60, "DosTcpConnect");
    add(Op::Read, KernelId::Os2, 0x72, "DosRead");
    add(Op::Write, KernelId::Os2, 0x73, "DosWrite");
    add(Op::Open, KernelId::Os2, 0x76, "DosOpen");
    add(Op::Close, KernelId::Os2, 0x77, "DosClose");
}

void SyscallMap::add(Op op, KernelId kernel, unsigned number, const char* native_name,
                     bool emulated) {
    SyscallEntry e;
    e.op = op;
    e.kernel = kernel;
    e.number = number;
    e.native_name = native_name;
    e.emulated = emulated;
    entries_.push_back(e);
}

const SyscallEntry* SyscallMap::find(Op op, KernelId kernel) const {
    for (const SyscallEntry& e : entries_) {
        if (e.op == op && e.kernel == kernel) return &e;
    }
    return nullptr;
}

const AbiLayout& SyscallMap::abi(KernelId kernel) const {
    switch (kernel) {
        case KernelId::Linux: return kLinuxAbi;
        case KernelId::FreeBsd: return kFreeBsdAbi;
        case KernelId::WindowsNt: return kWindowsAbi;
        case KernelId::Os2: return kOs2Abi;
        case KernelId::Unknown: break;
    }
    return kUnknownAbi;
}

Translation SyscallMap::translate(Op op, KernelId kernel) const {
    Translation t;
    const SyscallEntry* e = find(op, kernel);
    if (e == nullptr) return t;
    t.found = true;
    t.entry = *e;
    t.abi = &abi(kernel);
    return t;
}

std::vector<Op> SyscallMap::reverse(KernelId kernel, unsigned number) const {
    std::vector<Op> out;
    for (const SyscallEntry& e : entries_) {
        if (e.kernel != kernel || e.number != number) continue;
        if (std::find(out.begin(), out.end(), e.op) == out.end()) out.push_back(e.op);
    }
    return out;
}

Translation SyscallMap::remap(Op op, KernelId from, KernelId to) const {
    Translation src = translate(op, from);
    if (!src.found) return Translation{};
    Translation dst = translate(op, to);
    if (!dst.found) return Translation{};
    return dst;
}

std::size_t SyscallMap::count(KernelId kernel) const {
    std::size_t n = 0;
    for (const SyscallEntry& e : entries_) {
        if (e.kernel == kernel) ++n;
    }
    return n;
}

std::size_t SyscallMap::native_count(KernelId kernel) const {
    std::size_t n = 0;
    for (const SyscallEntry& e : entries_) {
        if (e.kernel == kernel && !e.emulated) ++n;
    }
    return n;
}

std::size_t SyscallMap::emulated_count(KernelId kernel) const {
    return count(kernel) - native_count(kernel);
}

std::vector<unsigned> SyscallMap::collisions(KernelId kernel) const {
    std::vector<unsigned> seen;
    std::vector<unsigned> dup;
    for (const SyscallEntry& e : entries_) {
        if (e.kernel != kernel) continue;
        if (std::find(seen.begin(), seen.end(), e.number) != seen.end()) {
            if (std::find(dup.begin(), dup.end(), e.number) == dup.end()) {
                dup.push_back(e.number);
            }
        } else {
            seen.push_back(e.number);
        }
    }
    std::sort(dup.begin(), dup.end());
    return dup;
}

std::string SyscallMap::table(KernelId kernel) const {
    char line[160];
    std::string out;
    std::snprintf(line, sizeof line, "%-16s %8s  %-24s %s\n", "operation", "number",
                  "native", "kind");
    out += line;
    for (const SyscallEntry& e : entries_) {
        if (e.kernel != kernel) continue;
        std::snprintf(line, sizeof line, "%-16s %8u  %-24s %s\n", op_name(e.op),
                      e.number, e.native_name, e.emulated ? "emulated" : "native");
        out += line;
    }
    return out;
}

}  // namespace kal
}  // namespace lowl
