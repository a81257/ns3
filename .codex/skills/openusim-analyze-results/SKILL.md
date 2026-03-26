---
name: openusim-analyze-results
description: Use when the simulation produced outputs or failure evidence and the user wants to interpret results against the original experiment goal and investigate likely causes.
---

# OpenUSim Analyze Results

## Overview

Interpret completed run outputs against the original experiment goal.
Make claims only to the extent supported by output files, case inputs, and repo code.

## When to Use

- The simulation has completed and outputs already exist.
- The simulation failed or stalled, and there are partial outputs, error logs, or console messages to interpret.
- The user wants to know whether the results match the intended experiment goal.
- The user wants help investigating likely causes for an unexpected outcome.

Do not use this skill to initialize the repo or to define a new experiment from scratch.
Do not use this skill for pre-run readiness, case-file gating, or execution-path validation.

## The Process

1. Read the current `experiment-spec.md`.
2. Inspect the relevant case inputs and output artifacts.
2b. If the run failed or stalled, collect failure evidence:
    - Console messages (`buffer full`, `Potential Deadlock`, `No task completed`, etc.)
    - Partial trace files in `runlog/`
    - Compare the failing case's `network_attribute.txt` against a known-good baseline case
    - Identify the minimal parameter difference that could explain the failure
3. Choose the matching knowledge card before answering specialized domain questions.
4. Compare the observed evidence against the original experiment goal.
5. State what the evidence supports, what it does not support, and what remains uncertain.
6. If another iteration is needed, hand the next bounded decision back to planning.

## Stop And Ask

- The necessary output artifacts do not exist.
- The requested conclusion depends on evidence the repo does not currently produce.
- The user is actually changing the experiment definition rather than asking for result interpretation.

## Handover

Stay in this skill when:

- the user is still asking for interpretation of the current run
- the next question is still about result evidence or likely causes

Hand off to `openusim-plan-experiment` when:

- the user wants to change the experiment for another iteration
- the current analysis identifies a new bounded decision for the next run

Before handoff, record in `experiment-spec.md`:

- the most important findings that affect the next iteration
- the specific result gaps or hypotheses that motivated the next change

## Integration

- Called by: `openusim-run-experiment`, direct requests about existing outputs
- Hands off to: `openusim-plan-experiment`
- Required references:
  - `../openusim-references/throughput-evidence.md`
  - `../openusim-references/trace-observability.md`
  - `../openusim-references/queue-backpressure-vs-topology.md`
  - `../openusim-references/transport-channel-modes.md`

## References

- `../openusim-references/throughput-evidence.md`
- `../openusim-references/trace-observability.md`
- `../openusim-references/queue-backpressure-vs-topology.md`
- `../openusim-references/transport-channel-modes.md`

## Failure Interpretation Checklist

When the run failed or produced unexpected results, check in this order:

1. **Is it a simulation result or a toolchain bug?**
   - `buffer full. Packet Dropped!`, `Potential Deadlock`, `No task completed` → simulation result, not a bug
   - Segfault, Python traceback, missing file → toolchain bug, return to run-experiment

2. **Compare against baseline:**
   - Find the closest known-good case (e.g., `scratch/2nodes_single-tp`)
   - Diff `network_attribute.txt` to identify parameter differences
   - Focus on: `FlowControl`, buffer sizes, `EnableRetrans`, routing mode

3. **Form a single-variable hypothesis:**
   - Change exactly one parameter from the failing case toward the baseline value
   - Re-run and compare

4. **Record durable findings in `experiment-spec.md`:**
   - The failure symptom and its code-level origin
   - The parameter change that resolved it (or didn't)
   - The root cause chain (e.g., "synchronous burst + no backpressure + finite ingress buffer + no retransmission")

## Common Mistakes

- Claiming line-rate conclusions without stating both the metric and the comparison target.
- Confusing runtime overhead from trace settings with simulated network semantics.
- Treating missing evidence as proof.
- Changing the experiment definition inside the analysis phase instead of handing back to planning.
