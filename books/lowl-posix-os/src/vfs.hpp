// MIT License - (c) Anthony Matarazzo 2026
//
// lowl-posix-os : inode based filesystem and per-process file descriptor table.
// Mirrors posix/filesystem.lp: superblock, inode table, directory entries made
// of (name, ino) pairs, 4096 byte blocks, and a 256 entry fd table per process.
#ifndef LOWL_VFS_HPP
#define LOWL_VFS_HPP

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace lowl {
namespace vfs {

// File type bits (S_IFMT style) and permission bits from the book.
constexpr std::uint32_t S_IFREG = 0x8000;
constexpr std::uint32_t S_IFDIR = 0x4000;
constexpr std::uint32_t S_IFCHR = 0x2000;
constexpr std::uint32_t S_IFBLK = 0x6000;
constexpr std::uint32_t S_IFIFO = 0x1000;
constexpr std::uint32_t S_IFLNK = 0xA000;
constexpr std::uint32_t S_IFSOCK = 0xC000;
constexpr std::uint32_t S_IFMT = 0xF000;

constexpr std::uint32_t S_IRUSR = 0x0100;
constexpr std::uint32_t S_IWUSR = 0x0080;
constexpr std::uint32_t S_IXUSR = 0x0040;
constexpr std::uint32_t S_IRGRP = 0x0020;
constexpr std::uint32_t S_IWGRP = 0x0010;
constexpr std::uint32_t S_IXGRP = 0x0008;
constexpr std::uint32_t S_IROTH = 0x0004;
constexpr std::uint32_t S_IWOTH = 0x0002;
constexpr std::uint32_t S_IXOTH = 0x0001;

// open(2) flags used by the simulator.
constexpr std::uint32_t O_RDONLY = 0x00;
constexpr std::uint32_t O_WRONLY = 0x01;
constexpr std::uint32_t O_RDWR = 0x02;
constexpr std::uint32_t O_CREAT = 0x40;
constexpr std::uint32_t O_TRUNC = 0x200;
constexpr std::uint32_t O_APPEND = 0x400;

// lseek(2) whence values.
constexpr int WHENCE_SET = 0;
constexpr int WHENCE_CUR = 1;
constexpr int WHENCE_END = 2;

constexpr std::uint32_t BLOCK_SIZE = 4096;
constexpr std::uint32_t MAX_INODES = 1024;
constexpr std::int32_t MAX_FDS = 256;
constexpr std::uint32_t ROOT_INO = 1;

struct Inode {
    std::uint32_t ino = 0;
    std::uint32_t mode = 0;   // S_IF* | permission bits
    std::uint32_t nlink = 0;
    std::uint32_t uid = 0;
    std::uint32_t gid = 0;
    std::uint64_t size = 0;
    std::uint64_t atime = 0;
    std::uint64_t mtime = 0;
    std::uint64_t ctime = 0;
    std::vector<std::string> blocks;                 // sparse: empty string = hole
    std::map<std::string, std::uint32_t> entries;    // directories only

    bool is_dir() const { return (mode & S_IFMT) == S_IFDIR; }
    bool is_reg() const { return (mode & S_IFMT) == S_IFREG; }
    std::uint32_t perm() const { return mode & 0x0FFF; }
    std::uint32_t block_count() const;
};

struct Stat {
    std::uint32_t st_ino = 0;
    std::uint32_t st_mode = 0;
    std::uint32_t st_nlink = 0;
    std::uint32_t st_uid = 0;
    std::uint32_t st_gid = 0;
    std::uint64_t st_size = 0;
    std::uint64_t st_atime = 0;
    std::uint64_t st_mtime = 0;
    std::uint64_t st_ctime = 0;
    std::uint32_t st_blocks = 0;
};

struct Superblock {
    std::uint32_t magic = 0x4E4653;  // "NFS"
    std::uint32_t blocks = 0;
    std::uint32_t inodes = 0;
    std::uint32_t free_blocks = 0;
    std::uint32_t free_inodes = 0;
    std::uint32_t block_size = BLOCK_SIZE;
};

// Result of a filesystem operation: negative errno-style value or >= 0.
enum class FsError : std::int32_t {
    Ok = 0,
    NoEnt = 2,
    BadF = 9,
    NoMem = 12,
    Exist = 17,
    NotDir = 20,
    IsDir = 21,
    Inval = 22,
    NoSpc = 28,
    Perm = 13
};

const char* strerror(FsError e);

class FileSystem {
public:
    FileSystem();

    void format(std::uint32_t inode_capacity = MAX_INODES);
    void set_clock(std::uint64_t t) { clock_ = t; }
    void tick(std::uint64_t n = 1) { clock_ += n; }

    const Superblock& superblock() const { return sb_; }
    Inode* inode(std::uint32_t ino);
    const Inode* inode(std::uint32_t ino) const;
    std::uint32_t root_ino() const { return ROOT_INO; }

    // Path resolution. Absolute paths start at root; relative paths start at
    // `cwd`. ".." above root stays at root. Returns 0 and sets out on success.
    FsError resolve(const std::string& path, std::uint32_t cwd, std::uint32_t& out) const;
    std::string path_of(std::uint32_t ino) const;

    FsError mkdir(const std::string& path, std::uint32_t cwd, std::uint32_t mode,
                  std::uint32_t uid = 0);
    FsError create(const std::string& path, std::uint32_t cwd, std::uint32_t mode,
                   std::uint32_t uid = 0, std::uint32_t* out_ino = nullptr);
    FsError unlink(const std::string& path, std::uint32_t cwd);
    FsError rmdir(const std::string& path, std::uint32_t cwd);
    FsError stat(const std::string& path, std::uint32_t cwd, Stat& out) const;

    // Raw data access by inode, used by the fd layer.
    std::int64_t read(std::uint32_t ino, std::uint64_t offset, char* buf, std::size_t count);
    std::int64_t write(std::uint32_t ino, std::uint64_t offset, const char* buf,
                       std::size_t count);
    FsError truncate(std::uint32_t ino, std::uint64_t size);

    std::size_t inode_count() const { return inodes_.size(); }
    std::size_t free_inode_count() const;
    std::size_t used_blocks() const;
    std::vector<std::uint32_t> list(std::uint32_t dir_ino) const;

private:
    std::uint32_t alloc_inode(std::uint32_t mode, std::uint32_t uid);
    void free_inode(std::uint32_t ino);
    Inode* lookup_child(Inode& dir, const std::string& name);
    FsError link_child(Inode& dir, const std::string& name, std::uint32_t ino);
    bool dir_is_empty(const Inode& dir) const;
    FsError split_parent(const std::string& path, std::uint32_t cwd, std::uint32_t& parent,
                         std::string& name) const;

    std::map<std::uint32_t, Inode> inodes_;
    std::uint32_t next_ino_ = ROOT_INO + 1;
    Superblock sb_;
    std::uint64_t clock_ = 0;
};

// Per-process file descriptor table (256 entries, as in the PCB).
struct FileDescriptor {
    std::int32_t fd = -1;
    std::uint32_t ino = 0;
    std::uint64_t offset = 0;
    std::uint32_t flags = 0;
    std::uint32_t refcount = 1;
    bool open = false;
};

class FdTable {
public:
    FdTable() = default;
    void reset();

    // 0, 1, 2 are the console. They are pre-installed by reset().
    std::int32_t open(FileSystem& fs, const std::string& path, std::uint32_t flags,
                      std::uint32_t mode, std::uint32_t cwd);
    std::int32_t close(FileSystem& fs, std::int32_t fd);
    std::int64_t read(FileSystem& fs, std::int32_t fd, char* buf, std::size_t count);
    std::int64_t write(FileSystem& fs, std::int32_t fd, const char* buf, std::size_t count);
    std::int64_t lseek(std::int32_t fd, std::int64_t offset, int whence);
    std::int32_t dup(std::int32_t fd);
    std::int32_t dup2(std::int32_t oldfd, std::int32_t newfd);
    FsError fstat(const FileSystem& fs, std::int32_t fd, Stat& out) const;

    bool valid(std::int32_t fd) const;
    const FileDescriptor* at(std::int32_t fd) const;
    std::size_t open_count() const;
    std::int32_t lowest_free() const;
    // Console sink: fd 1/2 writes land here so demos can capture output.
    std::string& console_out() { return console_; }
    const std::string& console_out() const { return console_; }
    void clear_console() { console_.clear(); }

private:
    std::array<FileDescriptor, MAX_FDS> fds_{};
    std::string console_;
};

}  // namespace vfs
}  // namespace lowl

#endif  // LOWL_VFS_HPP
