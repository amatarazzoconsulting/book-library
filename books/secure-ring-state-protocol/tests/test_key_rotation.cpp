// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "key_rotation.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using bytes = std::vector<std::uint8_t>;
bytes master() { return bytes(32, 0x5A); }
}  // namespace

int main() {
    srsp::keys::config cfg;
    cfg.mode = srsp::keys::policy::fixed_epoch;
    cfg.rotate_after_ops = 3;
    cfg.start_epoch = 1;
    srsp::keys::rotation_schedule sched(cfg, master());

    check(sched.current_epoch() == 1, "starts at the configured epoch");
    check(sched.derive(1).size() == cfg.key_len, "derived key honours key_len");
    check(sched.derive(1) == sched.derive(1), "derivation is deterministic");
    check(sched.derive(1) != sched.derive(2), "different epochs derive different keys");

    for (int i = 0; i < 3; ++i) sched.record_operation();
    check(sched.rotations() == 1, "three operations trigger one rotation");
    check(sched.current_epoch() == 2, "epoch advanced");

    check(sched.check(sched.current_epoch()).ok, "the current epoch is accepted");
    check(!sched.check(99).ok, "a future epoch is refused");

    check(sched.force_rotate(2).ok, "force_rotate succeeds");
    check(sched.current_epoch() == 4, "force_rotate skips ahead");

    srsp::keys::config window;
    window.mode = srsp::keys::policy::sliding_window;
    window.overlap_epochs = 2;
    srsp::keys::rotation_schedule wide(window, master());
    check(wide.acceptance_window() == 3, "sliding window keeps overlap plus current");
    check(wide.check(wide.current_epoch()).ok, "current epoch accepted under a window");
    check(!wide.check(wide.current_epoch() + 5).ok, "far future refused under a window");

    check(!sched.journal().empty(), "the journal records events");
    const auto before = sched.current_epoch();
    sched.revoke(before);
    check(sched.get(before) == std::nullopt, "revoked epoch is no longer handed out");

    sched.reset();
    check(sched.current_epoch() == cfg.start_epoch && sched.rotations() == 0, "reset restores the schedule");

    std::cout << (failures == 0 ? "all key rotation tests passed\n" : "key rotation failures\n");
    return failures == 0 ? 0 : 1;
}
