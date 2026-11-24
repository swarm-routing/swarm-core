// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The wire envelope of swarm-routing/.github ADR-0006: 4 single-byte fields
// in front of a protocol message, which follows byte for byte, unchanged.
//
//   offset 0  envelope_version  0x01; read first, any other value is a drop
//   offset 1  magic             0x53 ('S')
//   offset 2  protocol_id       which protocol's decoder gets the rest; 0x00 invalid
//   offset 3  flags             envelope version 0x01 defines none: any set bit is a drop
//   offset 4  the protocol message, starting with that protocol's own version byte
//
// It is on the wire only on swarm-routingd's socket (and so on every runtime
// that runs its binary): never in ns-3, never in a protocol's codec, never in
// a published number (ADR-0006 decision 4). The protocol_id table is
// docs/protocol-ids.md: an id is allocated by a PR to swarm-routing/swarm-core
// and never reused. A change to the envelope's layout or meaning bumps
// kEnvelopeVersion; a flag bit is defined only with such a bump.
#ifndef SWARM_CORE_ENVELOPE_H
#define SWARM_CORE_ENVELOPE_H

#include "swarm-wire.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace swarm::core {

/// The envelope's own version, byte 0.
inline constexpr std::uint8_t kEnvelopeVersion = 0x01;
/// Byte 1, the same for every protocol.
inline constexpr std::uint8_t kEnvelopeMagic = 0x53;
/// The bytes in front of the protocol message.
inline constexpr std::size_t kEnvelopeSize = 4;

/// `protocol_id` values (docs/protocol-ids.md): allocated by PR, never reused.
inline constexpr std::uint8_t kProtocolIdInvalid = 0x00;
inline constexpr std::uint8_t kProtocolIdAntHocNet = 0x01;
inline constexpr std::uint8_t kProtocolIdAra = 0x02;
inline constexpr std::uint8_t kProtocolIdBeeAdHoc = 0x03;
inline constexpr std::uint8_t kProtocolIdTermite = 0x04;
inline constexpr std::uint8_t kProtocolIdHopnet = 0x05;
/// 0xF0-0xFE: local experiments, which never ship.
inline constexpr std::uint8_t kProtocolIdExperimentalFirst = 0xF0;
inline constexpr std::uint8_t kProtocolIdExperimentalLast = 0xFE;
/// Reserved: never allocated.
inline constexpr std::uint8_t kProtocolIdReserved = 0xFF;

/// A parsed envelope: the protocol it names and its message.
struct Envelope {
    std::uint8_t protocolId = kProtocolIdInvalid;
    /// The protocol message, a view into the parsed buffer (may be empty).
    std::span<const std::uint8_t> payload;
};

/// Appends the envelope for `protocolId` (flags 0) and then `payload`,
/// unchanged, to `w`, with at most one allocation. Writes nothing and returns
/// false when `protocolId` is kProtocolIdInvalid.
inline bool writeEnvelope(WireWriter& w, std::uint8_t protocolId,
                          std::span<const std::uint8_t> payload) {
    if (protocolId == kProtocolIdInvalid) return false;
    w.reserve(kEnvelopeSize + payload.size());
    w.version(kEnvelopeVersion);
    w.u8(kEnvelopeMagic);
    w.u8(protocolId);
    w.u8(0);  // flags: version 0x01 defines none
    w.append(payload.data(), payload.size());
    return true;
}

/// Parses untrusted `bytes` and fails closed: nullopt on fewer than 4 bytes,
/// an envelope version other than kEnvelopeVersion (checked first, in O(1),
/// before anything else is read), another magic, protocol_id 0x00, or any
/// flag bit set. Otherwise the protocol_id and the rest of `bytes` as the
/// payload, a view that must not outlive `bytes`. Which ids have a decoder is
/// the caller's dispatch, not this check.
inline std::optional<Envelope> readEnvelope(std::span<const std::uint8_t> bytes) {
    WireReader r(bytes);
    if (!r.expectVersion(kEnvelopeVersion)) return std::nullopt;
    const std::uint8_t magic = r.u8();
    const std::uint8_t protocolId = r.u8();
    const std::uint8_t flags = r.u8();
    if (!r.ok() || magic != kEnvelopeMagic || protocolId == kProtocolIdInvalid || flags != 0) {
        return std::nullopt;
    }
    return Envelope{protocolId, bytes.subspan(kEnvelopeSize)};
}

}  // namespace swarm::core

#endif  // SWARM_CORE_ENVELOPE_H
