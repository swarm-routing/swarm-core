// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The protocol contract suite (#8): the checks every protocol core passes,
// header-only on the testbench (swarm-testbench.h), so a protocol supplies
// only the INodeAdapter<Msg> it already writes for its testbench tests.
//
//   DeterministicExecution              two runs with one seed give the same
//                                       trace and digest; another seed gives
//                                       another digest (org rule 5).
//   NullAndUnavailableProvidersAreSafe  the core runs with every optional port
//                                       null, and with every port present but
//                                       without a sample (NaN, "no evidence"),
//                                       and both runs put the same trace on
//                                       the air.
//   ZeroTtlAndSelfLoopPacketsAreDropped only for a protocol whose messages
//                                       carry a hop limit (ContractSpec::
//                                       hopLimited): a message with none left,
//                                       and a message back at its origin,
//                                       change nothing on the air.
//
// Header-only, the C++ standard library and core/ only (golden rule 10); no
// test framework: a check returns its failures as strings and
// runProtocolContract prints one line per check, so a plain ctest executable
// opts in:
//
//     namespace tb = swarm::core::testbench;
//     int main() {
//         tb::ContractSpec<MyMsg> spec;
//         spec.factory = [](tb::NodePorts<MyMsg>& p, const tb::ContractProviders& prov) {
//             return std::make_unique<MyAdapter>(p, prov);  // your testbench adapter
//         };
//         spec.hopLimited = [](swarm::core::NodeAddress origin, swarm::core::NodeAddress dest,
//                              int hopLimit) { return MyMsg::data(origin, dest, hopLimit); };
//         return tb::runProtocolContract(spec) == 0 ? 0 : 1;
//     }
//
// The optional ports are ContractProviders' pointers. It is the extension
// point for ports added later (#7's energy and location providers): a new
// port is a new pointer, null by default, plus an "unavailable" instance the
// suite hands in its second run, so an adapter written today keeps compiling.
//
// Determinism of the suite itself: every run is a fresh Network, seeded from
// the spec; no wall clock, no libc rand(), ordered containers only.
#ifndef SWARM_CORE_CONTRACT_TESTS_H
#define SWARM_CORE_CONTRACT_TESTS_H

#include "../swarm-observer.h"
#include "../swarm-ports.h"
#include "../swarm-types.h"
#include "swarm-testbench.h"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace swarm::core::testbench {

// --- the optional ports ---------------------------------------------------------

/// The optional ports a core may take besides the clock, the RNG, the timers
/// and the neighbours (which NodePorts gives). A null pointer is "no such
/// port"; the adapter hands each to its core as it would in a simulator.
struct ContractProviders {
    const ILinkState* linkState = nullptr;
    const ILinkQuality* linkQuality = nullptr;
    const IEnergy* energy = nullptr;
    IRouterObserver* observer = nullptr;
};

// --- what a protocol supplies -----------------------------------------------------

/// One protocol's opt-in: its adapter factory and the scene the checks run.
template <typename Msg> struct ContractSpec {
    /// Makes one node's adapter; hand `providers` to the core.
    using Factory = std::function<std::unique_ptr<INodeAdapter<Msg>>(NodePorts<Msg>&,
                                                                     const ContractProviders&)>;
    /// A message of the protocol from `origin` toward `dest` with `hopLimit`
    /// hops left, as a neighbour would send it.
    using HopLimited = std::function<Msg(NodeAddress origin, NodeAddress dest, int hopLimit)>;
    /// Extra script for every run (data to send, link changes), before it runs.
    using Script = std::function<void(Network<Msg>&)>;

    Factory factory;
    Topology topology = Topology::line(6);
    NetworkConfig config{};  ///< its seed is replaced by `seed` / `otherSeed`
    typename Network<Msg>::KindOf kindOf{};
    Script script{};
    Time duration = 10.0;
    std::uint64_t maxEvents = 1'000'000;  ///< a run that needs more fails
    std::uint64_t seed = 1;
    std::uint64_t otherSeed = 2;
    /// False for a core that draws no randomness that reaches the air: then
    /// DeterministicExecution does not ask another seed to differ.
    bool seedChangesTrace = true;

    /// Set only when the protocol's messages carry a hop limit; unset, the
    /// ZeroTtlAndSelfLoopPacketsAreDropped check is skipped.
    HopLimited hopLimited{};
    /// The hop limit of a message that still may travel (the control case).
    int liveHopLimit = 8;
    /// When the probe message is handed to a node; negative: duration / 2.
    Time injectAt = -1.0;
};

