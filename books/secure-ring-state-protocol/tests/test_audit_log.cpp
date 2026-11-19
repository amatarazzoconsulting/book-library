// MIT License - (c) Anthony Matarazzo 2026
#include <iostream>
#include <string>
#include <vector>

#include "audit_log.hpp"

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (!ok) ++failures;
}
using bytes = std::vector<std::uint8_t>;
}  // namespace

int main() {
    srsp::audit::options opt;
    opt.keyed = true;
    srsp::audit::audit_log log(opt);
    log.set_key(bytes(32, 0x42));
    check(log.keyed(), "a keyed log reports itself keyed");
    check(log.empty(), "starts empty");

    for (int i = 0; i < 5; ++i) {
        log.append(srsp::audit::severity::info, "node-a", "put",
                   "key-" + std::to_string(i));
    }
    check(log.size() == 5, "five records appended");
    const auto clean = log.verify();
    check(clean.ok && clean.checked == 5, "a clean chain verifies");
    check(!clean.head_hash.empty(), "verify reports the head hash");

    check(log.verify(log.head_hash()).ok, "verify accepts the expected head");

    const auto root = log.merkle_root();
    const auto proof = log.inclusion_proof(2);
    check(!proof.empty(), "an inclusion proof is produced");
    check(srsp::audit::audit_log::verify_inclusion(log.records()[2].hash, proof, root, 2, 5),
          "the inclusion proof verifies");
    check(!srsp::audit::audit_log::verify_inclusion(log.records()[3].hash, proof, root, 2, 5),
          "the proof rejects the wrong leaf");

    check(log.tamper_detail(2, "rewritten"), "tamper_detail edits a record");
    const auto broken = log.verify();
    check(!broken.ok, "the tampered chain fails");
    check(broken.bad_index == 2, "the break is located at the edited record");
    check(log.truncate_to(2), "truncate_to drops the damaged suffix");
    check(log.verify().ok, "the truncated chain verifies again");

    const std::string text = log.serialize();
    srsp::audit::audit_log restored;
    check(srsp::audit::audit_log::deserialize(text, restored, bytes(32, 0x42)),
          "a keyed log round trips with its key");
    check(restored.size() == log.size(), "the restored log has the same size");
    check(restored.verify().ok, "the restored log verifies");

    srsp::audit::options capped;
    capped.max_records = 2;
    srsp::audit::audit_log small(capped);
    small.append(srsp::audit::severity::info, "a", "x", "1");
    small.append(srsp::audit::severity::info, "a", "x", "2");
    check(!small.append(srsp::audit::severity::info, "a", "x", "3").has_value(),
          "max_records refuses further appends");
    check(small.size() == 2, "the capped log keeps two records");

    std::cout << (failures == 0 ? "all audit log tests passed\n" : "audit log failures\n");
    return failures == 0 ? 0 : 1;
}
