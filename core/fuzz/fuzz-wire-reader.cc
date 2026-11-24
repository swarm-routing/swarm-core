// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// libFuzzer target for WireReader (#9), the reader every protocol's decoder
// runs on untrusted bytes. The input is a program and a payload:
//
//   byte 0        n, the number of op bytes (capped by the input's size)
//   bytes 1..n    the ops, one byte each: the low nibble picks the read
//                 (expectVersion, u8, boolean, u16, u32, u64, i32, f32, f64,
//                 count, fail), the high nibble is its argument
//   the rest      the payload the reader reads
//
// Beyond "never crashes, never reads out of bounds" (ASan + UBSan), it checks
// the reader's contract: it fails closed (once failed, every read returns 0,
// nothing moves, ok() and atEnd() stay false), remaining() only shrinks,
// count() never passes its bound or the bytes left, and while the reader is
// ok, re-encoding what it returned with WireWriter gives back exactly the
// bytes it consumed. Built only with -DSWARM_CORE_FUZZ=ON (clang).
#include "../swarm-wire.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

namespace {

void check(bool condition) {
    if (!condition) std::abort();
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size == 0) return 0;
    const std::size_t nOps = data[0] < size - 1 ? data[0] : size - 1;
    const std::span<const std::uint8_t> ops(data + 1, nOps);
    const std::span<const std::uint8_t> payload(data + 1 + nOps, size - 1 - nOps);

    swarm::core::WireReader r(payload);
    std::vector<std::uint8_t> echo;
    swarm::core::WireWriter w(echo);

    for (const std::uint8_t op : ops) {
        const bool wasOk = r.ok();
        const std::size_t before = r.remaining();
        const auto arg = static_cast<std::uint8_t>(op >> 4);
        std::uint64_t got = 0;
        switch (op & 0x0Fu) {
        case 0:
            r.expectVersion(arg);
            if (r.ok()) w.version(arg);
            break;
        case 1:
            got = r.u8();
            if (r.ok()) w.u8(static_cast<std::uint8_t>(got));
            break;
        case 2:
            got = r.boolean() ? 1 : 0;
            if (r.ok()) w.boolean(got == 1);
            break;
        case 3:
            got = r.u16();
            if (r.ok()) w.u16(static_cast<std::uint16_t>(got));
            break;
        case 4:
            got = r.u32();
            if (r.ok()) w.u32(static_cast<std::uint32_t>(got));
            break;
        case 5:
            got = r.u64();
            if (r.ok()) w.u64(got);
            break;
        case 6: {
            const std::int32_t v = r.i32();
            got = static_cast<std::uint32_t>(v);
            if (r.ok()) w.i32(v);
            break;
        }
        case 7: {
            const float v = r.f32();
            got = std::bit_cast<std::uint32_t>(v);
            if (r.ok()) w.f32(v);
            break;
        }
        case 8: {
            const double v = r.f64();
            got = std::bit_cast<std::uint64_t>(v);
            if (r.ok()) w.f64(v);
            break;
        }
        case 9: {
            // The bound and the element size from the argument: a bound of 0
            // to 15 elements of 0 to 3 bytes each (0: a count of nothing).
            const std::uint16_t maxCount = arg;
            const std::size_t elementSize = arg & 0x03u;
            const std::size_t n = r.count(maxCount, elementSize);
            got = n;
            if (r.ok()) {
                check(n <= maxCount);
                check(elementSize == 0 || n * elementSize <= r.remaining());
                w.u16(static_cast<std::uint16_t>(n));
            } else {
                check(n == 0);
            }
            break;
        }
        case 10:
            r.fail();
            break;
        default:
            continue;  // 11..15: no-op, so the fuzzer can pad
        }
        // Fails closed: a failed reader stays failed, returns 0 and never moves.
        check(r.remaining() <= before);
        if (!wasOk) {
            check(!r.ok());
            check(got == 0);
            check(r.remaining() == before);
        }
        if (!r.ok()) check(!r.atEnd());
        check(r.remaining() <= payload.size());
    }

    // What an ok reader returned re-encodes to the very bytes it consumed.
    if (r.ok()) {
        const std::size_t consumed = payload.size() - r.remaining();
        check(echo.size() == consumed);
        check(consumed == 0 || std::memcmp(echo.data(), payload.data(), consumed) == 0);
        check(r.atEnd() == (r.remaining() == 0));
    } else {
        check(!r.atEnd());
    }
    return 0;
}
