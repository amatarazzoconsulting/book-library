# Writing lowl for this library

**By Anthony Matarazzo (c) 2026** - MIT License

Every `.lowl` file in this library must compile with the author's compiler and run.
This page is the contract: the toolchain, the verified syntax, and the constructs that
are broken in v2.1.0 and must be avoided.

## Toolchain

The compiler is vendored at `library/toolchain/lowl/` (cloned from
https://github.com/amatarazzoconsulting/lowl, MIT License, v2.1.0).

```sh
cd library/toolchain/lowl
./build.sh hello.lowl --run          # compile, assemble, link, run
./build.sh ../path/to/file.lowl      # build only
```

`build.sh` runs `lowlc.py` to emit NASM assembly, assembles it with `nasm -f elf64`,
and links it with `runtime.o`, which supplies `_start` and the print helpers the
compiler calls but does not define.

Outputs land in `build/` next to the sources (`OUTDIR=...` overrides).

### What the runtime provides

| Symbol | Signature | Purpose |
| --- | --- | --- |
| `print_string` / `print` | `rdi = char*` | Write a zero terminated string |
| `print_char` / `putc` | `dil = char` | Write one byte |
| `print_u64` | `rdi = u64` | Decimal unsigned |
| `print_i64` | `rdi = i64` | Decimal signed |
| `print_hex_u64` | `rdi = u64` | `0x` prefixed hex |
| `exit` | `rdi = code` | Terminate |

The compiler emits SysV AMD64: arguments in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`,
return value in `rax`.

## Verified syntax

Everything below compiles and runs today. `conformance.lowl` in the toolchain
directory is the executable version of this list.

```lowl
// comments use two slashes
struct Point:
    x: u64
    y: u64

fn add(a: u64, b: u64) -> u64:
    return a + b

fn main() -> u32:
    let mut total: u64 = 0
    let mut i: u64 = 0
    while i < 5:
        total = total + i
        i = i + 1

    if total == 10 and i == 5:
        print_string("ok ")
    elif total > 10:
        print_string("high ")
    else:
        print_string("low ")

    print_u64(add(total, 32))
    print_string("\n")

    let p: Point = Point { x: 3, y: 4 }
    print_u64(p.x * p.x + p.y * p.y)
    print_string("\n")
    return 0
```

Verified and safe to use:

* `fn name(a: u64, b: u64) -> u64:` with `return`
* `let` and `let mut` with explicit types, integer inference
* `u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 bool char`
* arithmetic `+ - * /`, comparisons `== != < > <= >=`, `and`, `or`, `not`
* `if` / `elif` / `else`, `while`
* `struct` declaration, struct construction `Point { x: 3, y: 4 }`, field reads
* nested function calls, recursion
* `print_string`, `print_u64`, `print_i64`, `print_char`, `print_hex_u64`

## Broken in v2.1.0 - do not use

These were tested against the vendored compiler and fail. Do not put them in book
examples.

| Construct | Symptom | Use instead |
| --- | --- | --- |
| `for i in 0..n:` | Emits `mov rax, qword [rel i]`, nasm error `symbol 'i' not defined` | `while` with an explicit counter |
| `array<T, N>` indexing, constant or variable | Compiles, then segfaults at run time | Scalars and struct fields; keep array based algorithms in the C++ sources |
| Assigning to a struct field (`q.a = ...`) | Silently yields zero | Build a new struct literal, or keep the running value in a scalar |
| `name == "text"` string equality | Emits `qword [rel ch]`, nasm error `invalid 64-bit effective address` | Compare lengths or characters explicitly |
| `else if` on one line | Parse error | `elif` |

If a book algorithm needs a data structure the compiler cannot express, implement the
runnable version in the book's C++ sources, and keep the `.lowl` file to the part the
compiler can actually execute. Say so plainly in the book README.

## Required layout for a lowl book

```
books/<slug>/
  src/<algo>.lowl        one file per algorithm, compiles and runs
  src/lowltests/*.lowl   optional focused probes
  README.md              states which algorithms have a lowl implementation
  learn/                 lessons quote the .lowl source and its real output
```

Each `.lowl` file starts with the license line:

```lowl
// MIT License - (c) Anthony Matarazzo 2026
```

Every claim about output must come from an actual `./build.sh ... --run`.
