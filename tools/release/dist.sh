#!/usr/bin/env bash
# Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
# SPDX-License-Identifier: MIT OR GPL-2.0-only
#
# dist.sh VERSION OUT-DIR: the release asset, for swarm-routing/.github's
# reusable-release.yml once the release is wired up here (#10):
#
#   swarm-core-VERSION.tar.gz   swarm-core/ with core/ (the primitives, with
#                               core/LICENSE; a CMake project of its own),
#                               docs/, README.md, LICENSE, LICENSE-MIT,
#                               LICENSING.md, VERSION, INSTALL.txt
#
# Consumers fetch it with FetchContent URL + URL_HASH and SOURCE_SUBDIR core,
# the layout the swarm-adapter-ns3 tarball had (#6), so only the URL changes.
#
# Deterministic: only files tracked by git, entries sorted with a fixed owner
# and the release commit's time, gzip -n.
#
# Then it checks what it built, so a release never ships a package that does
# not work (the workflow stops before signing or tagging on a non-zero exit):
# the tarball's core/ builds alone with plain CMake, passes ctest, installs as
# the CMake package swarm-core, and a scratch consumer finds it with
# find_package and passes. No ns-3, no docker: cmake and a C++23 compiler.
# DIST_CHECK=0 skips the check (a quick local build).
set -euo pipefail
ver=${1:?usage: dist.sh VERSION OUT-DIR}
out=${2:?usage: dist.sh VERSION OUT-DIR}
repo=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p "$out"
out=$(cd "$out" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
epoch=$(git -C "$repo" log -1 --format=%ct)

[ "$(cat "$repo/VERSION")" = "$ver" ] ||
    { echo "dist.sh: VERSION is $(cat "$repo/VERSION"), not $ver" >&2; exit 1; }

# --- swarm-core-VERSION.tar.gz ----------------------------------------------
pkg="$tmp/pkg/swarm-core"
mkdir -p "$pkg"
git -C "$repo" ls-files -z -- core docs README.md LICENSE LICENSE-MIT LICENSING.md VERSION |
    (cd "$repo" && xargs -0 cp --parents -t "$pkg")
cat > "$pkg/INSTALL.txt" <<TXT
swarm-core $ver (swarm-routing/swarm-core): the simulator-free primitives
every swarm-routing protocol core builds on. MIT OR GPL-2.0-only
(LICENSING.md, core/LICENSE). core/ targets C++23.

core/ alone, with no simulator:

  cmake -S swarm-core/core -B build && cmake --build build && ctest --test-dir build

installed as the CMake package swarm-core, for find_package:

  cmake --install build --prefix /path/to/prefix
  # in your project, with CMAKE_PREFIX_PATH=/path/to/prefix:
  #   find_package(swarm-core $ver CONFIG REQUIRED)
  #   target_link_libraries(my_core PUBLIC swarm::core)

or fetched by URL: FetchContent_Declare(swarm_core URL <this tarball's URL>
URL_HASH SHA256=<its sha256> SOURCE_SUBDIR core).

Source: https://github.com/swarm-routing/swarm-core/tree/v$ver
TXT
tar --sort=name --owner=0 --group=0 --numeric-owner --mtime="@${epoch}" \
    -C "$tmp/pkg" -cf - swarm-core | gzip -n > "$out/swarm-core-$ver.tar.gz"
ls -l "$out"

[ "${DIST_CHECK:-1}" = 0 ] && { echo "dist.sh: DIST_CHECK=0, the package is not checked"; exit 0; }

# --- check: the tarball's core/ builds, tests and installs alone -------------
mkdir -p "$tmp/unpacked"
tar -xzf "$out/swarm-core-$ver.tar.gz" -C "$tmp/unpacked"
src=$tmp/unpacked/swarm-core
cmake -S "$src/core" -B "$tmp/core-build"
cmake --build "$tmp/core-build" -j "$(nproc)"
ctest --test-dir "$tmp/core-build" --output-on-failure
cmake --install "$tmp/core-build" --prefix "$tmp/core-prefix"
cmake -S "$src/core/test/consumer-package" -B "$tmp/core-consumer" \
    -DCMAKE_PREFIX_PATH="$tmp/core-prefix" -DSWARM_CORE_WANT="$ver"
cmake --build "$tmp/core-consumer" -j "$(nproc)"
ctest --test-dir "$tmp/core-consumer" --output-on-failure
echo "dist.sh: swarm-core-$ver.tar.gz: its core/ passes ctest alone and installs as a package find_package(swarm-core $ver) finds"
