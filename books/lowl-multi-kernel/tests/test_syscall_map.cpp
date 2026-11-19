// MIT License - (c) Anthony Matarazzo 2026
// Tests for the syscall translation map.
#include <cstdio>
#include <string>
#include <vector>

#include "syscall_map.hpp"

using namespace lowl::kal;

static int failures = 0;

static void check(bool ok, const char* what) {
    if (ok) {
        std::printf("PASS %s\n", what);
    } else {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

int main() {
    SyscallMap map;

    // Normal: known numbers on each kernel.
    Translation t = map.translate(Op::Read, KernelId::Linux);
    check(t.found && t.entry.number == 0 && std::string(t.entry.native_name) == "read",
          "linux read is syscall 0");
    t = map.translate(Op::Fork, KernelId::FreeBsd);
    check(t.found && t.entry.number == 2, "freebsd fork is syscall 2");
    t = map.translate(Op::Open, KernelId::WindowsNt);
    check(t.found && t.entry.number == 85, "windows open is NtCreateFile 85");
    check(t.found && !t.entry.emulated, "NtCreateFile is native");
    t = map.translate(Op::Open, KernelId::Os2);
    check(t.found && t.entry.number == 0x76, "os2 open is DosOpen 0x76");

    // Normal: the ABI layout comes back with the translation.
    t = map.translate(Op::Read, KernelId::Linux);
    check(t.abi != nullptr && std::string(t.abi->number_reg) == "rax",
          "linux number register is rax");
    check(std::string(map.abi(KernelId::WindowsNt).number_reg) == "eax",
          "windows number register is eax");
    check(std::string(map.abi(KernelId::Os2).syscall_insn) == "call gate",
          "os2 uses a call gate");

    // Normal: emulation flags match the book.
    check(map.translate(Op::AsyncIo, KernelId::FreeBsd).entry.emulated,
          "freebsd async_io is emulated on kevent");
    check(map.translate(Op::Fork, KernelId::WindowsNt).entry.emulated,
          "windows fork is emulated on NtCreateProcess");
    check(map.translate(Op::SignalDeliver, KernelId::WindowsNt).entry.emulated,
          "windows signal is emulated with APCs");

    // Normal: reverse lookup.
    std::vector<Op> ops = map.reverse(KernelId::WindowsNt, 7);
    check(ops.size() == 2, "windows number 7 maps to two abstract ops");
    ops = map.reverse(KernelId::Linux, 0);
    check(ops.size() == 1 && ops[0] == Op::Read, "linux number 0 maps only to read");
    check(map.reverse(KernelId::Linux, 9999).empty(), "unknown number reverses to nothing");

    // Normal: collision detection finds the aliased number.
    std::vector<unsigned> col = map.collisions(KernelId::WindowsNt);
    check(col.size() == 1 && col[0] == 7, "windows has exactly one collision at 7");
    check(map.collisions(KernelId::Linux).empty(), "linux table has no collisions");

    // Normal: counts.
    check(map.count(KernelId::Linux) == 12, "linux has 12 mapped operations");
    check(map.native_count(KernelId::Linux) == 12, "all linux entries are native");
    check(map.emulated_count(KernelId::FreeBsd) == 1, "freebsd has one emulated entry");
    check(map.emulated_count(KernelId::Os2) == 2, "os2 has two emulated entries");

    // Normal: remap between kernels.
    Translation r = map.remap(Op::Fork, KernelId::Linux, KernelId::FreeBsd);
    check(r.found && r.entry.number == 2, "remap fork linux->freebsd gives 2");
    check(map.remap(Op::Fork, KernelId::Linux, KernelId::Linux).entry.number == 57,
          "remap to the same kernel is identity");

    // Edge: operation absent on the target kernel.
    r = map.remap(Op::AsyncIo, KernelId::Linux, KernelId::Os2);
    check(!r.found, "remap async_io to os2 reports not found");
    check(!map.translate(Op::AsyncIo, KernelId::Os2).found,
          "os2 has no async_io entry at all");
    check(map.translate(Op::ThreadCreate, KernelId::Os2).found,
          "os2 does have thread_create");

    // Edge: unknown kernel everywhere.
    check(!map.translate(Op::Read, KernelId::Unknown).found, "unknown kernel translates nothing");
    check(map.count(KernelId::Unknown) == 0, "unknown kernel has zero entries");
    check(std::string(map.abi(KernelId::Unknown).number_reg) == "-",
          "unknown kernel abi is a dash placeholder");
    check(!map.remap(Op::Read, KernelId::Unknown, KernelId::Linux).found,
          "remap from unknown kernel fails");
    check(!map.remap(Op::Read, KernelId::Linux, KernelId::Unknown).found,
          "remap to unknown kernel fails");

    // Edge: table text is non-empty and lists a known row.
    std::string table = map.table(KernelId::Linux);
    check(table.find("operation") != std::string::npos, "table has a header");
    check(table.find("io_uring_setup") != std::string::npos, "table lists io_uring_setup");
    check(map.table(KernelId::Unknown).find("operation") != std::string::npos,
          "empty table still prints the header");

    std::printf(failures == 0 ? "all syscall_map tests passed\n"
                              : "syscall_map tests FAILED\n");
    return failures == 0 ? 0 : 1;
}
