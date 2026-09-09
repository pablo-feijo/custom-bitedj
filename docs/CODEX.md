# Repository-local Codex tooling

This repository uses OpenAI's native agent instructions, project configuration
and discoverable skills. No custom agent loader or MCP server is needed.

| Location | Purpose |
| --- | --- |
| [AGENTS.md](../AGENTS.md) | Lightweight, automatically discovered repository instructions |
| [.codex/config.toml](../.codex/config.toml) | Shared Codex configuration for this trusted project |
| [.agents/skills/](../.agents/skills/) | Native skills with `SKILL.md`, optional UI metadata and focused references |
| `tasks/`, `test-results/` | Ignored execution records and generated output; not skill source |

## Available skills

Codex can select these from their descriptions, or you can invoke one by name.
Only the selected skill body and relevant references need to be read.

| Skill | When to use |
| --- | --- |
| [$bitedj-workflow](../.agents/skills/bitedj-workflow/SKILL.md) | Start/resume changes, isolate worktrees and apply architecture/validation rules |
| [$bitedj-build-test](../.agents/skills/bitedj-build-test/SKILL.md) | Build, test, operate owned GUI instances, capture or prepare Pi artifacts |
| [$bitedj-ui](../.agents/skills/bitedj-ui/SKILL.md) | Change settings, waveforms, effects, touch pads or controller UI; update maps/gallery |
| [$bitedj-integration](../.agents/skills/bitedj-integration/SKILL.md) | Authorized squash integration, exact-commit CI follow-through and cleanup |

Repository skills are discovered under `.agents/skills` from the working
directory up to the Git root. A plain `.agents/` folder is not an automatic
instruction loader. Each skill has native `name`/`description` frontmatter and
`agents/openai.yaml` UI metadata. [OpenAI skill documentation](https://learn.chatgpt.com/docs/build-skills).

## Project configuration

The tracked config keeps the documented 32 KiB `project_doc_max_bytes` ceiling;
small entry points and skill references control context growth rather than
truncating required instructions. Model, approval, sandbox and MCP settings
inherit from the user's environment. Project config loads only for trusted
projects; CLI overrides and managed policy still apply. Add MCP definitions only
for actual project services, with credentials outside Git.
[OpenAI configuration](https://learn.chatgpt.com/docs/config-file/config-basic),
[instruction discovery](https://learn.chatgpt.com/docs/agent-configuration/agents-md).

Start a new Codex session in this worktree to use its configuration and skills.
For a read-only local discovery check on the installed CLI, run
`codex debug prompt-input` and inspect the skill catalog (verified with CLI
0.153.4). Keep
that diagnostic output in ignored `test-results/`, since it includes session
context. This command renders inputs without starting a model task.

The installed CLI does not advertise a `codex init` subcommand. Maintain the
existing root `AGENTS.md` directly; do not replace it with a generated scaffold.

Under workspace-write sandboxing, agent configuration directories can be
protected even inside a writable repository. Follow the active session's actual
permissions and approval result; do not blanket-enable unrestricted access or
assume every edit needs another confirmation. See [OpenAI security guidance](https://learn.chatgpt.com/docs/agent-approvals-security).

## Maintaining these guides

- Keep root `AGENTS.md` under 60 lines and `docs/AGENTS.md` a redirect under 15.
  Keep skill entry points under 100 lines and references under 150 where practical.
- Put task triggers in the skill description and conditional details in linked
  `references/`. Preserve automatic selection; explicit invocation uses `$name`.
- Keep one owner for each control map. Public TESTING, GUI_TESTING,
  BRANCH_VERSIONING and LICENSING guides retain their canonical procedures.
- Update moved links, control maps, affected automation and UI gallery images
  together. Preserve runtime controls and existing authorization boundaries.
- Validate frontmatter with OpenAI's skill-creator `quick_validate.py`, check
  native discovery, TOML parsing, links/anchors, `git diff --check` and the staged
  storage guard. Use the relevant test layer when executable helpers change.
- Keep reusable helpers in the existing `scripts/` layout and execution state
  in ignored `tasks/` or `test-results/`. Do not put logs or caches in skills.
