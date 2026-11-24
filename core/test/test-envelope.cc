// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The wire envelope (swarm-routing/.github ADR-0006 decision 6): a
// deterministic roundtrip for every assigned protocol_id, an empty payload,
// the payload's bytes untouched, and the reader failing closed on a wrong
// version, a wrong magic, protocol_id 0, any undefined flag bit and a buffer
// shorter than 4 bytes.
#include "../swarm-envelope.h"
#include "../swarm-wire.h"
#include "swarm-core-tests.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace swarm::core::test {

namespace {

using Bytes = std::vector<std::uint8_t>;

constexpr std::array<std::uint8_t, 5> kAssignedIds{kProtocolIdAntHocNet, kProtocolIdAra,
                                                   kProtocolIdBeeAdHoc, kProtocolIdTermite,
                                                   kProtocolIdHopnet};

Bytes wrap(std::uint8_t protocolId, std::span<const std::uint8_t> payload) {
    Bytes out;
    WireWriter w(out);
    writeEnvelope(w, protocolId, payload);
    return out;
}

bool sameBytes(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

}  // namespace

void TestEnvelopeRoundtrip(Checker& c) {
    c.check(kAssignedIds == std::array<std::uint8_t, 5>{0x01, 0x02, 0x03, 0x04, 0x05},
            "the assigned ids are docs/protocol-ids.md's");
    // A payload that starts like a protocol message (its own version byte)
    // and holds every byte value, including ones that look like an envelope.
    Bytes payload{0x04, 0x01, 0x53, 0x00, 0xFF};
    for (int b = 0; b < 256; ++b)
        payload.push_back(static_cast<std::uint8_t>(b));
    const Bytes original = payload;

    for (const std::uint8_t id : kAssignedIds) {
        const std::string tag = "id " + std::to_string(id) + ": ";
        const Bytes frame = wrap(id, payload);
        c.check(frame.size() == kEnvelopeSize + payload.size(), tag + "4 bytes added");
        c.check(frame.size() >= 4 && frame[0] == 0x01 && frame[1] == 0x53 && frame[2] == id &&
                    frame[3] == 0x00,
                tag + "the envelope's bytes");
        c.check(sameBytes(std::span(frame).subspan(kEnvelopeSize), payload),
                tag + "the payload follows byte for byte");
        c.check(wrap(id, payload) == frame, tag + "the same input, the same bytes");

        const auto env = readEnvelope(frame);
        c.check(env.has_value(), tag + "parses");
        if (!env) continue;
        c.check(env->protocolId == id, tag + "protocol_id back");
        c.check(sameBytes(env->payload, payload), tag + "payload back");
        c.check(env->payload.data() == frame.data() + kEnvelopeSize,
                tag + "the payload is a view into the frame");
        c.check(wrap(env->protocolId, env->payload) == frame, tag + "re-wraps to the same bytes");
    }
    c.check(payload == original, "the writer leaves the payload untouched");

    // Appending after earlier bytes keeps them.
    Bytes out{0xAA};
    WireWriter w(out);
    c.check(writeEnvelope(w, kProtocolIdAra, payload), "writes after earlier bytes");
    c.check(out.size() == 1 + kEnvelopeSize + payload.size() && out[0] == 0xAA &&
                out[1] == kEnvelopeVersion,
            "earlier bytes kept, the envelope appended");

    // protocol_id 0 is never written.
    Bytes none;
    WireWriter wn(none);
    c.check(!writeEnvelope(wn, kProtocolIdInvalid, payload) && none.empty(),
            "protocol_id 0: nothing written");
}

void TestEnvelopeEmptyPayload(Checker& c) {
    for (const std::uint8_t id : kAssignedIds) {
        const Bytes frame = wrap(id, {});
        c.check(frame == Bytes{0x01, 0x53, id, 0x00}, "the envelope alone");
        const auto env = readEnvelope(frame);
        c.check(env.has_value() && env->protocolId == id && env->payload.empty(),
                "parses to an empty payload");
    }
}

void TestEnvelopeFailsClosed(Checker& c) {
    const Bytes payload{0x04, 0x10, 0x20};
    const Bytes good = wrap(kProtocolIdAntHocNet, payload);
    c.check(readEnvelope(good).has_value(), "the reference frame parses");

    auto rejects = [&](Bytes frame, const std::string& what) {
        const Bytes before = frame;
        c.check(!readEnvelope(frame).has_value(), what + ": rejected");
        c.check(frame == before, what + ": the buffer is not touched");
    };

    // Wrong envelope version: every other value.
    for (int v = 0; v < 256; ++v) {
        if (v == kEnvelopeVersion) continue;
        Bytes f = good;
        f[0] = static_cast<std::uint8_t>(v);
        rejects(f, "version " + std::to_string(v));
    }
    // A wrong version is rejected on byte 0 alone, whatever follows.
    rejects(Bytes{0x02}, "version 2, 1 byte");
    // Wrong magic: every other value.
    for (int m = 0; m < 256; ++m) {
        if (m == kEnvelopeMagic) continue;
        Bytes f = good;
        f[1] = static_cast<std::uint8_t>(m);
        rejects(f, "magic " + std::to_string(m));
    }
    // protocol_id 0.
    {
        Bytes f = good;
        f[2] = kProtocolIdInvalid;
        rejects(f, "protocol_id 0");
    }
    // Every undefined flag bit, alone and all together.
    for (int bit = 0; bit < 8; ++bit) {
        Bytes f = good;
        f[3] = static_cast<std::uint8_t>(1u << bit);
        rejects(f, "flag bit " + std::to_string(bit));
    }
    {
        Bytes f = good;
        f[3] = 0xFF;
        rejects(f, "every flag bit");
    }
    // Shorter than 4 bytes: every prefix of a valid envelope.
    for (std::size_t n = 0; n < kEnvelopeSize; ++n) {
        rejects(Bytes(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n)),
                std::to_string(n) + " bytes");
    }
}

}  // namespace swarm::core::test
