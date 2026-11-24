# `protocol_id` assignments

The `protocol_id` byte of the wire envelope (offset 2), which tells
swarm-routingd which protocol's decoder gets the rest of a frame
([swarm-routing/.github ADR-0006](https://github.com/swarm-routing/.github/blob/main/docs/adr/0006-one-wire-format-rule-and-a-daemon-only-envelope.md),
decision 3). The envelope and these constants are `core/swarm-envelope.h`;
the envelope is on the wire only on swarm-routingd's socket, never in ns-3.

**Rules.** An id is allocated by a PR to swarm-routing/swarm-core that adds
its row here and its constant in `core/swarm-envelope.h`, in the same PR. An
id is never reused, not even after its protocol is retired: its row stays,
marked retired. A protocol gets its id when its `core/` lands; comparator
arms run in ns-3 only and get none.

| `protocol_id` | Constant | Protocol | Repository |
|---:|---|---|---|
| `0x00` | `kProtocolIdInvalid` | invalid: a frame carrying it is dropped, and `writeEnvelope` refuses it | |
| `0x01` | `kProtocolIdAntHocNet` | AntHocNet | swarm-routing/AntHocNet |
| `0x02` | `kProtocolIdAra` | ARA | swarm-routing/ARA |
| `0x03` | `kProtocolIdBeeAdHoc` | BeeAdHoc | swarm-routing/BeeAdHoc |
| `0x04` | `kProtocolIdTermite` | Termite | swarm-routing/Termite |
| `0x05` | `kProtocolIdHopnet` | HOPNET | swarm-routing/HOPNET |
| `0x06`–`0xEF` | | unallocated, next in order | |
| `0xF0`–`0xFE` | `kProtocolIdExperimentalFirst`–`kProtocolIdExperimentalLast` | local experiments; never ship | |
| `0xFF` | `kProtocolIdReserved` | reserved; never allocated | |
