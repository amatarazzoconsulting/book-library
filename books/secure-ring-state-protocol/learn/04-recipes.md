# 04 - Recipes

**By Anthony Matarazzo (c) 2026** - MIT License

Short, runnable patterns. Each one is a fragment of a real program in `src/`.

## Bring a ring up, then drain it

```cpp
srsp::ring::ring_machine ring;
srsp::ring::guard_context ctx;
ctx.quorum_votes = 3;
ctx.quorum_required = 2;
ctx.peers_reachable = 3;

const std::size_t moved = ring.advance(ctx);   // boot -> probing -> joining -> syncing -> active
ctx.drain_requested = true;
ctx.drain_complete = true;
ring.step(srsp::ring::ring_state::draining, ctx);
ring.step(srsp::ring::ring_state::sealed, ctx);
```

`advance` stops at the first guard that fails, so it doubles as a readiness probe:
`moved == 4` means fully active.

## Reopen a sealed ring only on a new epoch

```cpp
ctx.rekey_ready = true;
ctx.epoch = ring.epoch();          // same epoch: refused
ring.step(srsp::ring::ring_state::probing, ctx);
ctx.epoch = ring.epoch() + 1;      // advanced epoch: accepted
ring.step(srsp::ring::ring_state::probing, ctx);
```

The guard is `epoch_advanced`, so rekeying without advancing the epoch does nothing.
That is deliberate: a sealed ring should not come back on the key it was sealed with.

## Authenticate a peer and seal a frame

```cpp
srsp::handshake::identity me{"node-a", my_secret};
srsp::handshake::identity peer{"node-b", peer_secret};
srsp::handshake::endpoint a(me), b(peer);
a.trust_peer("node-b", peer_secret);
b.trust_peer("node-a", my_secret);

if (!srsp::handshake::run_handshake(a, b).ok) return fail();

const auto& session = *a.established_session();
const auto sealed = session.seal(payload);       // genetic cipher + HMAC
bytes plain;
if (!b.established_session()->open(sealed, plain)) return fail();
```

Both endpoints must hold each other's verification key first. The handshake is a
pre-shared trust model, not a certificate chain.

## Rotate keys and refuse stale epochs

```cpp
srsp::keys::config cfg;
cfg.mode = srsp::keys::policy::sliding_window;
cfg.overlap_epochs = 2;
srsp::keys::rotation_schedule sched(cfg, master_secret);

sched.record_operation();                 // may rotate at the policy threshold
if (!sched.check(peer_epoch).ok) refuse();
const bytes key = sched.derive(sched.current_epoch());
sched.revoke(old_epoch);                  // make an epoch unservable
```

`acceptance_window()` returns overlap plus current, so `check()` has a single
definition of "still acceptable" rather than one per call site.

## Commit only when the majority agrees

```cpp
srsp::consensus::config cfg;
cfg.ring_size = 5;
cfg.quorum = 3;
srsp::consensus::engine votes(cfg);
std::string err;

votes.propose("tx-1", "balance:ada=100", tick(), &err);
votes.cast_vote("tx-1", "node-a", srsp::consensus::vote_kind::accept, tick(), &err);
votes.cast_vote("tx-1", "node-b", srsp::consensus::vote_kind::accept, tick(), &err);
votes.cast_vote("tx-1", "node-c", srsp::consensus::vote_kind::accept, tick(), &err);
// state is committed; two rejects of five would have left it pending
```

A node votes once. Duplicates are refused, unknown ids are refused, and a proposal
whose quorum has become impossible is rejected immediately with `quorum_lost`.

## Lease a resource safely

```cpp
srsp::locks::config cfg;
cfg.default_lease = 30000;
srsp::locks::table locks(cfg);

if (locks.acquire("db/users", session_id, node_id, now()) == srsp::locks::acquire_result::acquired) {
    // long job: refresh before the lease ends
    locks.refresh("db/users", session_id, now());
    locks.release("db/users", session_id, now());
}
```

Only the owning session may refresh or release. The version bumps on every
acquisition, so a holder that lost its lease can detect the loss by comparing
`version_of()` with the version it recorded.

## Versioned state with watchers and atomic batches

```cpp
srsp::store::kv_store db;
const auto token = db.watch("user:1", [](const std::string& k, const std::string& v, bool del) {
    log(k, v, del);
});

db.put("user:1", "ada", tick());

std::string err;
const bool ok = db.batch({{srsp::store::op_kind::put, "user:2", "grace"},
                          {srsp::store::op_kind::remove, "user:3", ""}},
                         tick(), &err);
db.unwatch(token);
```

If any operation in a batch is invalid, for example removing a key that does not
exist, nothing is applied and `err` explains which operation failed.

## Audit a change and prove it later

```cpp
srsp::audit::options opt;
opt.keyed = true;
srsp::audit::audit_log log(opt);
log.set_key(log_key);
log.append(srsp::audit::severity::info, node_id, "put", "user:1");

if (!log.verify().ok) {
    log.truncate_to(log.verify().bad_index);   // cut back to the last good prefix
}

const auto root = log.merkle_root();
const auto proof = log.inclusion_proof(index);
const bool member = srsp::audit::audit_log::verify_inclusion(
    log.records()[index].hash, proof, root, index, log.size());
```

`verify()` names the first bad record, so a replica can truncate to the last good
prefix instead of discarding the whole log. A keyed log can only be re-verified by a
holder of the same log key, which is why `deserialize` takes it explicitly.

Next: [../benchmarks/RESULTS.md](../benchmarks/RESULTS.md).
