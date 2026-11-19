// MIT License - (c) Anthony Matarazzo 2026
// Quorum based transaction commit for the ring. Any node may propose, a simple
// majority of ring members commits, and a proposal that can no longer reach a
// majority is rejected immediately instead of waiting for its deadline.
#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace srsp {
namespace consensus {

enum class tx_state { pending, accepted, committed, rejected };
const char* to_string(tx_state s);

enum class vote_kind { accept, reject, abstain };
const char* to_string(vote_kind k);

enum class reject_reason { none, quorum_lost, deadline, unknown_tx, duplicate_vote, not_member, bad_proposal };
const char* to_string(reject_reason r);

struct config {
    std::size_t ring_size = 3;      // voting members
    std::size_t quorum = 2;         // accepts needed to commit
    std::uint64_t deadline = 1000;  // logical ticks a proposal may stay pending
    bool reject_on_impossible = true;  // reject early when rejects make quorum impossible
};

struct vote {
    std::string node;
    vote_kind kind = vote_kind::accept;
    std::uint64_t at = 0;
};

struct transaction {
    std::string id;
    std::string payload_hash;
    tx_state state = tx_state::pending;
    reject_reason reason = reject_reason::none;
    std::vector<vote> votes;
    std::uint64_t created_at = 0;
    std::uint64_t decided_at = 0;
    std::size_t accepts = 0;
    std::size_t rejects = 0;

    bool decided() const { return state == tx_state::committed || state == tx_state::rejected; }
};

class engine {
public:
    explicit engine(config cfg);

    const config& settings() const { return cfg_; }

    bool propose(const std::string& id, const std::string& payload, std::uint64_t now,
                 std::string* error);
    bool cast_vote(const std::string& id, const std::string& node, vote_kind kind,
                   std::uint64_t now, std::string* error);

    // Rejects every pending proposal whose deadline has passed.
    std::size_t sweep(std::uint64_t now);

    const transaction* find(const std::string& id) const;
    const std::vector<transaction>& history() const { return order_; }

    std::size_t committed() const;
    std::size_t rejected() const;
    std::size_t pending() const;
    std::size_t votes_cast() const;

private:
    void tally(transaction& tx, std::uint64_t now);

    config cfg_;
    std::map<std::string, transaction> by_id_;
    std::vector<transaction> order_;  // insertion order, stable for reporting
    std::size_t votes_cast_ = 0;
};

}  // namespace consensus
}  // namespace srsp
