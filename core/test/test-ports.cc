// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// The energy provider and the experimental location provider, with their
// null providers (swarm-routing/swarm-core#7).
#include "../swarm-energy-metric.h"
#include "../swarm-location.h"
#include "../swarm-ports.h"
#include "swarm-core-tests.h"

#include <cmath>
#include <limits>

namespace swarm::core::test {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

EnergySnapshot snap(PowerSource source, double fraction) {
    EnergySnapshot s;
    s.source = source;
    s.fraction = fraction;
    return s;
}

}  // namespace

void TestEnergyProvider(Checker& c) {
    // The defaults: no evidence.
    const EnergySnapshot none;
    c.check(none.source == PowerSource::Unknown && std::isnan(none.fraction),
            "default snapshot: Unknown, NaN fraction");
    c.check(std::isnan(none.residualFraction()), "Unknown reads NaN");
    c.check(std::isnan(snap(PowerSource::Unknown, 0.3).residualFraction()),
            "Unknown ignores a fraction");

    // Battery: the fraction, clamped; NaN stays NaN.
    c.check(snap(PowerSource::Battery, 0.42).residualFraction() == 0.42, "battery fraction");
    c.check(snap(PowerSource::Battery, -0.5).residualFraction() == 0.0, "battery clamped at 0");
    c.check(snap(PowerSource::Battery, 1.5).residualFraction() == 1.0, "battery clamped at 1");
    c.check(std::isnan(snap(PowerSource::Battery, kNaN).residualFraction()),
            "battery with no reading is NaN");

    // Mains: a full battery, whatever the fraction says.
    c.check(snap(PowerSource::Mains, kNaN).residualFraction() == 1.0, "mains reads 1");
    c.check(snap(PowerSource::Mains, 0.1).residualFraction() == 1.0, "mains ignores a fraction");

    // A host serving both ports answers IEnergy from the snapshot.
    struct Provider : IEnergyProvider, IEnergy {
        EnergySnapshot s;
        EnergySnapshot snapshot() const override { return s; }
        double residualEnergyFraction() const override { return s.residualFraction(); }
    } p;
    const IEnergyProvider& asProvider = p;
    const IEnergy& asEnergy = p;
    p.s = snap(PowerSource::Battery, 0.25);
    c.check(asProvider.snapshot().source == PowerSource::Battery, "snapshot through the port");
    c.check(asEnergy.residualEnergyFraction() == 0.25, "battery through IEnergy");
    p.s = snap(PowerSource::Mains, kNaN);
    c.check(asEnergy.residualEnergyFraction() == 1.0, "mains through IEnergy");
    // A mains relay costs EnergyMetric one plain hop, as a full battery.
    c.check(EnergyMetric{}.hopCost([&] {
        HopSignals h;
        h.residualEnergy = asEnergy.residualEnergyFraction();
        return h;
    }()) == 1.0,
            "a mains relay costs one hop");

    // The null provider: Unknown, so NaN: hop count, never a wrong ranking.
    const NullEnergyProvider null;
    const IEnergyProvider& nullPort = null;
    c.check(nullPort.snapshot().source == PowerSource::Unknown, "null provider: Unknown");
    c.check(std::isnan(nullPort.snapshot().residualFraction()), "null provider: NaN");
}

void TestLocationProvider(Checker& c) {
    const Vec3 origin;
    c.check(origin.x == 0.0 && origin.y == 0.0 && origin.z == 0.0, "Vec3 defaults to 0");
    const GeoPose still;
    c.check(!still.velocity.has_value(), "default pose: velocity unknown, not zero");

    // A 2D host with velocity, then a position-only host.
    struct Provider : ILocationProvider {
        std::optional<GeoPose> p;
        std::optional<GeoPose> pose() const override { return p; }
    } host;
    const ILocationProvider& port = host;
    c.check(!port.pose().has_value(), "unknown position is empty");

    GeoPose moving;
    moving.position = {10.0, -5.0, 0.0};
    moving.velocity = Vec3{1.5, 0.0, 0.0};
    host.p = moving;
    const auto got = port.pose();
    c.check(got.has_value() && got->position.x == 10.0 && got->position.y == -5.0 &&
                got->position.z == 0.0,
            "2D position");
    c.check(got.has_value() && got->velocity.has_value() && got->velocity->x == 1.5,
            "velocity when known");

    GeoPose fixed;
    fixed.position = {1.0, 2.0, 3.0};
    host.p = fixed;
    const auto got3 = port.pose();
    c.check(got3.has_value() && got3->position.z == 3.0 && !got3->velocity.has_value(),
            "3D position, velocity absent");

    // The null provider: always empty.
    const NullLocationProvider null;
    const ILocationProvider& nullPort = null;
    c.check(!nullPort.pose().has_value(), "null provider: empty");
}

}  // namespace swarm::core::test
