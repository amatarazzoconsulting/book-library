// MIT License - (c) Anthony Matarazzo 2026
//
// Syscall translation map: abstract LOWL operation -> per-kernel syscall
// number, native symbol and argument register layout. The map is static data
// plus a small query API, so lookups are deterministic and allocation free.
#ifndef LOWL_KAL_SYSCALL_MAP_HPP
#define LOWL_KAL_SYSCALL_MAP_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "kernel_adapter.hpp"

namespace lowl {
namespace kal {

// Argument passing convention of a kernel ABI.
struct AbiLayout {
    const char* syscall_insn;  // "syscall", "int 0x80", "sysenter", "call gate"
    const char* number_reg;    // register holding the syscall number
    const char* result_reg;    // register holding the result
    const char* arg_regs[4];   // argument registers in order
    unsigned max_args;
};

struct SyscallEntry {
    Op op{Op::Open};
    KernelId kernel{KernelId::Unknown};
    unsigned number{0};
    const char* native_name{""};
    bool emulated{false};
};

// Result of a translation attempt. `found` is false when the kernel has no
// entry for the operation, which the caller must treat as unsupported.
struct Translation {
    bool found{false};
    SyscallEntry entry{};
    const AbiLayout* abi{nullptr};
};

class SyscallMap {
public:
    SyscallMap();

    // Forward lookup. Returns found=false for unknown kernel or missing op.
    Translation translate(Op op, KernelId kernel) const;

    // Reverse lookup: which abstract operations land on this number. A kernel
    // can alias several LOWL ops onto one number, so this returns a vector.
    std::vector<Op> reverse(KernelId kernel, unsigned number) const;

    // Re-express an operation that was numbered for `from` under `to`.
    // found=false when either side lacks the operation.
    Translation remap(Op op, KernelId from, KernelId to) const;

    const AbiLayout& abi(KernelId kernel) const;
    const std::vector<SyscallEntry>& entries() const { return entries_; }

    std::size_t count(KernelId kernel) const;
    std::size_t native_count(KernelId kernel) const;
    std::size_t emulated_count(KernelId kernel) const;

    // Detect numbers that more than one distinct op shares on a kernel.
    std::vector<unsigned> collisions(KernelId kernel) const;

    // Fixed width text report, one row per abstract operation.
    std::string table(KernelId kernel) const;

private:
    void add(Op op, KernelId kernel, unsigned number, const char* native_name,
             bool emulated = false);
    const SyscallEntry* find(Op op, KernelId kernel) const;

    std::vector<SyscallEntry> entries_;
};

}  // namespace kal
}  // namespace lowl

#endif  // LOWL_KAL_SYSCALL_MAP_HPP
