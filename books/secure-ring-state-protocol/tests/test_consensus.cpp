// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>

#include "consensus.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using namespace srsp::consensus;
}  // namespace

int main() {
    config cfg;
    cfg.ring_size = 5;
    cfg.quorum = 3;
    cfg.deadline = 50;
    engine e(cfg);
    std::string error;

    check(e.propose("t1", "payload", 0, &error), "propose accepts a new id");
    check(!e.propose("t1", "payload", 0, &error), "propose rejects a duplicate id");
    check(!e.propose("", "payload", 0, &error), "propose rejects an empty id");
    check(!e.propose("t0", "", 0, &error), "propose rejects an empty payload");

    check(e.cast_vote("t1", "n1", vote_kind::accept, 1, &error), "first accept recorded");
    check(e.find("t1")->state == tx_state::accepted, "one accept of three leaves it accepted");
    check(!e.cast_vote("t1", "n1", vote_kind::accept, 2, &error), "a node votes once");
    check(!e.cast_vote("missing", "n1", vote_kind::accept, 2, &error), "votes on unknown ids are refused");

    e.cast_vote("t1", "n2", vote_kind::accept, 2, &error);
    check(e.find("t1")->state == tx_state::accepted, "two accepts are still short of quorum");
    e.cast_vote("t1", "n3", vote_kind::accept, 3, &error);
    check(e.find("t1")->state == tx_state::committed, "three accepts commit");
    check(e.find("t1")->decided_at == 3, "the decision tick is recorded");

    check(e.propose("t2", "payload2", 10, &error), "second proposal accepted");
    e.cast_vote("t2", "n1", vote_kind::reject, 11, &error);
    e.cast_vote("t2", "n2", vote_kind::reject, 12, &error);
    check(e.find("t2")->state != tx_state::rejected,
          "two rejects of five leave quorum mathematically possible");
    e.cast_vote("t2", "n3", vote_kind::reject, 13, &error);
    check(e.find("t2")->state == tx_state::rejected, "early rejection when quorum is impossible");
    check(e.find("t2")->reason == reject_reason::quorum_lost, "the reason is quorum_lost");

    check(e.propose("t3", "payload3", 100, &error), "third proposal accepted");
    e.cast_vote("t3", "n1", vote_kind::abstain, 101, &error);
    check(e.find("t3")->state == tx_state::pending, "an abstain alone leaves it pending");
    check(e.sweep(149) == 0, "sweep before the deadline does nothing");
    check(e.sweep(150) == 1, "sweep at the deadline rejects one proposal");
    check(e.find("t3")->reason == reject_reason::deadline, "the reason is deadline");

    check(e.committed() == 1 && e.rejected() == 2 && e.pending() == 0, "tallies agree");
    check(e.votes_cast() == 7, "seven votes were counted");
    check(e.history().size() == 3, "history keeps insertion order");
    check(std::string(to_string(tx_state::committed)) == "committed", "state names are stable");

    config single;
    single.ring_size = 1;
    engine one(single);
    check(one.settings().quorum == 1, "a single node ring needs one vote");
    one.propose("s1", "p", 0, &error);
    one.cast_vote("s1", "solo", vote_kind::accept, 0, &error);
    check(one.find("s1")->state == tx_state::committed, "one node commits alone");

    std::cout << (failures == 0 ? "all consensus tests passed\n" : "consensus failures\n");
    return failures == 0 ? 0 : 1;
}
