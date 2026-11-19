// MIT License - (c) Anthony Matarazzo 2026
#include "consensus.hpp"

#include "sha256.hpp"

namespace srsp {
namespace consensus {

const char* to_string(tx_state s) {
    switch (s) {
        case tx_state::pending: return "pending";
        case tx_state::accepted: return "accepted";
        case tx_state::committed: return "committed";
        case tx_state::rejected: return "rejected";
    }
    return "unknown";
}

const char* to_string(vote_kind k) {
    switch (k) {
        case vote_kind::accept: return "accept";
        case vote_kind::reject: return "reject";
        case vote_kind::abstain: return "abstain";
    }
    return "unknown";
}

const char* to_string(reject_reason r) {
    switch (r) {
        case reject_reason::none: return "none";
        case reject_reason::quorum_lost: return "quorum_lost";
        case reject_reason::deadline: return "deadline";
        case reject_reason::unknown_tx: return "unknown_tx";
        case reject_reason::duplicate_vote: return "duplicate_vote";
        case reject_reason::not_member: return "not_member";
        case reject_reason::bad_proposal: return "bad_proposal";
    }
    return "unknown";
}

engine::engine(config cfg) : cfg_(cfg) {
    if (cfg_.ring_size == 0) cfg_.ring_size = 1;
    if (cfg_.quorum == 0) cfg_.quorum = cfg_.ring_size / 2 + 1;
    if (cfg_.quorum > cfg_.ring_size) cfg_.quorum = cfg_.ring_size;
}

bool engine::propose(const std::string& id, const std::string& payload, std::uint64_t now,
                     std::string* error) {
    if (id.empty() || payload.empty()) {
        if (error) *error = to_string(reject_reason::bad_proposal);
        return false;
    }
    if (by_id_.count(id)) {
        if (error) *error = "duplicate proposal id";
        return false;
    }
    transaction tx;
    tx.id = id;
    tx.payload_hash = srsp::hash::digest_hex(payload);
    tx.created_at = now;
    by_id_.emplace(id, tx);
    order_.push_back(tx);
    return true;
}

bool engine::cast_vote(const std::string& id, const std::string& node, vote_kind kind,
                       std::uint64_t now, std::string* error) {
    auto it = by_id_.find(id);
    if (it == by_id_.end()) {
        if (error) *error = to_string(reject_reason::unknown_tx);
        return false;
    }
    transaction& tx = it->second;
    if (tx.decided()) {
        if (error) *error = "transaction already " + std::string(to_string(tx.state));
        return false;
    }
    if (node.empty()) {
        if (error) *error = to_string(reject_reason::not_member);
        return false;
    }
    for (const auto& v : tx.votes) {
        if (v.node == node) {
            if (error) *error = to_string(reject_reason::duplicate_vote);
            return false;
        }
    }
    vote v;
    v.node = node;
    v.kind = kind;
    v.at = now;
    tx.votes.push_back(v);
    ++votes_cast_;
    tally(tx, now);

    // Mirror the decision into the insertion order copy so history() stays current.
    for (auto& recorded : order_) {
        if (recorded.id == id) recorded = tx;
    }
    return true;
}

void engine::tally(transaction& tx, std::uint64_t now) {
    tx.accepts = 0;
    tx.rejects = 0;
    for (const auto& v : tx.votes) {
        if (v.kind == vote_kind::accept) ++tx.accepts;
        if (v.kind == vote_kind::reject) ++tx.rejects;
    }
    if (tx.accepts >= cfg_.quorum) {
        tx.state = tx_state::committed;
        tx.decided_at = now;
        return;
    }
    // Quorum is impossible once enough members have refused: the remaining
    // undecided members cannot reach the threshold.
    const std::size_t undecided = cfg_.ring_size > tx.votes.size() ? cfg_.ring_size - tx.votes.size() : 0;
    if (cfg_.reject_on_impossible && tx.accepts + undecided < cfg_.quorum) {
        tx.state = tx_state::rejected;
        tx.reason = reject_reason::quorum_lost;
        tx.decided_at = now;
        return;
    }
    tx.state = tx.accepts > 0 ? tx_state::accepted : tx_state::pending;
}

std::size_t engine::sweep(std::uint64_t now) {
    std::size_t expired = 0;
    for (auto& kv : by_id_) {
        transaction& tx = kv.second;
        if (tx.decided()) continue;
        if (now >= tx.created_at + cfg_.deadline) {
            tx.state = tx_state::rejected;
            tx.reason = reject_reason::deadline;
            tx.decided_at = now;
            ++expired;
            for (auto& recorded : order_) {
                if (recorded.id == tx.id) recorded = tx;
            }
        }
    }
    return expired;
}

const transaction* engine::find(const std::string& id) const {
    auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : &it->second;
}

std::size_t engine::committed() const {
    std::size_t n = 0;
    for (const auto& kv : by_id_) n += kv.second.state == tx_state::committed ? 1 : 0;
    return n;
}

std::size_t engine::rejected() const {
    std::size_t n = 0;
    for (const auto& kv : by_id_) n += kv.second.state == tx_state::rejected ? 1 : 0;
    return n;
}

std::size_t engine::pending() const {
    std::size_t n = 0;
    for (const auto& kv : by_id_) n += kv.second.decided() ? 0 : 1;
    return n;
}

std::size_t engine::votes_cast() const { return votes_cast_; }

}  // namespace consensus
}  // namespace srsp
