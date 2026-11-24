// Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
// SPDX-License-Identifier: MIT OR GPL-2.0-only
//
// ILocationProvider: the position port (swarm-routing/swarm-core#7).
// Experimental: outside the SemVer promise until a second protocol uses it
// (docs/api-policy.md "Experimental"; maintainer decision 2026-11-22 on
// swarm-routing/swarm-core#7). Its first users are AntHocNet's kinematics
// (swarm-routing/AntHocNet#573, its IKinematics / Kinematics, ADR-0025) and
// POSANT.
//
// The fields are AntHocNet's: position (m) and velocity (m/s) in the host's
// Cartesian frame, compared only between nodes of one network, so no
// geodesy. A 2D host sets z (and vz) to 0. What differs: an unknown position
// is an empty optional, and an unknown velocity is an empty optional beside a
// known position, never zeros, so "standing still at the origin" and "no
// evidence" stay apart.
#ifndef SWARM_CORE_LOCATION_H
#define SWARM_CORE_LOCATION_H

#include <optional>

namespace swarm::core {

/// A Cartesian vector: metres for a position, metres per second for a
/// velocity. z is 0 on a 2D host.
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/// Where a node is and, when the host knows it, how it moves.
struct GeoPose {
    Vec3 position;
    /// Empty when the host has no velocity (a static or position-only host).
    std::optional<Vec3> velocity;
};

/// This node's position. Experimental (see the file comment).
class ILocationProvider {
public:
    virtual ~ILocationProvider() = default;
    /// This node's pose now, or empty when the position is unknown.
    virtual std::optional<GeoPose> pose() const = 0;
};

/// The location provider of a host without a position signal: always empty.
class NullLocationProvider final : public ILocationProvider {
public:
    std::optional<GeoPose> pose() const override { return std::nullopt; }
};

}  // namespace swarm::core

#endif  // SWARM_CORE_LOCATION_H
