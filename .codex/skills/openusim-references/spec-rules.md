# Spec Rules

Use this reference when writing or updating `experiment-spec.md`.

## Core rule

- `experiment-spec.md` is the only shared artifact across stage skills.
- It is the current experiment description, not a chat log.
- It must contain enough facts for the next stage skill to continue without hidden session memory.

## Natural-language intent resolution — cross-slot principle

Users may describe any planning slot in natural language rather than concrete values. This applies equally to topology, workload, network parameters, observability, and routing. The same resolution principle governs all slots:

1. **Decompose to the slot's primitive elements.** Every slot has a small set of irreducible building blocks:

| Slot | Primitive elements |
|------|--------------------|
| Topology | node groups + connectivity rules + bandwidth constraints |
| Workload | communication primitives (AllReduce, P2P, ...) + phase ordering + data sizes → traffic.csv rows |
| Network overrides | ns3 attribute key + value |
| Observability | named tier or individual trace-switch settings |
| Routing intent | algorithm choice + path-source choice |

2. **Search the matching reference for concrete options.** Each slot has a reference doc (`topology-options.md`, `workload-options.md`, `spec-to-toolchain.md`, `trace-observability.md`). If the user's intent matches an option in the reference, present it for confirmation.

3. **If multiple mappings are possible, present the choices.** Do not pick one silently.

4. **If the intent cannot be mapped or is ambiguous, ask the user.** Never silently assume a concrete value from vague natural language.

5. **Every mapping from natural language to a concrete value requires user confirmation** before writing into `experiment-spec.md`.

## Write-back timing

Write back only when:

- a planning slot gained stable new information
- the user confirmed that slot
- a stage handoff needs durable facts

Do not rewrite the whole spec on every turn.

## Minimal template

Use a small stable structure so every stage skill can find the same facts quickly:

```md
# Experiment Spec

## Goal
- what the user wants to learn

## Topology
- chosen topology family
- concrete sizing facts
- old case reference, if any

## Topology Realization
- `supported-family` or `custom-graph`
- bounded node/link facts if the topology is custom
- output materialization notes needed by run stage

## Workload
- chosen workload family or reference traffic file
- concrete scale facts
- rank_mapping (optional, default: linear)
- phase_delay (optional, default: 0)

## Routing Intent
- routing_algorithm (`HASH` or `ADAPTIVE`)
- whether only shortest-path candidates are allowed
- whether route generation is auto-path-finder or manual-route-table
- any bounded routing constraints that must survive handoff

## Network Overrides
- resolved parameter overrides that matter for the run

## Transport Channel Mode
- `precomputed` or `on-demand`
- default: `on-demand`

## Observability
- chosen trace/debug posture

## Startup Readiness
- startup facts that constrain generation or execution

## Execution Record
- actual case path
- actual run command
- produced output artifacts
- unresolved explicit run errors

## Validation Notes
- unsupported configuration asks that were rejected before run
- assumptions or fallback rules used when runtime discovery was incomplete

## Analysis Notes
- result findings that matter for the next iteration
- hypotheses to test next
```

Use empty sections only when the next stage clearly needs that slot.
Do not turn the spec into a transcript or a turn-by-turn checklist.

## Planning inputs

The planning surface must leave these durable facts before run handoff:

- experiment goal
- topology choice
- topology realization mode
- routing intent
- workload choice
- network parameter overrides
- transport channel mode (default to `on-demand` unless the user explicitly requests preconfigured TP mappings)
- observability choice
- explicit approval to generate or run

## Old case rule

If an old case is used as reference:

- summarize it in the conversation first
- do not write it into the new spec until the user says what to keep and what to change

## Parameter naming rule

Use toolchain-native parameter names in the spec to avoid translation ambiguity:
- `host_num`, `leaf_sw_num`, `comm_domain_size`, `data_size`
- See `spec-to-toolchain.md` for the full mapping

## Parameter value validation rule

The skill-layer toolchain validates parameter **keys** against the runtime catalog but does not validate parameter **values**. To reduce the risk of invalid values reaching ns-3:

- For enum parameters (e.g. `FlowControl`, `RoutingAlgorithm`, `VlScheduler`), verify the value against the catalog entry's `description` field or the C++ source `MakeEnumChecker(...)`. See `spec-to-toolchain.md` "Parameter value validation boundary" for the source-of-truth table.
- For `traffic.csv` `opType` values, verify against `TaOpcodeMap` in `src/unified-bus/model/ub-app.h`.
- Do not hardcode a fixed list of valid values in the spec or in agent logic — always consult the code or catalog as the authoritative source.

## Readiness rule

`ready for run` means:

- topology is concrete enough to generate with repo-native tools
- topology realization mode is explicit enough to produce case-root CSVs
- routing intent is explicit enough to choose auto or manual route generation
- workload is concrete enough to generate with repo-native tools
- main parameter choices are concrete enough for `network_attribute.txt`
- transport channel mode is chosen explicitly or defaults to `on-demand`
- observability mode is chosen
- explicit run approval has been given
