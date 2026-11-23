## v2.0.0 (2026-11-23)

### BREAKING CHANGE

- the release asset swarm-core-<ver>.tar.gz now carries
core/ only (a CMake project: the primitives, the testbench, the CMake
package swarm::core), with no ns-3 module. The ns-3 module
(SwarmRoutingBase, the monitors, the helpers) ships from
swarm-routing/swarm-adapter-ns3 (its v2.0.0,
swarm-routing/swarm-adapter-ns3#231). A protocol's core/ keeps fetching
this tarball with SOURCE_SUBDIR core; an ns-3 module, such as
AntHocNet's contrib/swarm-core install, moves to swarm-adapter-ns3's
release.

### Feat

- **release**: release swarm-core from its own repository (#40)
- **core**: build, check and license core/ from swarm-core
- **core**: merge core/ from swarm-adapter-ns3 with its history
