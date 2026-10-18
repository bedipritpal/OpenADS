# Vouch as a Service - design note

Status: brainstorm, recorded 2026-10-18 10:04 CDT. Not a committed direction.

## The idea

Vouch is installed on each client's own Windows machine. openads_serverd
runs remote on one reasonably good cloud server, one instance per customer,
each on its own port with its own openads.ini. Same pattern the owner
already ran with LetoDB on a Linux EC2 box, where Vouch itself ran in the
cloud.

## Owner's open concern

Data protection responsibility falls on the service operator (the owner).
Unresolved; this is the main reason the direction is not decided.

## Verified facts (main at 650ab997)

- serverd accepts --config with a path to any openads.ini. Precedence:
  command line over ini over defaults.
- Per-ini settings cover what must differ per customer: port, data,
  auth_module, tls_cert, tls_key, error_log_path, enable_file_func,
  legacy_paths, max_sessions.
- The auth worker is found next to the serverd binary and spawned fresh per
  login with that instance's own auth_module. One installation can serve
  many instances, each with its own .hrb.
- Record locks are OS byte-range locks (fcntl on Linux).
- One serverd per data root is the only tested model. Each customer needs
  its own data folder.
- serverd has no per-customer CPU, memory, or bandwidth quota. The machine
  is shared; max_sessions caps sessions per instance.
- 2026-10-17 22:10 CDT: one mtfix52 Linux serverd on EC2 held 5 concurrent
  Vouch instances (one on 10 threads) with zero errors in the client log.
  That was many clients on one serverd, not many serverd processes.

## Suggested layout (untested)

- /opt/openads: binaries (serverd and openads_auth_worker)
- /etc/openads/<customer>.ini: one ini per customer
- /srv/openads/<customer>/data: one data root per customer
- one log per customer via error_log_path
- one systemd template unit openads@.service, started per customer as
  openads@<customer>, running serverd --config /etc/openads/<customer>.ini
- TLS cert and key may be shared across instances or per customer; both are
  paths in the ini.

## Untested

- Many serverd processes on one box (capacity unknown).
- Two serverd instances sharing one data root (do not rely on this).
- The systemd template unit above.
- Deployment, backup, and data-protection operating procedures for a
  hosted service.
