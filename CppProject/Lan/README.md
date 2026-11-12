# LAN collaboration core — integration status

This directory contains an independently tested networking foundation, not a finished collaboration mode in the editor. There is no host/join menu, project codec, undo adapter or viewport overlay hook yet. Building the application does not automatically open a network socket.

## Modules

- `LanState` and `LanTransactions`: host authority, explicit kind/field validators, object-specific permissions, edit leases, atomic batches, version checks, idempotent accepted requests and reference validation.
- `LanSession`: Qt TCP transport, invitation-code authentication, signed ordered envelopes, initial/resync snapshots, membership, permissions, presence messages and SHA-256-addressed resource blocks.
- `LanReplica`: canonical client state; validates snapshots and strictly consecutive commits before replacing its state.
- `LanPresence`: pose interpolation and mini-drone line geometry. It does not render anything automatically.
- `LanEditorBridge`: bounded event mailbox. Socket callbacks never call the project adapter. The editor must invoke `drain` at a safe main-loop boundary and provide an atomic `applyProject` implementation.

## Required editor integration

Register every supported project kind and field before hosting. Unknown kinds and fields are rejected. A project graph validator must additionally enforce engine-specific hierarchy, ownership and cycle rules. Group permission inheritance is NOT inferred by the core; the project adapter must resolve it explicitly before granting object scopes.

Seed the host from the current shared project state, then call `adoptHostState` before processing participant edits. Private viewport/UI state must not be included in project transactions. Client speculative drags must remain separate from the canonical replica.

Use the bridge's busy gate during loading, export, history replay or another editor transaction. The project adapter must either apply the entire event or leave the engine unchanged and return failure. Adapter failure closes the session instead of continuing with divergent engine state. Host edits must go through `hostTransaction`, not directly through `authority().transact`, so accepted edits are broadcast.

Camera geometry belongs exclusively in an interactive viewport overlay, never the scene-object collection, shadow pass, picking pass or final render. Coordinate conversion and the actual overlay renderer remain to be connected.

Resource blocks have no remote filesystem paths. Mapping models, skins, textures, videos and world data into a verified manifest and importing them into the engine are still required.

## Protocol limitations

Authentication is not encryption. Use only on a trusted LAN. Do not expose the listener to the Internet or claim TLS/confidentiality. All participants using the invitation code share its secret; this is not an identity system protecting mutually hostile participants from an active network attacker.

New participants are read-only. Object overrides replace global rights, including an empty override meaning deny. Editing/deleting existing objects requires a valid participant lease; the host observes other participants' active leases too.

A reconnect currently requires a new join attempt, creates a new participant identity and starts read-only with a fresh snapshot. Automatic retry policy and an identity-preserving approval flow are not implemented.

Limits include eight remote sockets, 256 operations per transaction, 50,000 allocated entity IDs per session, 8 MiB canonical state and 64 KiB resource blocks. Very large projects require a revised state/asset architecture, not merely increasing packet sizes.

## Standalone checks

Requirements: CMake, C++17 compiler, Qt 5.15 Core/Network/Gui.

```sh
cmake -S tools/lan-state-tests -B build/lan-tests -G Ninja
cmake --build build/lan-tests --parallel 2
ctest --test-dir build/lan-tests --output-on-failure
```

The suites cover authority invariants, real loopback host/client traffic, invitation/build rejection, replica convergence, idempotent retry, block transfer, revocation, reconnect, camera geometry and safe editor-boundary application. They do not prove Windows/macOS application builds or end-to-end Mine-imator project synchronization.

The CI pipeline now builds committed source bytes with read-only repository permissions. `repair.py` and its guarded payload were a one-time source-transfer migration and are not part of the current build pipeline.
