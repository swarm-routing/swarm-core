// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The protocol contract suite (#8, core/testbench/swarm-contract-tests.h)
// against a toy protocol written for this test: a hop-limited flood with
// jittered rebroadcasts (so the seed reaches the air), duplicate suppression
// in a bounded DedupHistory, and the optional ports read through null-checked
// pointers. Every check passes on it; three broken variants show each check
// fails when its rule is broken.
#include "../swarm-dedup-history.h"
#include "../testbench/swarm-contract-tests.h"
#include "swarm-core-tests.h"

#include <cmath>
#include <cstdint>
#include <memory>

namespace swarm::core::test {

namespace {

namespace tb = swarm::core::testbench;

struct FloodMsg {
    NodeAddress origin = kInvalidAddress;
    NodeAddress dest = kInvalidAddress;
    std::uint32_t seq = 0;
    int hopLimit = 0;
};

/// How a toy node breaks the contract, for the negative tests.
enum class Flaw { None, SharedState, ForwardsSpent, PricesNaN };

/// The flaw of shared-state nodes leaks between runs, like a static in a core.
std::uint32_t gSharedCounter = 0;

class FloodNode : public tb::INodeAdapter<FloodMsg> {
public:
    FloodNode(tb::NodePorts<FloodMsg>& p, const tb::ContractProviders& prov, Flaw flaw)
        : p_(p), prov_(prov), flaw_(flaw) {}

    void start() override {
        if (p_.address() != 0) return;
        const NodeAddress last = 5;  // the far end of the suite's 6-node line
        for (std::uint32_t s = 1; s <= 3; ++s) {
            p_.schedule(0.5 * s, [this, last, s]() { originate(last, s); });
        }
    }

    void receive(NodeAddress from, const FloodMsg& m) override {
        if (prov_.observer != nullptr) prov_.observer->onControlReceived(1, 16);
        if (flaw_ != Flaw::ForwardsSpent) {
            if (m.hopLimit <= 0 || m.origin == p_.address()) return;
        }
        if (!seen_.record(m.origin, m.seq)) return;
        if (m.dest == p_.address()) return;
        // A sample-less port is no evidence: NaN reads as "keep going".
        double sinr =
            prov_.linkQuality != nullptr ? prov_.linkQuality->sinrDbFrom(from) : std::nan("");
        if (flaw_ == Flaw::PricesNaN && prov_.linkQuality != nullptr && std::isnan(sinr))
            sinr = -100.0;
        if (!std::isnan(sinr) && sinr < 0.0) return;
        const double energy =
            prov_.energy != nullptr ? prov_.energy->residualEnergyFraction() : std::nan("");
        if (!std::isnan(energy) && energy < 0.1) return;
        const int queued =
            prov_.linkState != nullptr ? prov_.linkState->macQueueLength(kInvalidAddress) : 0;
        FloodMsg fwd = m;
        fwd.hopLimit = m.hopLimit - 1;
        const Time jitter = 0.01 * p_.rng().uniform() + 0.001 * (queued > 0 ? queued : 0);
        p_.schedule(jitter, [this, fwd]() { send(fwd); });
    }

private:
    void originate(NodeAddress dest, std::uint32_t s) {
        const std::uint32_t seq = flaw_ == Flaw::SharedState ? ++gSharedCounter : s;
        // The shared counter shows on the air through the hop limit.
        const int hops = flaw_ == Flaw::SharedState ? 2 + static_cast<int>((seq - 1) / 3) : 8;
        FloodMsg m{p_.address(), dest, seq, hops};
        seen_.record(m.origin, m.seq);
        send(m);
    }
    void send(const FloodMsg& m) {
        if (prov_.observer != nullptr) prov_.observer->onControlSent(1, true, 16);
        p_.broadcast(m);
    }

    tb::NodePorts<FloodMsg>& p_;
    tb::ContractProviders prov_;
    Flaw flaw_;
    DedupHistory seen_{64};
};

tb::ContractSpec<FloodMsg> spec(Flaw flaw) {
    tb::ContractSpec<FloodMsg> s;
    s.factory = [flaw](tb::NodePorts<FloodMsg>& p, const tb::ContractProviders& prov) {
        return std::make_unique<FloodNode>(p, prov, flaw);
    };
    s.duration = 4.0;
    s.injectAt = 3.0;
    s.hopLimited = [](NodeAddress origin, NodeAddress dest, int hopLimit) {
        return FloodMsg{origin, dest, 0xC0DE, hopLimit};
    };
    return s;
}

}  // namespace

void TestContractToyPasses(Checker& c) {
    const auto results = tb::contractResults(spec(Flaw::None));
    c.check(results.size() == 3, "three checks");
    for (const auto& r : results) {
        c.check(!r.skipped, r.name);
        for (const auto& f : r.failures)
            c.check(false, std::string(r.name) + ": " + f);
    }
    c.check(tb::runProtocolContract(spec(Flaw::None), nullptr) == 0,
            "runProtocolContract: 0 failed");
}

void TestContractCatchesBrokenCores(Checker& c) {
    gSharedCounter = 0;
    c.check(!tb::checkDeterministicExecution(spec(Flaw::SharedState)).ok(),
            "state shared between runs fails DeterministicExecution");
    c.check(!tb::checkZeroTtlAndSelfLoopPacketsAreDropped(spec(Flaw::ForwardsSpent)).ok(),
            "forwarding a spent or looped message fails ZeroTtlAndSelfLoopPacketsAreDropped");
    c.check(!tb::checkNullAndUnavailableProvidersAreSafe(spec(Flaw::PricesNaN)).ok(),
            "reading NaN as a bad link fails NullAndUnavailableProvidersAreSafe");

    auto noTtl = spec(Flaw::None);
    noTtl.hopLimited = {};
    const auto skipped = tb::checkZeroTtlAndSelfLoopPacketsAreDropped(noTtl);
    c.check(skipped.skipped && skipped.ok(), "no hop limit: the check is skipped");

    auto silent = spec(Flaw::None);
    silent.factory = [](tb::NodePorts<FloodMsg>& p, const tb::ContractProviders& prov) {
        struct Mute : FloodNode {
            using FloodNode::FloodNode;
            void start() override {}
            void receive(NodeAddress, const FloodMsg&) override {}
        };
        return std::make_unique<Mute>(p, prov, Flaw::None);
    };
    c.check(!tb::checkDeterministicExecution(silent).ok(), "a silent scene proves nothing: fails");
    c.check(!tb::checkZeroTtlAndSelfLoopPacketsAreDropped(silent).ok(),
            "an adapter that ignores what it is handed fails the control case");

    tb::ContractSpec<FloodMsg> unset;
    c.check(!tb::checkDeterministicExecution(unset).ok(), "no factory: fails, no crash");
}

}  // namespace swarm::core::test
