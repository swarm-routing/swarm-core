// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// libFuzzer target for readEnvelope (#9, #52; swarm-routing/.github ADR-0006),
// the first parser of every frame on swarm-routingd's socket. The input is the
// frame. Beyond "never crashes, never reads out of bounds" (ASan + UBSan), it
// checks the fail-closed rule exactly: a frame parses if and only if it has 4
// bytes or more, envelope_version 0x01, magic 0x53, a protocol_id other than
// 0x00 and flags 0; a parsed frame's payload is the rest of the bytes, in
// place, and writeEnvelope re-encodes it to the very same bytes. Built only
// with -DSWARM_CORE_FUZZ=ON (clang).
#include "../swarm-envelope.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

namespace {

void check(bool condition) {
    if (!condition) std::abort();
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    namespace sc = swarm::core;
    const std::span<const std::uint8_t> bytes(data, size);
    const auto env = sc::readEnvelope(bytes);

    const bool valid = size >= sc::kEnvelopeSize && data[0] == sc::kEnvelopeVersion &&
                       data[1] == sc::kEnvelopeMagic && data[2] != sc::kProtocolIdInvalid &&
                       data[3] == 0;
    check(env.has_value() == valid);
    if (!env) return 0;

    check(env->protocolId == data[2]);
    check(env->payload.data() == data + sc::kEnvelopeSize);
    check(env->payload.size() == size - sc::kEnvelopeSize);

    std::vector<std::uint8_t> echo;
    sc::WireWriter w(echo);
    check(sc::writeEnvelope(w, env->protocolId, env->payload));
    check(echo.size() == size);
    check(std::equal(echo.begin(), echo.end(), bytes.begin()));
    return 0;
}
