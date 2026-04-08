# AGENTS — How to interact with this repository using AI assistants

Purpose
- Provide clear, repo-specific instructions and prompt templates so Claude, Gemini, GitHub Copilot (Copilot Chat) and similar assistants can work effectively on this project.

Supported assistants
- Claude (Anthropic) — use system + user instruction style
- Gemini (Google) — use system + user instruction style
- GitHub Copilot Chat — use repo-aware instructions and Copilot customization files where available

Repo quick pointers
- Read the high-level project info first: [README.md](README.md)
- Main firmware entry: [knobby/knobby.ino](knobby/knobby.ino)
- C sources and headers: [knobby/](knobby/)
- HAL/device glue: [knobby/hal/](knobby/hal/)

General rules for all agents
- Always open and reference the files linked in “Repo quick pointers” before proposing changes.
- Prefer minimal, focused edits (fix root cause, don't patch around).
- Respect the LICENSE and CONTRIBUTING guidelines.
- When suggesting commands, use the exact `arduino-cli` commands from [README.md](README.md) for build/flash.
- Do not invent or assume hardware specifics beyond what's in README and source headers.

Suggested system prompts / personas

- Claude / Gemini (system message)
  You are a concise, repo-aware embedded firmware assistant. Prioritize safety and minimal changes. Always cite the files you read and include file links. When proposing code, give a small patch or a git-style diff and explain why it fixes the root cause. Use the repository's build instructions from [README.md](README.md) to validate commands.

- Copilot Chat
  Use this repository as the single source of truth. Follow existing code style and keep changes minimal. When asked to implement a feature or fix, list steps, produce code snippets, and show how to compile with `arduino-cli` from the README.

Task request template (user → agent)
- Title: short summary
- Goal: one-sentence acceptance criteria
- Context: files you reviewed (list links)
- Constraints: e.g., memory, performance, avoid external libs
- Tests: how to verify (compile, manual device steps)

Example user prompt to the agent
- "Fix the display rotation issue: files I checked: [knobby/scr_st77916.h](knobby/scr_st77916.h), [knobby/knobby.ino](knobby/knobby.ino). Acceptance: project compiles and UI renders upright. Propose minimal change and show compile steps."

Deliverables for code tasks
- Short plan (2–4 steps)
- Patch (unified diff or file snippet)
- Build commands to reproduce
- Short testing/verification steps

Commit & PR guidance
- Branch name: `fix/<short-desc>` or `feat/<short-desc>`
- Commit message: one-line summary + short body explaining rationale
- Include `arduino-cli compile` command used and result (success/failure)
- Small PRs are preferred; include screenshots or device-verified notes when applicable

Coding conventions & constraints
- Follow existing C style in repo (no one-letter non-trivial names)
- Prefer root-cause fixes; do not add heavy dependencies
- Keep flash/PSRAM settings consistent with README build flags

Security & safety
- Do not publish device access tokens or private keys.
- Avoid recommending unaudited third-party binary firmware.

Maintainers / contact
- Use the GitHub repo issues for discussion. Mention maintainers in PRs if available.

Notes
- There are no automated tests in the repo; the primary verification is `arduino-cli compile` and manual device verification per README.