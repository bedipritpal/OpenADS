# Unmerged auth capacity switch

This is a daemon-only test proposal. No client ABI, worker executable, compiler, Qt, upstream or release change.

In the operator's existing INI passed through `--config`, add:

```ini
[server]
auth_worker_concurrency = 4
```

The build must explicitly set `-DOPENADS_AUTH_CAPACITY_TEST_SWITCH=ON`. This compile option defaults OFF. With OFF, the parser rejects this INI key (even value 4) and the daemon stays at four slots. This keeps the override out of production-shaped builds.

Default: 4. Accepted range: integer 1..256, inclusive. Dash spelling is accepted by the existing canonical key parser. Zero is not unlimited. Negative, blank, noninteger and out-of-range values reject the configuration with a line-numbered error and startup exit 2. Restart to change capacity. No CLI/environment override was added.

The bound is simultaneous child HRB invocations in the shared AuthWorker of one daemon. It is not a database-session limit and is independent of `max_sessions`. Higher values consume more memory/process resources, especially on x86. 256 is an allowed ceiling, not a tested throughput guarantee. The worker's two-second deadline is unchanged.

Admission remains fail-fast, with no queue. A full capacity gate returns `Auth busy` on the wire and closes the unauthenticated connection, without `login_failed`. Malformed responses, process failures/timeouts and HRB DENY retain existing authentication-failure throttling. Only verified capacity saturation avoids the penalty.

The existing client still collapses unsuccessful Complete responses into generic `5000: CompleteAuth denied`. There is no new retry-after, automatic retry, or reliable client distinction between busy and denied in this daemon-only change. Complete consumes its pending handle/challenge. After a known transient overload, stagger or back off before a new Begin, regenerate the blob for the new challenge, then Complete once promptly. Do not replay the old blob or sleep after Begin; challenge lifetime remains 30 seconds. Do not blindly retry all denied credentials.

Local validation: INI suite 19 cases/147 assertions; exact admission/recovery with 1, 4 and 10 slots; TLS test with ten busy refusals while one worker is held, no IP penalty, fresh Begin recovery, and genuine deny still throttled; daemon build; invalid INI startup rejection. A 100-child Python fixture exceeded the unchanged two-second deadline under container load. No claim of 100-worker throughput, native Windows storm performance, or owner acceptance.

Local capacity fixture uses a synthetic process for deterministic slot occupancy, not the owner's private HRB. Existing native Harbour worker and database proofs are separate evidence.