/// The outcome of one check.
struct ContractResult {
    const char* name = "";
    bool skipped = false;
    std::vector<std::string> failures;
    bool ok() const { return failures.empty(); }
};

namespace detail {

struct RunOutcome {
    std::uint64_t digest = 0;  ///< Network::digest, over every trace record
    std::uint64_t sent = 0;
    std::vector<std::string> failures;
};

template <typename Msg>
RunOutcome runOnce(const ContractSpec<Msg>& spec, std::uint64_t seed,
                   const ContractProviders& providers,
                   const typename ContractSpec<Msg>::Script& probe = {}) {
    RunOutcome out;
    if (!spec.factory) {
        out.failures.emplace_back("ContractSpec::factory is not set");
        return out;
    }
    NetworkConfig config = spec.config;
    config.seed = seed;
    Network<Msg> net(spec.topology, config, spec.kindOf);
    try {
        net.install([&spec, &providers](NodePorts<Msg>& p) { return spec.factory(p, providers); });
        if (spec.script) spec.script(net);
        if (probe) probe(net);
        const std::uint64_t ran = net.run(spec.duration, spec.maxEvents);
        if (ran == spec.maxEvents && net.events().pending() != 0) {
            out.failures.push_back("seed " + std::to_string(seed) + ": the run needed more than " +
                                   std::to_string(spec.maxEvents) + " events");
        }
    } catch (const std::exception& e) {
        out.failures.push_back("seed " + std::to_string(seed) + ": threw " + e.what());
    }
    if (net.events().refused() != 0) {
        out.failures.push_back("seed " + std::to_string(seed) + ": the event queue refused " +
                               std::to_string(net.events().refused()) + " events (unbounded)");
    }
    out.digest = net.digest();
    out.sent = net.sentTotal();
    return out;
}

inline void append(std::vector<std::string>& to, const std::vector<std::string>& from,
                   const std::string& prefix) {
    for (const std::string& f : from)
        to.push_back(prefix + f);
}

}  // namespace detail

// --- the checks ---------------------------------------------------------------------

/// Two runs with `seed` give the same trace and digest; a run with
/// `otherSeed` gives another digest (unless !seedChangesTrace). The scene
/// must put something on the air, or the check would prove nothing.
template <typename Msg> ContractResult checkDeterministicExecution(const ContractSpec<Msg>& spec) {
    ContractResult r{"DeterministicExecution", false, {}};
    const ContractProviders none{};
    const detail::RunOutcome a = detail::runOnce(spec, spec.seed, none);
    const detail::RunOutcome b = detail::runOnce(spec, spec.seed, none);
    detail::append(r.failures, a.failures, "first run: ");
    detail::append(r.failures, b.failures, "second run: ");
    if (a.sent == 0) r.failures.emplace_back("the scene sends nothing: nothing to compare");
    if (a.digest != b.digest) {
        r.failures.push_back("seed " + std::to_string(spec.seed) +
                             " twice gave two traces (state shared between runs, a wall "
                             "clock, or an unordered container)");
    }
    if (spec.seedChangesTrace) {
        const detail::RunOutcome c = detail::runOnce(spec, spec.otherSeed, none);
        detail::append(r.failures, c.failures, "other seed: ");
        if (c.digest == a.digest) {
            r.failures.push_back("seeds " + std::to_string(spec.seed) + " and " +
                                 std::to_string(spec.otherSeed) +
                                 " gave one trace: the core ignores its IRng");
        }
    }
    return r;
}

/// The core runs with every optional port null, and with every port present
/// but without a sample (a link state with empty queues and no service time,
/// a link quality and an energy port that read NaN, a no-op IRouterObserver);
/// "no evidence" must not change a decision, so both runs give one trace.
template <typename Msg>
ContractResult checkNullAndUnavailableProvidersAreSafe(const ContractSpec<Msg>& spec) {
    // Local classes: the sample-less ports are the check's, not API.
    struct UnavailableLinkState : ILinkState {
        int macQueueLength(NodeAddress /*nextHop*/) const override { return 0; }
        Time macServiceTime(NodeAddress /*nextHop*/) const override { return 0.0; }
    };
    struct UnavailableLinkQuality : ILinkQuality {
        double sinrDbFrom(NodeAddress /*neighbor*/) const override {
            return std::numeric_limits<double>::quiet_NaN();
        }
    };
    struct UnavailableEnergy : IEnergy {
        double residualEnergyFraction() const override {
            return std::numeric_limits<double>::quiet_NaN();
        }
    };

    ContractResult r{"NullAndUnavailableProvidersAreSafe", false, {}};
    const ContractProviders none{};
    const detail::RunOutcome nul = detail::runOnce(spec, spec.seed, none);
    detail::append(r.failures, nul.failures, "null providers: ");

    const UnavailableLinkState linkState{};
    const UnavailableLinkQuality linkQuality{};
    const UnavailableEnergy energy{};
    IRouterObserver observer;
    const ContractProviders unavailable{&linkState, &linkQuality, &energy, &observer};
    const detail::RunOutcome un = detail::runOnce(spec, spec.seed, unavailable);
    detail::append(r.failures, un.failures, "unavailable providers: ");

    if (nul.sent == 0) r.failures.emplace_back("the scene sends nothing: nothing to compare");
    if (nul.digest != un.digest) {
        r.failures.emplace_back("null and unavailable providers gave two traces: a port without "
                                "a sample changed a decision");
    }
    return r;
}

