# swarm-core

<!-- org-sync:begin badges -->
| Group | Badges |
| --- | --- |
| Build | [![Built on: ns3-dev](https://img.shields.io/badge/built%20on-ns3--dev%20image-blue?logo=docker)](https://github.com/orgs/swarm-routing/packages/container/package/ns3-dev) |
| Quality | [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Bugs](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=bugs)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=swarm-routing_swarm-core&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=swarm-routing_swarm-core) [![CodeRabbit Pull Request Reviews](https://img.shields.io/coderabbit/prs/github/swarm-routing/swarm-core?utm_source=oss&utm_medium=github&utm_campaign=swarm-routing%2Fswarm-core&labelColor=171717&color=FF570A&link=https%3A%2F%2Fcoderabbit.ai&label=CodeRabbit+Reviews)](https://coderabbit.ai) |
| Release | [![Licence: MIT OR GPL-2.0-only](https://img.shields.io/badge/licence-MIT%20OR%20GPL--2.0--only-blue)](LICENSING.md) |
| Trust | [![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/swarm-routing/swarm-core/badge)](https://scorecard.dev/viewer/?uri=github.com/swarm-routing/swarm-core) |
| Stack | [![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus)](AGENTS.md) |
| Community & docs | [![Open RCAPA](https://img.shields.io/github/issues/swarm-routing/swarm-core/rcapa?label=open%20RCAPA&color=blue)](https://github.com/swarm-routing/swarm-core/blob/main/docs/rcapa/README.md) [![Conventional Commits](https://img.shields.io/badge/Conventional%20Commits-1.0.0-yellow)](https://www.conventionalcommits.org) |
<!-- org-sync:end badges -->

swarm-core, layer 1 of the org's hexagonal layout (swarm-routing/.github#672): the pure C++23 ports (`IClock`, `ITransport`, `IRouteTable`, `IEnergyProvider`, `ILocationProvider`, `ILinkMetrics`), the 4-byte common wire envelope (`magic`, `protocol_id`, `version`, `flags`) and the header-only `ProtocolContractTest` suite. No OS or simulator header. It will be extracted with its history from swarm-routing/swarm-adapter-ns3's `core/`.

**Status: planned.** Nothing is built yet; the plan is #3 and swarm-routing/.github#672.

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

## Licence

MIT OR GPL-2.0-only, the whole repository: see [LICENSING.md](LICENSING.md).
