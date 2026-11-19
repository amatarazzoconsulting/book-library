// MIT License - (c) Anthony Matarazzo 2026
#include "vfs.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace lowl {
namespace vfs {

const char* strerror(FsError e) {
    switch (e) {
        case FsError::Ok: return "success";
        case FsError::NoEnt: return "no such file or directory";
        case FsError::BadF: return "bad file descriptor";
        case FsError::NoMem: return "cannot allocate memory";
        case FsError::Exist: return "file exists";
        case FsError::NotDir: return "not a directory";
        case FsError::IsDir: return "is a directory";
        case FsError::Inval: return "invalid argument";
        case FsError::NoSpc: return "no space left on device";
        case FsError::Perm: return "permission denied";
    }
    return "unknown";
}

std::uint32_t Inode::block_count() const {
    std::uint32_t n = 0;
    for (const auto& b : blocks) {
        if (!b.empty()) ++n;
    }
    return n;
}

namespace {

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path) {
        if (c == '/') {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

}  // namespace

FileSystem::FileSystem() { format(); }

void FileSystem::format(std::uint32_t inode_capacity) {
    inodes_.clear();
    next_ino_ = ROOT_INO + 1;
    clock_ = 0;
    sb_ = Superblock();
    sb_.inodes = inode_capacity;
    sb_.block_size = BLOCK_SIZE;

    Inode root;
    root.ino = ROOT_INO;
    root.mode = S_IFDIR | 0755;
    root.nlink = 2;  // "." and ".."
    root.ctime = root.mtime = root.atime = clock_;
    root.entries["."] = ROOT_INO;
    root.entries[".."] = ROOT_INO;
    inodes_[ROOT_INO] = root;
}

Inode* FileSystem::inode(std::uint32_t ino) {
    auto it = inodes_.find(ino);
    return it == inodes_.end() ? nullptr : &it->second;
}

const Inode* FileSystem::inode(std::uint32_t ino) const {
    auto it = inodes_.find(ino);
    return it == inodes_.end() ? nullptr : &it->second;
}

std::uint32_t FileSystem::alloc_inode(std::uint32_t mode, std::uint32_t uid) {
    if (inodes_.size() >= sb_.inodes) return 0;
    Inode n;
    n.ino = next_ino_++;
    n.mode = mode;
    n.uid = uid;
    n.gid = uid;
    n.ctime = n.mtime = n.atime = clock_;
    std::uint32_t ino = n.ino;
    inodes_[ino] = n;
    return ino;
}

void FileSystem::free_inode(std::uint32_t ino) {
    if (ino == ROOT_INO) return;
    inodes_.erase(ino);
}

std::size_t FileSystem::free_inode_count() const {
    return sb_.inodes > inodes_.size() ? sb_.inodes - inodes_.size() : 0;
}

std::size_t FileSystem::used_blocks() const {
    std::size_t n = 0;
    for (const auto& kv : inodes_) n += kv.second.block_count();
    return n;
}

FsError FileSystem::resolve(const std::string& path, std::uint32_t cwd, std::uint32_t& out) const {
    std::uint32_t cur = (path.empty() || path[0] != '/') ? cwd : ROOT_INO;
    if (!inode(cur)) cur = ROOT_INO;
    std::vector<std::uint32_t> stack;
    stack.push_back(cur);

    for (const auto& part : split_path(path)) {
        if (part == ".") continue;
        if (part == "..") {
            if (stack.size() > 1) stack.pop_back();
            cur = stack.back();
            continue;
        }
        const Inode* d = inode(cur);
        if (!d) return FsError::NoEnt;
        if (!d->is_dir()) return FsError::NotDir;
        auto it = d->entries.find(part);
        if (it == d->entries.end()) return FsError::NoEnt;
        cur = it->second;
        stack.push_back(cur);
    }
    out = cur;
    return FsError::Ok;
}

std::string FileSystem::path_of(std::uint32_t ino) const {
    if (ino == ROOT_INO) return "/";
    // Breadth first upward search: find the directory holding `ino`.
    std::vector<std::pair<std::uint32_t, std::string>> frontier{{ino, ""}};
    for (int depth = 0; depth < 64 && !frontier.empty(); ++depth) {
        std::vector<std::pair<std::uint32_t, std::string>> next;
        for (const auto& f : frontier) {
            for (const auto& kv : inodes_) {
                const Inode& dir = kv.second;
                if (!dir.is_dir()) continue;
                for (const auto& e : dir.entries) {
                    if (e.first == "." || e.first == "..") continue;
                    if (e.second != f.first) continue;
                    std::string p = f.second.empty() ? e.first : e.first + "/" + f.second;
                    if (kv.first == ROOT_INO) return "/" + p;
                    next.push_back({kv.first, p});
                }
            }
        }
        frontier.swap(next);
    }
    return "?";
}

Inode* FileSystem::lookup_child(Inode& dir, const std::string& name) {
    auto it = dir.entries.find(name);
    if (it == dir.entries.end()) return nullptr;
    return inode(it->second);
}

FsError FileSystem::link_child(Inode& dir, const std::string& name, std::uint32_t ino) {
    if (!dir.is_dir()) return FsError::NotDir;
    if (dir.entries.count(name)) return FsError::Exist;
    dir.entries[name] = ino;
    dir.mtime = dir.ctime = clock_;
    return FsError::Ok;
}

bool FileSystem::dir_is_empty(const Inode& dir) const {
    for (const auto& e : dir.entries) {
        if (e.first == "." || e.first == "..") continue;
        return false;
    }
    return true;
}

FsError FileSystem::split_parent(const std::string& path, std::uint32_t cwd,
                                 std::uint32_t& parent, std::string& name) const {
    auto parts = split_path(path);
    if (parts.empty()) return FsError::Inval;
    name = parts.back();
    if (name == "." || name == "..") return FsError::Inval;
    std::string parent_path;
    if (!path.empty() && path[0] == '/') parent_path = "/";
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        if (!parent_path.empty() && parent_path.back() != '/') parent_path += '/';
        parent_path += parts[i];
    }
    if (parent_path.empty()) parent_path = ".";
    FsError err = resolve(parent_path, cwd, parent);
    if (err != FsError::Ok) return err;
    const Inode* p = inode(parent);
    if (!p || !p->is_dir()) return FsError::NotDir;
    return FsError::Ok;
}

FsError FileSystem::mkdir(const std::string& path, std::uint32_t cwd, std::uint32_t mode,
                          std::uint32_t uid) {
    std::uint32_t parent = 0;
    std::string name;
    FsError err = split_parent(path, cwd, parent, name);
    if (err != FsError::Ok) return err;

    Inode* p = inode(parent);
    if (p->entries.count(name)) return FsError::Exist;

    std::uint32_t ino = alloc_inode(S_IFDIR | (mode & 0x0FFF), uid);
    if (ino == 0) return FsError::NoSpc;
    Inode* d = inode(ino);
    d->nlink = 2;
    d->entries["."] = ino;
    d->entries[".."] = parent;
    link_child(*p, name, ino);
    p->nlink += 1;  // the new child's ".."
    return FsError::Ok;
}

FsError FileSystem::create(const std::string& path, std::uint32_t cwd, std::uint32_t mode,
                           std::uint32_t uid, std::uint32_t* out_ino) {
    std::uint32_t existing = 0;
    if (resolve(path, cwd, existing) == FsError::Ok) {
        const Inode* e = inode(existing);
        if (e && e->is_dir()) return FsError::IsDir;
        if (out_ino) *out_ino = existing;
        return FsError::Exist;
    }

    std::uint32_t parent = 0;
    std::string name;
    FsError err = split_parent(path, cwd, parent, name);
    if (err != FsError::Ok) return err;
    Inode* p = inode(parent);
    if (p->entries.count(name)) return FsError::Exist;

    std::uint32_t ino = alloc_inode(S_IFREG | (mode & 0x0FFF), uid);
    if (ino == 0) return FsError::NoSpc;
    inode(ino)->nlink = 1;
    link_child(*p, name, ino);
    if (out_ino) *out_ino = ino;
    return FsError::Ok;
}

FsError FileSystem::unlink(const std::string& path, std::uint32_t cwd) {
    std::uint32_t parent = 0, target = 0;
    std::string name;
    FsError err = split_parent(path, cwd, parent, name);
    if (err != FsError::Ok) return err;
    Inode* p = inode(parent);
    auto it = p->entries.find(name);
    if (it == p->entries.end()) return FsError::NoEnt;
    target = it->second;
    Inode* t = inode(target);
    if (!t) return FsError::NoEnt;
    if (t->is_dir()) return FsError::IsDir;
    p->entries.erase(it);
    p->mtime = p->ctime = clock_;
    t->nlink = t->nlink > 0 ? t->nlink - 1 : 0;
    if (t->nlink == 0) free_inode(target);
    return FsError::Ok;
}

FsError FileSystem::rmdir(const std::string& path, std::uint32_t cwd) {
    std::uint32_t parent = 0, target = 0;
    std::string name;
    FsError err = split_parent(path, cwd, parent, name);
    if (err != FsError::Ok) return err;
    Inode* p = inode(parent);
    auto it = p->entries.find(name);
    if (it == p->entries.end()) return FsError::NoEnt;
    target = it->second;
    if (target == ROOT_INO) return FsError::Inval;
    Inode* t = inode(target);
    if (!t) return FsError::NoEnt;
    if (!t->is_dir()) return FsError::NotDir;
    if (!dir_is_empty(*t)) return FsError::Inval;  // ENOTEMPTY
    p->entries.erase(it);
    p->nlink = p->nlink > 2 ? p->nlink - 1 : 2;
    p->mtime = p->ctime = clock_;
    free_inode(target);
    return FsError::Ok;
}

FsError FileSystem::stat(const std::string& path, std::uint32_t cwd, Stat& out) const {
    std::uint32_t ino = 0;
    FsError err = resolve(path, cwd, ino);
    if (err != FsError::Ok) return err;
    const Inode* n = inode(ino);
    if (!n) return FsError::NoEnt;
    out.st_ino = n->ino;
    out.st_mode = n->mode;
    out.st_nlink = n->nlink;
    out.st_uid = n->uid;
    out.st_gid = n->gid;
    out.st_size = n->size;
    out.st_atime = n->atime;
    out.st_mtime = n->mtime;
    out.st_ctime = n->ctime;
    out.st_blocks = n->block_count();
    return FsError::Ok;
}

std::int64_t FileSystem::read(std::uint32_t ino, std::uint64_t offset, char* buf,
                              std::size_t count) {
    Inode* n = inode(ino);
    if (!n) return -1;
    if (n->is_dir()) return -1;
    if (offset >= n->size) { n->atime = clock_; return 0; }

    std::uint64_t avail = n->size - offset;
    std::size_t want = static_cast<std::size_t>(std::min<std::uint64_t>(count, avail));
    std::size_t done = 0;
    while (done < want) {
        std::uint64_t pos = offset + done;
        std::uint32_t blk = static_cast<std::uint32_t>(pos / BLOCK_SIZE);
        std::uint32_t off = static_cast<std::uint32_t>(pos % BLOCK_SIZE);
        std::size_t chunk = std::min<std::size_t>(BLOCK_SIZE - off, want - done);
        if (blk < n->blocks.size() && !n->blocks[blk].empty()) {
            const std::string& b = n->blocks[blk];
            std::size_t have = b.size() > off ? b.size() - off : 0;
            std::size_t n_copy = std::min(chunk, have);
            if (n_copy) std::memcpy(buf + done, b.data() + off, n_copy);
            // A short block means a hole at the tail; leave zeros.
        } else {
            std::memset(buf + done, 0, chunk);
        }
        done += chunk;
    }
    n->atime = clock_;
    return static_cast<std::int64_t>(done);
}

std::int64_t FileSystem::write(std::uint32_t ino, std::uint64_t offset, const char* buf,
                               std::size_t count) {
    Inode* n = inode(ino);
    if (!n) return -1;
    if (n->is_dir()) return -1;
    if (count == 0) return 0;

    std::size_t done = 0;
    while (done < count) {
        std::uint64_t pos = offset + done;
        std::uint32_t blk = static_cast<std::uint32_t>(pos / BLOCK_SIZE);
        std::uint32_t off = static_cast<std::uint32_t>(pos % BLOCK_SIZE);
        std::size_t chunk = std::min<std::size_t>(BLOCK_SIZE - off, count - done);
        if (n->blocks.size() <= blk) n->blocks.resize(blk + 1);
        std::string& b = n->blocks[blk];
        if (b.size() < off + chunk) b.resize(off + chunk, '\0');
        std::memcpy(&b[off], buf + done, chunk);
        done += chunk;
    }
    if (offset + count > n->size) n->size = offset + count;
    n->mtime = n->ctime = clock_;
    return static_cast<std::int64_t>(count);
}

FsError FileSystem::truncate(std::uint32_t ino, std::uint64_t size) {
    Inode* n = inode(ino);
    if (!n) return FsError::NoEnt;
    if (n->is_dir()) return FsError::IsDir;
    std::uint32_t keep_blocks = static_cast<std::uint32_t>((size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    if (n->blocks.size() > keep_blocks) n->blocks.resize(keep_blocks);
    if (size > 0 && !n->blocks.empty()) {
        std::uint32_t last = static_cast<std::uint32_t>((size - 1) / BLOCK_SIZE);
        std::uint32_t off = static_cast<std::uint32_t>(size % BLOCK_SIZE);
        if (last < n->blocks.size() && off != 0) n->blocks[last].resize(off, '\0');
    }
    n->size = size;
    n->mtime = n->ctime = clock_;
    return FsError::Ok;
}

std::vector<std::uint32_t> FileSystem::list(std::uint32_t dir_ino) const {
    std::vector<std::uint32_t> out;
    const Inode* d = inode(dir_ino);
    if (!d || !d->is_dir()) return out;
    for (const auto& e : d->entries) out.push_back(e.second);
    return out;
}

// ---------------------------------------------------------------- FdTable

void FdTable::reset() {
    fds_.fill(FileDescriptor{});
    console_.clear();
    // 0,1,2 are the console. They have no inode but are always valid.
    for (std::int32_t fd = 0; fd < 3; ++fd) {
        fds_[static_cast<std::size_t>(fd)].fd = fd;
        fds_[static_cast<std::size_t>(fd)].open = true;
        fds_[static_cast<std::size_t>(fd)].flags = O_RDWR;
    }
}

bool FdTable::valid(std::int32_t fd) const {
    if (fd < 0 || fd >= MAX_FDS) return false;
    return fds_[static_cast<std::size_t>(fd)].open;
}

const FileDescriptor* FdTable::at(std::int32_t fd) const {
    if (!valid(fd)) return nullptr;
    return &fds_[static_cast<std::size_t>(fd)];
}

std::size_t FdTable::open_count() const {
    std::size_t n = 0;
    for (const auto& f : fds_) if (f.open) ++n;
    return n;
}

std::int32_t FdTable::lowest_free() const {
    for (std::int32_t i = 0; i < MAX_FDS; ++i) {
        if (!fds_[static_cast<std::size_t>(i)].open) return i;
    }
    return -1;
}

std::int32_t FdTable::open(FileSystem& fs, const std::string& path, std::uint32_t flags,
                           std::uint32_t mode, std::uint32_t cwd) {
    std::uint32_t ino = 0;
    FsError err = fs.resolve(path, cwd, ino);
    if (err != FsError::Ok) {
        if (!(flags & O_CREAT)) return -static_cast<std::int32_t>(err);
        FsError cerr = fs.create(path, cwd, mode, 0, &ino);
        if (cerr != FsError::Ok && cerr != FsError::Exist) {
            return -static_cast<std::int32_t>(cerr);
        }
    }
    const Inode* n = fs.inode(ino);
    if (!n) return -static_cast<std::int32_t>(FsError::NoEnt);
    if (n->is_dir()) return -static_cast<std::int32_t>(FsError::IsDir);

    std::int32_t fd = lowest_free();
    if (fd < 0) return -static_cast<std::int32_t>(FsError::NoMem);

    if (flags & O_TRUNC) fs.truncate(ino, 0);

    FileDescriptor d;
    d.fd = fd;
    d.ino = ino;
    d.flags = flags;
    d.refcount = 1;
    d.open = true;
    d.offset = (flags & O_APPEND) ? n->size : 0;
    fds_[static_cast<std::size_t>(fd)] = d;
    return fd;
}

std::int32_t FdTable::close(FileSystem& fs, std::int32_t fd) {
    (void)fs;
    if (!valid(fd) || fd < 3) return -static_cast<std::int32_t>(FsError::BadF);
    FileDescriptor& d = fds_[static_cast<std::size_t>(fd)];
    if (d.refcount > 0) --d.refcount;
    if (d.refcount == 0) d = FileDescriptor{};
    return 0;
}

std::int64_t FdTable::read(FileSystem& fs, std::int32_t fd, char* buf, std::size_t count) {
    if (!valid(fd)) return -static_cast<std::int64_t>(FsError::BadF);
    FileDescriptor& d = fds_[static_cast<std::size_t>(fd)];
    if (fd == 0) return 0;  // no keyboard in the simulator
    if (d.flags == O_WRONLY) return -static_cast<std::int64_t>(FsError::BadF);
    std::int64_t n = fs.read(d.ino, d.offset, buf, count);
    if (n > 0) d.offset += static_cast<std::uint64_t>(n);
    return n;
}

std::int64_t FdTable::write(FileSystem& fs, std::int32_t fd, const char* buf,
                            std::size_t count) {
    if (!valid(fd)) return -static_cast<std::int64_t>(FsError::BadF);
    FileDescriptor& d = fds_[static_cast<std::size_t>(fd)];
    if (fd == 1 || fd == 2) {
        console_.append(buf, count);
        return static_cast<std::int64_t>(count);
    }
    if (fd == 0) return -static_cast<std::int64_t>(FsError::BadF);
    if (d.flags == O_RDONLY) return -static_cast<std::int64_t>(FsError::BadF);
    if (d.flags & O_APPEND) {
        const Inode* n = fs.inode(d.ino);
        if (n) d.offset = n->size;
    }
    std::int64_t n = fs.write(d.ino, d.offset, buf, count);
    if (n > 0) d.offset += static_cast<std::uint64_t>(n);
    return n;
}

std::int64_t FdTable::lseek(std::int32_t fd, std::int64_t offset, int whence) {
    if (!valid(fd) || fd < 3) return -static_cast<std::int64_t>(FsError::BadF);
    FileDescriptor& d = fds_[static_cast<std::size_t>(fd)];
    std::int64_t base = 0;
    switch (whence) {
        case WHENCE_SET: base = 0; break;
        case WHENCE_CUR: base = static_cast<std::int64_t>(d.offset); break;
        default: return -static_cast<std::int64_t>(FsError::Inval);
    }
    std::int64_t next = base + offset;
    if (next < 0) return -static_cast<std::int64_t>(FsError::Inval);
    d.offset = static_cast<std::uint64_t>(next);
    return next;
}

std::int32_t FdTable::dup(std::int32_t fd) {
    if (!valid(fd)) return -static_cast<std::int32_t>(FsError::BadF);
    std::int32_t nf = lowest_free();
    if (nf < 0) return -static_cast<std::int32_t>(FsError::NoMem);
    fds_[static_cast<std::size_t>(nf)] = fds_[static_cast<std::size_t>(fd)];
    fds_[static_cast<std::size_t>(nf)].fd = nf;
    fds_[static_cast<std::size_t>(nf)].refcount += 1;
    return nf;
}

std::int32_t FdTable::dup2(std::int32_t oldfd, std::int32_t newfd) {
    if (!valid(oldfd) || newfd < 0 || newfd >= MAX_FDS) {
        return -static_cast<std::int32_t>(FsError::BadF);
    }
    fds_[static_cast<std::size_t>(newfd)] = fds_[static_cast<std::size_t>(oldfd)];
    fds_[static_cast<std::size_t>(newfd)].fd = newfd;
    fds_[static_cast<std::size_t>(newfd)].refcount += 1;
    return newfd;
}

FsError FdTable::fstat(const FileSystem& fs, std::int32_t fd, Stat& out) const {
    const FileDescriptor* d = at(fd);
    if (!d) return FsError::BadF;
    const Inode* n = fs.inode(d->ino);
    if (!n) return FsError::BadF;
    out.st_ino = n->ino;
    out.st_mode = n->mode;
    out.st_nlink = n->nlink;
    out.st_uid = n->uid;
    out.st_gid = n->gid;
    out.st_size = n->size;
    out.st_atime = n->atime;
    out.st_mtime = n->mtime;
    out.st_ctime = n->ctime;
    out.st_blocks = n->block_count();
    return FsError::Ok;
}

}  // namespace vfs
}  // namespace lowl
