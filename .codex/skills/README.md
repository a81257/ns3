# OpenUSim Skills

This directory contains repo-local Claude Code skills for the ns-3-ub project.

## Skill Overview

| Skill | Purpose | Entry Point |
|-------|---------|-------------|
| `openusim-welcome` | Mandatory first gate for repo initialization and readiness check | Always invoke first |
| `openusim-plan-experiment` | Experiment definition, custom topology clarification, and routing-intent capture | After welcome |
| `openusim-run-experiment` | Case generation, validation, execution, and explicit run errors | After plan |
| `openusim-analyze-results` | Result interpretation and likely-cause analysis | After run |

## Usage Flow

```
openusim-welcome (mandatory gate)
    ↓
openusim-plan-experiment (define experiment)
    ↓
openusim-run-experiment (generate & run)
    ↓
openusim-analyze-results (interpret results)
```

## References

Shared knowledge cards in `openusim-references/`:

- `trace-observability.md` - Trace/debug semantics
- `transport-channel-modes.md` - Transport channel semantics
- `throughput-evidence.md` - Throughput and line-rate interpretation
- `spec-to-toolchain.md` - Spec-to-toolchain mapping
- `topology-options.md` - Supported topology families and bounded `custom-graph` flow
- `workload-options.md` - Workload modes
- `spec-rules.md` - Experiment spec format rules
- `queue-backpressure-vs-topology.md` - Queue backpressure concepts

## Skill Discovery

These skills can be discovered via:
- `.codex/skills/` (canonical location)
- `.claude/skills/` (symlinks)

Each skill's `SKILL.md` defines its purpose and usage conditions.
