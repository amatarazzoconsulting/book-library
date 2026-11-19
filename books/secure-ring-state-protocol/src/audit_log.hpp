// MIT License - (c) Anthony Matarazzo 2026
//
// audit_log.hpp - tamper evident append only log with chained checksums.
//
// Each record stores prev_hash = hash(previous record) and its own hash, so
// editing, reordering, inserting or deleting any record breaks every hash that
// follows it. Records are also HMAC'd with a log key when one is supplied, which
// turns the chain from "integrity against accident" into "integrity against an
// author who cannot compute the MAC". Verification reports the index and reason
// of the first broken link so an operator can truncate the log at that point.
//
// Optional Merkle style checkpoint: the root over all record hashes lets a
// client that only knows a 32 byte root prove a single record is included.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace srsp {
namespace audit {

using bytes = std::vector<std::uint8_t>;

enum class severity {
    info = 0,
    notice,
    warning,
    critical
};

const char* to_string(severity s);
bool parse_severity(const std::string& name, severity& out);

struct record {
    std::uint64_t index = 0;
    std::uint64_t epoch = 0;
    std::uint64_t logical_time = 0;
    severity level = severity::info;
    std::string actor;
    std::string action;
    std::string detail;
    bytes payload;
    bytes prev_hash;
    bytes hash;
};

struct append_request {
    severity level = severity::info;
    std::string actor;
    std::string action;
    std::string detail;
    bytes payload;
    std::uint64_t epoch = 0;
};

enum class break_kind {
    none = 0,
    bad_index,
    broken_prev,
    broken_hash,
    bad_mac,
    truncated
};

const char* to_string(break_kind k);

struct verify_report {
    bool ok = false;
    std::size_t checked = 0;
    std::size_t bad_index = 0;
    break_kind kind = break_kind::none;
    std::string reason;
    bytes head_hash;
    bytes root_hash;
};

struct options {
    bool keyed = false;          // HMAC each record with the log key
    std::size_t hash_len = 32;   // truncation length for stored hashes
    bool include_payload = true; // fold payload bytes into the record hash
    std::size_t max_records = 0; // 0 means unbounded
    bool fsync_each = false;     // record a durable write in the journal
};

class audit_log {
public:
    explicit audit_log(options opt = options{});

    // Install an HMAC key. Without one the chain is still hash linked but not
    // authenticated, which the README calls out as a weaker mode.
    void set_key(const bytes& key);
    bool keyed() const { return opt_.keyed; }

    void tick(std::uint64_t amount = 1);
    std::uint64_t now() const { return clock_; }

    // Append one record. Returns the stored record, or nullopt when the log is
    // at max_records.
    std::optional<record> append(const append_request& req);

    // Convenience overloads.
    std::optional<record> append(severity level, const std::string& actor,
                                 const std::string& action, const std::string& detail);

    std::size_t size() const { return records_.size(); }
    bool empty() const { return records_.empty(); }
    const std::vector<record>& records() const { return records_; }
    std::optional<record> at(std::size_t index) const;

    bytes head_hash() const;
    std::string head_hex() const;

    // Verify the whole chain. Optional expected head lets a replica confirm it
    // converged on exactly the same log.
    verify_report verify() const;
    verify_report verify(const bytes& expected_head) const;

    // Merkle style root over the record hashes, for inclusion proofs.
    bytes merkle_root() const;
    std::vector<bytes> inclusion_proof(std::size_t index) const;
    static bool verify_inclusion(const bytes& leaf, const std::vector<bytes>& proof,
                                 const bytes& root, std::size_t index, std::size_t count);

    // Deliberate mutations, used by tests and by the teaching program to show
    // what a tamper looks like. They never touch the hash of the edited record.
    bool tamper_detail(std::size_t index, const std::string& new_detail);
    bool tamper_payload(std::size_t index, const bytes& new_payload);
    bool drop_record(std::size_t index);
    bool swap_records(std::size_t a, std::size_t b);

    // Cut the log back to a known good prefix, typically after verify() failed.
    bool truncate_to(std::size_t count);

    // Export and import in a simple line oriented text form.
    std::string serialize() const;
    // A keyed log can only be re-verified by a holder of the same log key, so
    // the key is supplied here rather than being stored in the text form.
    static bool deserialize(const std::string& text, audit_log& out,
                            const bytes& key = bytes{});

    const options& settings() const { return opt_; }

private:
    bytes compute_hash(const record& r) const;
    void reseal();

    options opt_;
    bytes key_;
    std::vector<record> records_;
    std::uint64_t clock_ = 0;
    std::size_t writes_ = 0;
};

}  // namespace audit
}  // namespace srsp
