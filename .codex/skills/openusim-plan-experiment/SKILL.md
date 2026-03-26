---
name: openusim-plan-experiment
description: Use when the user wants to define one OpenUSim experiment and converge on a runnable experiment specification before any generation or execution.
---

# OpenUSim Plan Experiment

## Overview

Turn one natural-language OpenUSim goal into one stable `experiment-spec.md`.
Only solve one blocking planning slot per turn, but make sure topology shape and routing intent are explicit before handoff. Unless the user explicitly asks to preconfigure TP mappings, default `on-demand` `transport_channel_mode` behavior is acceptable.

## When to Use

- The user wants to design a new experiment.
- The user wants to refine an old case into a new experiment.
- The user asks to generate or run, but the experiment definition is still incomplete.

Do not use this skill to verify repo startup or to interpret completed run results.

## The Process

Output one line (using user's language) telling the user about the planning process, then bind the user-provided facts and ask for the next input. Follow the steps below:

1. Bind the user-provided facts before asking for more input.
2. Classify the first meaningful request as `broad`, `semi-specified`, or `reference-based`.
3. Resolve the first unstable planning slot in this order:
   - `goal`
   - `topology_shape` — validate parameter constraints from `topology-options.md` immediately (e.g. `host_num % leaf_sw_num == 0`); reject invalid values before proceeding
   - `routing_intent`
   - `workload` — for non-builtin workloads, follow the `custom-traffic-skeleton` decomposition steps in `workload-options.md`
   - `network_overrides` — verify parameter keys exist in the runtime catalog; for enum parameters, verify values against the catalog description or C++ source (see `spec-to-toolchain.md` "Parameter value validation boundary"); warn the user if a value cannot be confirmed
   - `observability`
   - `approval_ready`
4. Ask only one smallest blocking question per turn.
5. Use bounded `1/2/3/4` choices only when they help the user decide.
6. **When all slots are resolved and user approves generation/execution:**
   a. **Determine case directory:** Propose semantic case name with timestamp: `YYYYMMDD-<semantic-name>` (e.g., `20260322-clos-32hosts-bw-test`), form path as `scratch/{case_name}/`
   b. **Create case directory if not exists**
   c. **Use Write tool to create `{case_dir}/experiment-spec.md`** (template: `../openusim-references/spec-rules.md`)
   d. **Verify spec completeness** (use Read tool, check all required sections exist)
   e. **Announce handoff readiness** ("Spec written at `{case_dir}/experiment-spec.md`, ready to hand off to run stage")

## Stop And Ask

- The requested topology facts are still too incomplete to express as a bounded `custom-graph` or a supported family.
- The requested routing intent cannot be expressed through repo-native route generation or a bounded manual route table.
- The user asks for a parameter that is not in the current project parameter surface and cannot be confirmed from the runtime catalog or documented UB globals.
- The user asks to generate or run before the planning gate is satisfied.
- Startup facts are missing and block execution decisions.
- Repo startup facts have not been verified for this session and the user is approaching generation approval.

## Case Directory Naming

All experiment cases are created under `scratch/` with the following naming convention:

```
scratch/YYYYMMDD-<semantic-name>/
```

- `YYYYMMDD`: Date stamp (e.g., `20260322`)
- `<semantic-name>`: Short semantic description (e.g., `clos-32hosts-bw-test`, `ring-8nodes-a2a`)

**Examples:**
- `scratch/20260322-clos-32hosts-bw-test/`
- `scratch/20260322-ring-8nodes-a2a/`

The `experiment-spec.md` is written inside this directory: `scratch/YYYYMMDD-<name>/experiment-spec.md`

This `{case_dir}` is passed to `openusim-run-experiment` for generation and execution.

## Handover

Stay in this skill when:

- the experiment goal is still ambiguous
- topology shape, routing intent, or workload is still unresolved
- the user has not explicitly approved generation or execution

Hand off to `openusim-run-experiment` when:

- Step 6 of The Process is complete (spec written and verified, `{case_dir}` determined)
- The experiment goal is stable
- Topology, workload, network parameters, and observability are concrete enough to run
- The user explicitly approved generation or execution
- **Handoff data:** `{case_dir}` (full path to case directory under `scratch/`)

Before handoff, ensure `{case_dir}/experiment-spec.md` exists on disk with all required sections from `../openusim-references/spec-rules.md`. The file must contain:

- the confirmed experiment goal
- the chosen topology path or `custom-graph` realization facts
- the chosen routing intent
- the chosen workload path
- the chosen network parameter overrides
- the chosen transport channel mode (default `on-demand`, unless the user explicitly asked for preconfigured TP mappings)
- the chosen observability mode
- any assumptions needed to make execution concrete

Return to `openusim-welcome` when:

- repo startup facts are missing and block execution

## Integration

- Called by: user requests for experiment design, `openusim-welcome`, `openusim-analyze-results`
- Hands off to: `openusim-run-experiment`
- May return to: `openusim-welcome`
- Required references:
  - `../openusim-references/topology-options.md`
  - `../openusim-references/workload-options.md`
  - `../openusim-references/spec-rules.md`
  - `../openusim-references/spec-to-toolchain.md`

## References

- `../openusim-references/topology-options.md`
- `../openusim-references/workload-options.md`
- `../openusim-references/spec-rules.md`
- `../openusim-references/spec-to-toolchain.md`

## Common Mistakes

- Ignoring already-provided facts and falling back to a generic intake.
- Asking a full questionnaire when one missing decision is enough.
- Forcing an innovative topology back into a template family when the user already gave enough facts for a bounded `custom-graph`.
- Leaving routing intent implicit and hoping run-stage defaults match the user's goal.
- Prompting the user to configure `transport_channel.csv` even though the default path should stay `on-demand` unless they explicitly want fixed TP mappings.
- Treating runtime switches such as `--mtp-threads` as core experiment-definition slots.
- Starting generation or backend inspection before the planning gate is satisfied.
- Accepting topology parameters without checking constraints (e.g. `host_num % leaf_sw_num == 0` for clos-spine-leaf).
- Hardcoding parameter enum values from memory instead of verifying against the runtime catalog description or C++ source. Always check the actual source before telling the user a value is valid or invalid.
- Rejecting a non-builtin workload pattern when `custom-traffic-skeleton` in `workload-options.md` provides a concrete decomposition path.
