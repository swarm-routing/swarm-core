# swarm-core

<!-- org-sync:begin badges -->
| Group | Badges |
| --- | --- |
| Build | [![Built on: ns3-dev](https://img.shields.io/badge/built%20on-ns3--dev%20image-blue?logo=docker)](https://github.com/orgs/swarm-routing/packages/container/package/ns3-dev) |
| Quality | [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Bugs](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=bugs)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![CodeRabbit Pull Request Reviews](https://img.shields.io/coderabbit/prs/github/swarm-routing/swarm-core?utm_source=oss&utm_medium=github&utm_campaign=swarm-routing%2Fswarm-core&labelColor=171717&color=FF570A&link=https%3A%2F%2Fcoderabbit.ai&label=CodeRabbit+Reviews)](https://coderabbit.ai) |
| Release | [![Release](https://img.shields.io/github/v/release/swarm-routing/swarm-core?sort=semver&cacheSeconds=1800)](https://github.com/swarm-routing/swarm-core/releases) [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.24476619.svg)](https://doi.org/10.5281/zenodo.24476619) [![Cite](https://img.shields.io/badge/cite-CITATION.cff-blue)](CITATION.cff) [![vcpkg: swarm-core](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fswarm-routing%2Fvcpkg-registry%2Fmain%2Fports%2Fswarm-core%2Fvcpkg.json&query=%24%5B%22version-semver%22%5D&label=vcpkg%20swarm-core)](https://github.com/swarm-routing/vcpkg-registry/tree/main/ports/swarm-core) [![Licence: MIT OR GPL-2.0-only](https://img.shields.io/badge/licence-MIT%20OR%20GPL--2.0--only-blue)](LICENSING.md) |
| Trust | [![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/swarm-routing/swarm-core/badge)](https://scorecard.dev/viewer/?uri=github.com/swarm-routing/swarm-core) [![OpenSSF Best Practices](https://www.bestpractices.dev/projects/17075/badge)](https://www.bestpractices.dev/projects/17075) |
| Stack | [![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus)](AGENTS.md) |
| Community & docs | [![Colony Arena: swarm-core](https://img.shields.io/badge/Colony%20Arena-project%20page-orange)](https://swarm-routing.github.io/projects/swarm-core/) [![Open RCAPA](https://img.shields.io/github/issues/swarm-routing/swarm-core/rcapa?label=open%20RCAPA&color=blue)](https://github.com/swarm-routing/swarm-core/blob/main/docs/rcapa/README.md) [![Conventional Commits](https://img.shields.io/badge/Conventional%20Commits-1.0.0-yellow)](https://www.conventionalcommits.org) |
<!-- org-sync:end badges -->

swarm-core, layer 1 of the org's hexagonal layout (swarm-routing/.github#672): the pure C++23 ports (`IClock`, `ITransport`, `IRouteTable`, `IEnergyProvider`, `ILocationProvider`, `ILinkMetrics`), the 4-byte common wire envelope (`magic`, `protocol_id`, `version`, `flags`) and the header-only `ProtocolContractTest` suite. No OS or simulator header. It will be extracted with its history from swarm-routing/swarm-adapter-ns3's `core/`.

**Status: `core/` extracted from swarm-routing/swarm-adapter-ns3 with its history (#6); first release pending (#10).** The plan is #3 and swarm-routing/.github#672. `core/` builds, tests and installs alone: `cmake -S core -B build/core && cmake --build build/core && ctest --test-dir build/core` (`make test`); consumers add it with `FetchContent` `SOURCE_SUBDIR core` or `find_package(swarm-core CONFIG)` (target `swarm::core`).

<!-- org-sync:begin develop -->
## Develop

New here? [docs/development.md](docs/development.md) is the developer guide:
the tools to install, how to get the code and its dependencies, how to
build, test, run and lint it, the Docker images, what CI checks and how to
read a red run. How changes are proposed, reviewed and released is the org's
[CONTRIBUTING.md](https://github.com/swarm-routing/.github/blob/main/CONTRIBUTING.md).
Whether a regression here gets an RCAPA (root cause analysis with corrective
and preventive actions), and how: [docs/rcapa/README.md](docs/rcapa/README.md).
<!-- org-sync:end develop -->

<!-- org-sync:begin citing -->
## Releases & citing

Versions follow [SemVer](https://semver.org) ([CHANGELOG.md](CHANGELOG.md)). Each release is listed under [Releases](https://github.com/swarm-routing/swarm-core/releases), built by the org's release workflow and archived on [Zenodo](https://zenodo.org) with its own DOI.

To cite this repository, use the “Cite this repository” button (from [CITATION.cff](CITATION.cff)):

| DOI | resolves to |
|---|---|
| [`10.5281/zenodo.24476619`](https://doi.org/10.5281/zenodo.24476619) | **concept DOI**: always the latest release. This is the badge above, and the right citation when you mean this project. |

Each release also has its own **version DOI**, listed on [its Zenodo record](https://doi.org/10.5281/zenodo.24476619): cite that one when you reproduce a number measured at that release.

Works that build on it:

- D. H. Joppi, *swarm-adapter-ns3: the ns-3 adapter of swarm-core*, 2026, [doi:10.5281/zenodo.24138628](https://doi.org/10.5281/zenodo.24138628). The ns-3 adapter of these ports (swarm-routing/swarm-adapter-ns3): the ns-3 contrib module every protocol links, built on swarm-core's release.

Using it in your work? Contributions are welcome, and citing the DOI helps just as much.
<!-- org-sync:end citing -->

## Epics

<!-- epics:begin -->
<!-- Generated by swarm-routing/.github tools/epics/epics.py from the issues labelled `epic`; edit the issues, then re-run it. -->

What is planned and in progress here: the issues labelled [`epic`](https://github.com/swarm-routing/swarm-core/issues?q=label%3Aepic). An epic's open sub-issues carry a `priority:*` and a `model:*` label, so a contributor or an AI agent can pick one up ([how issues work](https://github.com/swarm-routing/.github/blob/main/CONTRIBUTING.md#issues)). The whole org's plan: the [roadmap](https://github.com/swarm-routing/.github/blob/main/ROADMAP.md#epics).

| Epic | Status | Sub-issues closed |
|------|--------|-------------------|
| [#3](https://github.com/swarm-routing/swarm-core/issues/3) Extract swarm-core (C++23 ports, the daemon's 4-byte wire envelope, ProtocolContractTest) & release v1.0.0 | In progress | 2 / 9 |

<!-- epics:end -->

## Licence

MIT OR GPL-2.0-only, the whole repository: see [LICENSING.md](LICENSING.md).
