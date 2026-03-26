import unittest
from pathlib import Path


class OpenUSimStageSkillDocsTest(unittest.TestCase):
    def repo_root(self):
        return Path(__file__).resolve().parents[2]

    def read_text(self, relative_path):
        return (self.repo_root() / relative_path).read_text(encoding="utf-8")

    def test_stage_skill_bundle_exists(self):
        repo_root = self.repo_root()
        for relative_path in (
            ".codex/skills/openusim-welcome/SKILL.md",
            ".codex/skills/openusim-plan-experiment/SKILL.md",
            ".codex/skills/openusim-run-experiment/SKILL.md",
            ".codex/skills/openusim-analyze-results/SKILL.md",
        ):
            self.assertTrue((repo_root / relative_path).is_file(), msg=relative_path)

    def test_stage_skill_docs_define_handover_surface(self):
        welcome_text = self.read_text(".codex/skills/openusim-welcome/SKILL.md")
        plan_text = self.read_text(".codex/skills/openusim-plan-experiment/SKILL.md")
        run_text = self.read_text(".codex/skills/openusim-run-experiment/SKILL.md")
        analyze_text = self.read_text(".codex/skills/openusim-analyze-results/SKILL.md")
        topology_text = self.read_text(".codex/skills/openusim-references/topology-options.md")
        spec_rules_text = self.read_text(".codex/skills/openusim-references/spec-rules.md")

        for text in (welcome_text, plan_text, run_text, analyze_text):
            self.assertIn("## Overview", text)
            self.assertIn("## When to Use", text)
            self.assertIn("## Handover", text)
            self.assertIn("## Integration", text)
            self.assertIn("Stay in this skill when:", text)

        self.assertIn("Hand off to `openusim-plan-experiment` when:", welcome_text)
        self.assertIn("Hand off to `openusim-run-experiment` when:", plan_text)
        self.assertIn("Before handoff, ensure `{case_dir}/experiment-spec.md` exists", plan_text)
        self.assertIn("Return to `openusim-welcome` when:", plan_text)
        self.assertIn("scratch/ns-3-ub-tools/net_sim_builder.py", run_text)
        self.assertIn("scratch/ns-3-ub-tools/traffic_maker/build_traffic.py", run_text)
        self.assertIn("Hand off to `openusim-analyze-results` when:", run_text)
        self.assertIn("Return to `openusim-plan-experiment` when:", run_text)
        self.assertIn("routing_intent", plan_text)
        self.assertIn("transport_channel_mode", plan_text)
        self.assertIn("default `on-demand`", plan_text)
        self.assertIn("custom-graph", plan_text)
        self.assertIn("graph.output_dir", run_text)
        self.assertIn("validate", run_text)
        self.assertIn("transport_channel_mode", run_text)
        self.assertIn("../openusim-references/trace-observability.md", analyze_text)
        self.assertIn("../openusim-references/throughput-evidence.md", analyze_text)
        self.assertIn("../openusim-references/transport-channel-modes.md", analyze_text)
        self.assertIn("../openusim-references/queue-backpressure-vs-topology.md", analyze_text)
        self.assertIn("## Failure Interpretation Checklist", analyze_text)
        self.assertIn("Hand off to `openusim-plan-experiment` when:", analyze_text)
        self.assertIn("### `custom-graph`", topology_text)
        self.assertIn("## Routing Intent", spec_rules_text)
        self.assertIn("## Transport Channel Mode", spec_rules_text)

    def test_welcome_skill_spells_out_startup_gate(self):
        welcome_text = self.read_text(".codex/skills/openusim-welcome/SKILL.md")
        for marker in (
            "`./ns3` exists",
            "`scratch/ns-3-ub-tools/` exists",
            "`scratch/ns-3-ub-tools/requirements.txt` exists",
            "`scratch/ns-3-ub-tools/net_sim_builder.py` exists",
            "`scratch/ns-3-ub-tools/traffic_maker/build_traffic.py` exists",
            "`scratch/ns-3-ub-tools/trace_analysis/parse_trace.py` exists",
            "`build/` exists",
            "`cmake-cache/` exists",
            "`scratch/2nodes_single-tp` exists",
            "`git submodule update --init --recursive`",
            "`python3 -m pip install --user -r scratch/ns-3-ub-tools/requirements.txt`",
            "`./ns3 configure`",
            "`./ns3 build`",
            "`./ns3 run 'scratch/ub-quick-example --case-path=scratch/2nodes_single-tp'`",
        ):
            self.assertIn(marker, welcome_text)

    def test_spec_rules_define_minimal_experiment_spec_shape(self):
        spec_rules_text = self.read_text(
            ".codex/skills/openusim-references/spec-rules.md"
        )
        for marker in (
            "## Minimal template",
            "# Experiment Spec",
            "## Goal",
            "## Topology",
            "## Topology Realization",
            "## Workload",
            "## Routing Intent",
            "## Network Overrides",
            "## Transport Channel Mode",
            "## Observability",
            "## Startup Readiness",
            "## Execution Record",
            "## Validation Notes",
            "## Analysis Notes",
        ):
            self.assertIn(marker, spec_rules_text)
        self.assertIn("default: `on-demand`", spec_rules_text)

    def test_repo_agents_route_by_stage_not_monolith(self):
        agents_text = self.read_text("AGENTS.md")
        self.assertIn("openusim-welcome", agents_text)
        self.assertIn("openusim-plan-experiment", agents_text)
        self.assertIn("openusim-run-experiment", agents_text)
        self.assertIn("openusim-analyze-results", agents_text)
        self.assertNotIn("openusim-helper", agents_text)

    def test_repo_entry_docs_match_stage_skill_surface(self):
        readme_text = self.read_text("README.md")
        readme_en_text = self.read_text("README_en.md")
        quick_start_text = self.read_text("QUICK_START.md")
        quick_start_en_text = self.read_text("QUICK_START_en.md")

        for text in (
            readme_text,
            readme_en_text,
            quick_start_text,
            quick_start_en_text,
        ):
            self.assertIn(".codex/skills/", text)
            self.assertIn("openusim-welcome", text)
            self.assertIn("openusim-plan-experiment", text)
            self.assertIn("openusim-run-experiment", text)
            self.assertIn("openusim-analyze-results", text)
            self.assertNotIn("openusim-helper", text)

    def test_old_openusim_helper_surface_is_gone(self):
        repo_root = self.repo_root()
        self.assertFalse((repo_root / ".codex/skills/openusim-helper/SKILL.md").exists())
        self.assertFalse((repo_root / ".codex/skills/openusim-helper" / "scripts").exists())
