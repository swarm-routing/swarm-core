// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The ports a simulator adapter implements. Derived from
// swarm-routing/AntHocNet core/include/anthocnet/core/ports.h @ 78c4640
// (MIT OR GPL-2.0-only): IClock, IRng, INeighborProvider, ITimerScheduler,
// ILinkState, ILinkQuality and IEnergy as there. AntHocNet's IKinematics
// stays behind: only AntHocNet uses it (golden rule 11); the experimental
// ILocationProvider (swarm-location.h, swarm-routing/swarm-core#7) is the
// shared position port meant to replace it. Its IRouterObserver,
// widened for every protocol, is in swarm-observer.h (#42).
//
// A protocol's core depends only on these interfaces, never on a simulator
// (golden rule 10): time, randomness, neighbours and deferred work all come in
// through them, so a fixed seed gives byte-identical output (org rule 5).
#ifndef SWARM_CORE_PORTS_H
#define SWARM_CORE_PORTS_H

#include "swarm-types.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace swarm::core {

/// Source of simulation time (seconds).
class IClock {
public:
    virtual ~IClock() = default;
    virtual Time now() const = 0;
};

/// Source of randomness. Every random choice a protocol makes draws through
/// this port, from the adapter's seeded stream; never libc rand() or the wall
/// clock (org rule 5).
class IRng {
public:
    virtual ~IRng() = default;
    /// Uniform double in [0, 1).
    virtual double uniform() = 0;
    /// Uniform integer in [0, n). The caller guarantees n > 0.
    virtual int uniformInt(int n) = 0;
};

/// The current one-hop neighbours, as the link layer sees them.
class INeighborProvider {
public:
    virtual ~INeighborProvider() = default;
    virtual std::vector<NodeAddress> neighbors() const = 0;
};

/// A cancellable timer (#46): what ITimerScheduler::scheduleCancellable
/// returns. kNoTimer is "nothing to cancel".
using TimerHandle = std::uint64_t;
inline constexpr TimerHandle kNoTimer = 0;

/// Deferred execution: periodic ant generation, table maintenance, timeouts.
class ITimerScheduler {
public:
    virtual ~ITimerScheduler() = default;
    virtual void schedule(Time delay, std::function<void()> callback) = 0;

    /// As schedule, and returns a handle cancel() takes, so a stale timeout
    /// (a discovery answered, a resend acknowledged) never runs. Additive: a
    /// scheduler that does not override it schedules as schedule() and
    /// returns kNoTimer, and cancel() is then a no-op, so a core that cancels
    /// must still tolerate its callback running.
    virtual TimerHandle scheduleCancellable(Time delay, std::function<void()> callback) {
        schedule(delay, std::move(callback));
        return kNoTimer;
    }
    /// Cancels a timer scheduleCancellable returned that has not run yet.
    /// kNoTimer, a timer that already ran, and one already cancelled are
    /// no-ops.
    virtual void cancel(TimerHandle /*timer*/) {}
};

/// Node-local MAC-layer signals for a congestion-aware per-hop cost. The
/// adapter owns the MAC, so it measures; a protocol turns these into a cost.
/// Both queries name the next hop the cost is computed for: a node with one
/// queue per interface (a satellite with several ISLs) has no single queue.
/// `nextHop == kInvalidAddress` asks for the aggregate across interfaces; a
/// single-interface adapter may ignore the parameter.
class ILinkState {
public:
    virtual ~ILinkState() = default;
    /// Packets queued for transmission toward `nextHop`. Negative reads as 0.
    virtual int macQueueLength(NodeAddress nextHop) const = 0;
    /// Smoothed per-packet MAC service time (seconds) toward `nextHop`, from
    /// the head of the interface queue to a successful transmission, without
    /// propagation. <= 0 when there is no sample yet.
    virtual Time macServiceTime(NodeAddress nextHop) const = 0;
};

/// Per-neighbour link quality: a property of one link, not of the node.
class ILinkQuality {
public:
    virtual ~ILinkQuality() = default;
    /// Signal-to-interference-and-noise ratio (dB) of the frames most recently
    /// received from `neighbor`. NaN when there is no sample: a link never
    /// heard on must not read as a bad link.
    virtual double sinrDbFrom(NodeAddress neighbor) const = 0;
};

/// This node's residual energy: the fraction of its initial battery left,
/// 0..1. NaN when the adapter has no energy model ("no evidence"), so an
/// energy-aware metric degrades to hop count, never to a wrong ranking.
class IEnergy {
public:
    virtual ~IEnergy() = default;
    virtual double residualEnergyFraction() const = 0;
};

/// What powers a node (swarm-routing/swarm-core#7). Unknown is "no evidence":
/// the host has no energy model.
enum class PowerSource : std::uint8_t {
    Unknown = 0,
    Battery = 1,
    Mains = 2,
};

/// One reading of a node's energy (swarm-routing/swarm-core#7), for the
/// energy-aware protocols (BeeAdHoc, BeeSensor, Q-Swarm). `fraction` is the
/// battery's residual fraction, 0..1, and is read only for a Battery source.
struct EnergySnapshot {
    PowerSource source = PowerSource::Unknown;
    double fraction = std::numeric_limits<double>::quiet_NaN();

    /// The residual fraction a protocol ranks by, with IEnergy's meaning:
    /// Battery gives `fraction` clamped to 0..1 (NaN stays NaN); Mains gives
    /// 1, a full battery that never drains (the mains fallback: a mains node
    /// is never priced as a drained relay); Unknown gives NaN, so an
    /// energy-aware metric degrades to hop count.
    double residualFraction() const {
        switch (source) {
        case PowerSource::Battery:
            if (std::isnan(fraction)) return fraction;
            return fraction < 0.0 ? 0.0 : (fraction > 1.0 ? 1.0 : fraction);
        case PowerSource::Mains:
            return 1.0;
        case PowerSource::Unknown:
            break;
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
};

/// The energy port with the power source (swarm-routing/swarm-core#7), added
/// beside IEnergy, which keeps its signature: an IEnergyProvider is an
/// IEnergy whose residualEnergyFraction() is snapshot().residualFraction(), so
/// it plugs in wherever an IEnergy does. A host implements snapshot() only.
class IEnergyProvider : public IEnergy {
public:
    virtual EnergySnapshot snapshot() const = 0;
    double residualEnergyFraction() const override { return snapshot().residualFraction(); }
};

/// The energy provider of a host without an energy signal: always Unknown, so
/// residualEnergyFraction() is NaN.
class NullEnergyProvider final : public IEnergyProvider {
public:
    EnergySnapshot snapshot() const override { return {}; }
};

}  // namespace swarm::core

#endif  // SWARM_CORE_PORTS_H