/// Only with spec.hopLimited. At injectAt, node 1 is handed, as if from
/// node 0, (a) a message with hop limit 0 and (b) its own message come back
/// (origin 1, liveHopLimit hops left); each run's trace must equal the run
/// without it. A control run hands a live message from node 0 and must
/// differ, so the check is not passed by a node that ignores what it is
/// handed. The topology needs nodes 0 and 1.
template <typename Msg>
ContractResult checkZeroTtlAndSelfLoopPacketsAreDropped(const ContractSpec<Msg>& spec) {
    ContractResult r{"ZeroTtlAndSelfLoopPacketsAreDropped", false, {}};
    if (!spec.hopLimited) {
        r.skipped = true;
        return r;
    }
    if (spec.topology.size() < 3) {
        r.failures.emplace_back("the topology needs at least 3 nodes");
        return r;
    }
    const ContractProviders none{};
    const Time when = spec.injectAt >= 0.0 ? spec.injectAt : spec.duration / 2.0;
    const NodeAddress dest = static_cast<NodeAddress>(spec.topology.size() - 1);
    auto handTo = [when](NodeAddress at, NodeAddress from, Msg msg) {
        return [when, at, from, msg](Network<Msg>& net) {
            net.at(when, [&net, at, from, msg]() { net.node(at).receive(from, msg); });
        };
    };

    const detail::RunOutcome base = detail::runOnce(spec, spec.seed, none);
    detail::append(r.failures, base.failures, "without a probe: ");

    const detail::RunOutcome live = detail::runOnce(
        spec, spec.seed, none, handTo(1, 0, spec.hopLimited(0, dest, spec.liveHopLimit)));
    detail::append(r.failures, live.failures, "live message: ");
    if (live.digest == base.digest) {
        r.failures.emplace_back("a live message (hop limit " + std::to_string(spec.liveHopLimit) +
                                ") changed nothing: the adapter ignores what it is handed, so "
                                "the drop cases prove nothing");
    }

    const detail::RunOutcome zero =
        detail::runOnce(spec, spec.seed, none, handTo(1, 0, spec.hopLimited(0, dest, 0)));
    detail::append(r.failures, zero.failures, "zero hop limit: ");
    if (zero.digest != base.digest) {
        r.failures.emplace_back("a message with hop limit 0 was acted on, not dropped");
    }

    const detail::RunOutcome loop = detail::runOnce(
        spec, spec.seed, none, handTo(1, 0, spec.hopLimited(1, dest, spec.liveHopLimit)));
    detail::append(r.failures, loop.failures, "self loop: ");
    if (loop.digest != base.digest) {
        r.failures.emplace_back("a message back at its origin was acted on, not dropped");
    }
    return r;
}

/// Every check, in a fixed order.
template <typename Msg> std::vector<ContractResult> contractResults(const ContractSpec<Msg>& spec) {
    return {checkDeterministicExecution(spec), checkNullAndUnavailableProvidersAreSafe(spec),
            checkZeroTtlAndSelfLoopPacketsAreDropped(spec)};
}

/// Runs every check, prints "ok", "skip" or "FAIL" and each failure per
/// check to `out` (none when null); returns the number of failed checks.
template <typename Msg>
int runProtocolContract(const ContractSpec<Msg>& spec, std::FILE* out = stdout) {
    int failed = 0;
    for (const ContractResult& r : contractResults(spec)) {
        if (!r.ok()) ++failed;
        if (out == nullptr) continue;
        std::fprintf(out, "%s %s\n", r.skipped ? "skip" : (r.ok() ? "ok  " : "FAIL"), r.name);
        for (const std::string& f : r.failures)
            std::fprintf(out, "     %s: %s\n", r.name, f.c_str());
    }
    return failed;
}

}  // namespace swarm::core::testbench

#endif  // SWARM_CORE_CONTRACT_TESTS_H
