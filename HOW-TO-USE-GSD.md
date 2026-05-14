# How to use GSD with this project

> Practical guide for using **GSD (Get Shit Done)** —
> <https://github.com/gsd-build/get-shit-done> — on the
> *Pool Water Temperature* project (battery ESP32 + DS18B20 → Home Assistant).

This is a **small, mostly-solo, mostly-finished hardware/firmware project**.
GSD is built for solo developers who want AI-driven work without enterprise
ceremony — but even then, full GSD is overkill for things like "fix one YAML
typo". This guide tells you **when to use which mode** so you get reliable
results without burning context or your patience.

Status assumed (from `SESSION_CHECKPOINT.md`, 2026-05-02):

- Hardware locked-in: LOLIN32 Lite v1.0.0, BOM finalised, wiring done.
- Smoke-test ESPHome YAML running on hardware (DS18B20 + WiFi + battery
  monitoring verified).
- Open: production-layer ESPHome build (deep sleep, esp-idf framework,
  multi-AP, pinned DS18B20 ROM, safe_mode, captive_portal).
- Repo published at `git@github.com:AINxtGenDev/poolwatertemperature` (public).

---

## 1. What GSD is in 30 seconds

GSD is a **meta-prompting + spec-driven workflow** for Claude Code. It
fights *context rot* (Claude getting dumber as its context fills) by:

- Splitting work into **phases** with traceable requirements.
- Spawning **fresh executor agents** per task so each starts with a clean
  200k context.
- Writing **atomic git commits** per task so deviations are easy to revert.
- Persisting decisions in `.planning/` so cross-session memory survives.

You drive it with `/gsd-*` slash commands inside Claude Code. The current
local install is **GSD 1.39.1** (per checkpoint); upstream has since
shipped 1.40 (six namespace meta-skills, see §10) and 1.41 (post-merge
build/test gate, runtime profile expansion, milestone-archive layout
support).

Since v1.40 you can also drive GSD via *namespace routers* —
`/gsd-workflow`, `/gsd-project`, `/gsd-quality`, `/gsd-context`,
`/gsd-manage`, `/gsd-ideate` — which dispatch to the concrete sub-skill
based on intent. Concrete commands (`/gsd-plan-phase`, etc.) still work
unchanged. The routers cut cold-start system-prompt overhead from
~2 150 tokens to ~120 — useful if you run `--minimal`.

A v1.51 *Package Legitimacy Gate* runs `slopcheck` against any package
the researcher recommends and surfaces a `[SLOP]`/`[SUS]`/`[OK]` audit
in `RESEARCH.md`. Largely irrelevant for this firmware project (no npm
deps), but if a future phase needs e.g. a Python helper for log parsing,
expect a `checkpoint:human-verify` before any unfamiliar package is
installed.

---

## 2. Does this project actually need GSD?

Be honest: not all of it.

| Task type in this project              | Recommended tool                |
| --------------------------------------- | ------------------------------- |
| One-line YAML tweak, doc typo           | Plain Claude Code (no GSD)      |
| "Add battery sensor" ad-hoc edit        | `/gsd-quick` or `/gsd-fast`     |
| Production-layer ESPHome build (multi-step, risky) | `/gsd-quick --full` *or* a real phase (`/gsd-discuss-phase` → `/gsd-plan-phase` → `/gsd-execute-phase`) |
| Debug a flaky deep-sleep wake           | `/gsd-debug`                    |
| New idea you're not sure about          | `/gsd-spike` (throwaway code)   |
| New milestone (v2 with MQTT? second device?) | `/gsd-new-milestone`        |

The big win for **this** project is the **production-layer build** — that's
multi-step, has real consequences (bricking the device or 6 hours of
re-flashing), and benefits from a written plan + atomic commits + a verifier
gate. Everything else is `/gsd-quick` territory.

---

## 3. Quick-start (5 minutes)

You already have GSD installed globally. To use it on this project:

```bash
cd /home/nuc8/01_smarthome/01_HomeAssistant/03_esphome/02_pool_watertemperature/esp32_lolin32_lite
claude --dangerously-skip-permissions   # avoid permission-prompt friction
```

Then in Claude:

```text
/gsd-help            # full command list (renamed: /gsd-do is gone, use /gsd-progress --do)
/gsd-progress        # GSD's status dashboard — works even on a non-GSD repo
```

If `/gsd-progress` says "no `.planning/` found", that's expected. Initialise
on a brownfield repo with:

```text
/gsd-map-codebase    # scans existing files, writes .planning/codebase/*.md
```

This gives future GSD sessions accurate intel about your YAML, your
firmware, your README. Cheap to run, run it once.

> **Don't run `/gsd-new-project`** here. That assumes a green-field project.
> Brownfield uses `/gsd-map-codebase` (+ optionally `/gsd-ingest-docs` for
> existing planning docs).

---

## 4. Mapping THIS project to GSD concepts

GSD concept   | What it maps to here
---           | ---
**Project**   | Pool water temperature monitor, battery-powered.
**Milestone** | "v1.0 — first production deployment". (You're not there yet — smoke-test only.)
**Phase**     | Coherent multi-task unit. *Open phase candidate:* "Production-layer ESPHome build" (Plan F: 3-h sleep, OTA every 56th boot).
**Plan**      | One atomic block of work. The production build splits into ~3 plans: (a) framework + safe_mode + secrets, (b) deep-sleep + boot counter + pinned DS18B20, (c) multi-AP + diagnostics.
**Task**      | One commit-sized unit inside a plan.
**STATE.md**  | Cross-session memory. **You already have `SESSION_CHECKPOINT.md`** doing this manually — see §8 on how to make them coexist.

---

## 5. Recommended workflow for the open items

The checkpoint lists 11 open items. Here's how to hit them with GSD:

### 5a. Hardware-only items → no GSD

Items 2 (desolder LED), 3 (JST pigtail), 5 (bench test), 11 (waterproofing)
are **physical actions**. GSD has nothing to add. Cross them off in
`SESSION_CHECKPOINT.md` as you do them.

### 5b. Trivial config tweaks → `/gsd-fast` or `/gsd-quick`

Items 7 (ADC calibration constant), 8 (real WiFi/MQTT credentials in
`secrets.yaml`), 9 (real `STATIC_IP`):

```text
/gsd-fast set ADC_CAL_FACTOR to 1.013 in firmware/include/config.h
/gsd-quick add wifi_ladi_ssid and wifi_ladi_password to secrets.yaml as multi-AP fallback
```

`/gsd-fast` = inline, no planning, no subagents. Use only when you've
verified the change in your head.
`/gsd-quick` = single GSD-managed task with atomic commit and basic
verification, no full phase ceremony.

### 5c. Production-layer ESPHome build → real phase

Item 1 in the latest checkpoint ("Deep-sleep + production layer — pick Plan
A/B/C/D/E/F"). This is multi-step, risky, and benefits from structure:

```text
# Step 1: capture the design decisions you've already half-made
/gsd-discuss-phase 1

# GSD will ask things like:
#   - Which plan? (you've recommended F)
#   - GPIO33 button or no?
#   - Boot counter cumulative or per-firmware?
#   - LED blink during OTA window?
# These are exactly the 5 blocking questions in the 2026-04-26 checkpoint.

# Step 2: research + plan
/gsd-plan-phase 1

# Step 3: execute (atomic commits per task)
/gsd-execute-phase 1

# Step 4: verify on hardware
/gsd-verify-work 1
```

Want to chain it without stopping at each gate?

```text
/gsd-discuss-phase 1 --chain     # auto-sequences into plan + execute
```

Or skip discuss/plan and trust GSD to manage just the execution:

```text
/gsd-quick --full implement plan F: 3-hour deep sleep, esp-idf framework, multi-AP, pinned DS18B20 ROM, safe_mode, captive_portal
```

For *this* project I'd recommend the explicit 4-step phase route. The risk
of a botched OTA bricking the device is real, and `/gsd-verify-work` is a
good speed bump.

### 5d. Specific-known-bug session → `/gsd-debug`

Items like "DMM cross-check disagrees with ADC" or "deep sleep not actually
sleeping (current too high)" should use `/gsd-debug` so the investigation
state survives across context resets.

```text
/gsd-debug deep-sleep current measures 4 mA instead of < 120 µA after LED desolder
```

---

## 6. Best practices for this project specifically

### 6.1. Pick a model profile and stick with it

```text
/gsd-settings
```

For this project recommend **`balanced`** (Opus plan, Sonnet execute).
Reasons:

- Plans for hardware/firmware are short — Opus's reasoning is worth it.
- Execution is mostly YAML/C++ edits — Sonnet handles them fine.
- Cost stays reasonable (you'll run a few cycles before the device is
  field-ready).

If a phase goes badly twice in a row, escalate one step:
`quality` profile (Opus everywhere) for one phase, then back down.

### 6.2. Keep `.planning/` in git, secrets out

Your `.gitignore` already excludes `secrets.yaml`. Good. **Add
`.planning/` to git** when you start using GSD — it's *the* point of the
tool. The checkpoint and decisions become part of repo history.

```bash
echo "" >> .gitignore
echo "# Keep .planning/ tracked — it's GSD's persistent memory" >> .gitignore
git add .planning/
git commit -m "Initialise GSD planning directory"
```

(Don't actually add a comment to `.gitignore` instructing you NOT to ignore
something — `.gitignore` doesn't allow negation that simply. Just don't add
`.planning/` to it.)

### 6.3. Accept that the source-of-truth ESPHome YAML is NOT in this repo

The actual `pooltemperature.yaml` lives inside the docker volume at
`/config/pooltemperature.yaml`. This repo has the **firmware/** PlatformIO
track + docs + factory binary, but the live ESPHome config is volume-only.

When GSD plans a task that edits ESPHome YAML, it must:

1. Back up first inside the container:
   `docker exec esphome cp /config/pooltemperature.yaml /config/pooltemperature.yaml.bak.$(date +%Y%m%d-%H%M%S)`
2. Edit via `docker exec -i esphome tee /config/pooltemperature.yaml < new.yaml`
3. Validate: `docker exec esphome esphome config /config/pooltemperature.yaml`
4. Then **mirror the validated YAML into this repo** (e.g. at
   `firmware/esphome/pooltemperature.yaml`) so the commit captures it.

If you want the repo to be the source of truth instead, mount the repo
directory into the docker container — but that's a bigger change; flag it
to GSD explicitly if you want it.

### 6.4. Critical-infra defaults (already in the esphome-expert skill)

Whenever GSD generates ESPHome YAML, it should pull from
`~/.claude/skills/esphome-expert/`. The skill already encodes the right
defaults: static IP, multi-AP, `safe_mode:` separate, esp-idf framework,
pinned DS18B20 address, `RESTORE_DEFAULT_OFF` for actuators. If a generated
plan skips any of those, ask GSD to re-plan citing the skill.

### 6.5. Validation gate on every ESPHome change

Add this to your `.planning/config.json` (after `/gsd-map-codebase` creates
the directory) as a project rule:

```json
{
  "verification": {
    "esphome_validate_required": true
  }
}
```

GSD respects custom verification keys via the verifier agent prompt. Or
just put a one-liner in `PROJECT.md`:

> Every plan that edits ESPHome YAML must run
> `docker exec esphome esphome config /config/<file>.yaml` as its
> verification step and fail if the output isn't `Configuration is valid!`.

---

## 7. Tips & tricks

### 7.1. Use `--minimal` install if you're token-budgeting

```bash
npx get-shit-done-cc@latest --claude --global --minimal
```

Drops the (now 59-skill, post-1.40 consolidation) full surface to 6
core ones (~700 tokens vs ~12k system-prompt overhead). For a project
this small, `--minimal` is plenty — you mostly need `new-project`,
`discuss-phase`, `plan-phase`, `execute-phase`, `help`, `update`.
Anything else (`/gsd-debug`, `/gsd-spike`) you can re-install on
demand. On Sonnet 4.6 / Opus 4.7 (1M context) the savings are mostly
academic; on local 32K–128K-context models, `--minimal` is the
difference between "works" and "won't fit".

> Caveat: `--minimal` removes some of the niceties listed in this guide
> (`/gsd-quick`, `/gsd-fast`, `/gsd-progress`). Worth it if context is tight.

### 7.2. Always pass `--minimal` again when updating

```bash
npx get-shit-done-cc@latest --claude --global --minimal
```

Without it, the updater puts all 86 skills back. The checkpoint already
notes you're on 1.39.1 — bump deliberately.

### 7.3. Enable auto-advance only after you trust the flow

```text
/gsd-settings
# set workflow.auto_advance = true
```

Skips the manual "OK now plan" / "OK now execute" gates. Useful once you
know GSD handles your style of work well. **Don't enable it on day one.**

### 7.4. `/gsd-explore` before committing to a major design choice

Before locking in Plan F vs Plan D (button-triggered OTA), spend 10 min in
`/gsd-explore`:

```text
/gsd-explore plan F (3h sleep, OTA every 56th boot) vs plan D (button-triggered OTA on GPIO33)
```

Throwaway Socratic dialogue, no commits. Better than a phase that commits
to F and then has to be re-done as D.

### 7.5. `/gsd-spike` for quick feasibility checks

Before adding `bluetooth_proxy:` (item parked in checkpoint), do:

```text
/gsd-spike does esp32_ble_tracker break the deep-sleep current budget on this device?
```

Spike answers the question with a throwaway YAML, doesn't pollute the
production track.

### 7.6. `/gsd-review` for cross-AI sanity check

Before flashing the production-layer YAML to the only NCR18650B-powered
prototype, run:

```text
/gsd-review
```

GSD will spawn a peer review against another AI CLI (Codex, Gemini, etc.)
on the latest plan. Catches "I'd never put `update_interval: 1s` on a
deep-sleep device" before it bricks the cell.

### 7.7. Atomic commits make rollback cheap

Each GSD task = one commit. If `/gsd-verify-work` finds the deep-sleep
current is 4 mA instead of < 120 µA, you can `git revert <task-commit>`
without unwinding the whole production phase. Lean on this.

### 7.8. Keep `secrets.yaml` audits in front of multi-AP changes

Per the 2026-04-26 entry, the previous multi-AP block referenced secrets
that didn't exist (`poolschacht_wifi_*`, `outdoor_wifi_*`). When GSD plans a
multi-AP task, ask it to **diff the planned `wifi:` block against
`secrets.yaml` keys** before validation — `esphome config` only catches
this if the typo is *syntactic*, not if you reference a non-existent secret
that just resolves to an empty string.

### 7.9. The dashboard is a third source of truth — beware

You have three places where YAML lives:

1. ESPHome dashboard at `http://<host>:6052` (writes to docker volume).
2. `docker exec` direct volume edits (also writes to volume).
3. The committed copy in this repo.

Pick one as authoritative. Recommended: the **repo copy** is canonical;
docker volume is a working copy; never touch the dashboard's "edit" button
on production firmware (it bypasses your validate→commit cycle).

### 7.10. The factory binary in the repo is convenience, not source

`pooltemperature.factory.bin` is committed for emergency re-flashing
without rebuilding. It's a *snapshot*, not source. Regenerate it
explicitly at the end of a production phase:

```bash
docker exec esphome esphome compile /config/pooltemperature.yaml
docker cp esphome:/config/.esphome/build/pooltemperature/.pioenvs/pooltemperature/firmware-factory.bin \
  ./pooltemperature.factory.bin
git add pooltemperature.factory.bin
git commit -m "build: refresh factory.bin from production-layer firmware"
```

---

## 8. `SESSION_CHECKPOINT.md` ↔ GSD `STATE.md`

You already maintain `SESSION_CHECKPOINT.md` manually (per the project's
auto-memory rule). GSD's `.planning/STATE.md` overlaps but isn't identical.

| File                        | Audience            | Format                | Updated by         |
| --------------------------- | ------------------- | --------------------- | ------------------ |
| `SESSION_CHECKPOINT.md`     | You + future Claude | Free-form prose       | Claude (auto)      |
| `.planning/STATE.md`        | GSD agents          | Structured (YAML-ish) | GSD commands       |
| `.planning/{N}-CONTEXT.md`  | GSD agents          | Decisions for phase N | `/gsd-discuss-phase` |
| `.planning/{N}-VERIFICATION.md` | You + GSD       | UAT results           | `/gsd-verify-work` |

**Recommended split:**

- Keep `SESSION_CHECKPOINT.md` as the **human narrative** — what you did,
  why, what's open in plain English. (It's already your habit; the auto-
  memory rule keeps it updated; keep it.)
- Let GSD write `.planning/STATE.md` for **structured decisions** when
  you start running phases. Don't hand-edit it.
- After each `/gsd-execute-phase`, append a one-paragraph summary to
  `SESSION_CHECKPOINT.md` linking to the phase docs:

  ```markdown
  ## 2026-05-XX — Phase 1 production-layer ESPHome build (GSD)

  Ran `/gsd-execute-phase 1`. Plans: see `.planning/01-*-PLAN.md`.
  Verification passed: deep-sleep current 95 µA, 18-month projection holds.
  Pinned DS18B20 ROM `0x28...`, framework now esp-idf 5.3.1, multi-AP active.
  ```

Don't duplicate the structured content. Link to it.

---

## 9. Anti-patterns to avoid in *this* project

1. **Running `/gsd-execute-phase` without backing up the docker-volume
   YAML first.** GSD's atomic commits don't help you if the broken file
   is in the docker volume and the only good copy was the volume's
   in-place edit.

2. **Treating the SESSION_CHECKPOINT entries as a substitute for
   `/gsd-discuss-phase`.** The checkpoint is prose narrative; discuss-
   phase extracts machine-readable decisions GSD's planner can act on.
   Both have value.

3. **Letting GSD generate ESPHome YAML without consulting the
   `esphome-expert` skill.** The skill knows about the LOLIN32 Lite
   pin gotchas (GPIO34–39 input-only, ADC2 + WiFi conflict, esp-idf as
   default). Generic ESPHome generation will miss them.

4. **Running `/gsd-fast` on anything that touches `secrets.yaml`,
   deep-sleep config, or `safe_mode`.** Those are the three places
   where a wrong commit costs you a serial-cable reflash. Use a real
   phase with verification.

5. **Skipping `/gsd-verify-work` on hardware tasks.** Software
   verification (`esphome config`, `esphome compile`) catches syntax;
   only flashing to the device and watching the log catches behavioural
   bugs (sensor never wakes, deep-sleep current too high, OTA window
   not actually 120 s).

6. **Running multiple workstreams against the single hardware
   prototype.** You have one device. `/gsd-workstreams create` makes
   sense for parallel features in software; here it just creates
   conflicts. Skip it.

7. **Putting `secrets.yaml` real values in `.planning/` docs.** GSD
   commits planning docs to git by default. Reference secrets by name
   (`!secret wifi_ssid`), never paste values into a plan.

8. **Trusting `/gsd-fast` to run `docker exec esphome esphome config`
   on its own.** `/gsd-fast` skips verification by design. Either
   manually validate after, or use `/gsd-quick` instead.

9. **Forgetting to bump `min_version:` and `project.version:` in the
   ESPHome YAML when GSD makes changes.** A plan that edits the YAML
   should treat `version: "1.0.0" → "1.1.0"` as part of the task,
   not an afterthought.

10. **Commit-spamming `pooltemperature.factory.bin` after every
    phase.** It's a 935 KB binary. Update it once per
    `/gsd-complete-milestone`, not once per execute-phase.

---

## 10. Cheat-sheet — the commands you'll actually use

```text
# Status & navigation
/gsd-help                       # full command list
/gsd-progress                   # what GSD thinks you should do next
/gsd-progress --do              # do the suggested next step right now (replaces deleted /gsd-do)
/gsd-progress --next            # peek at what /gsd-progress would do (replaces deleted /gsd-next)
/gsd-settings                   # model profile, workflow toggles, git strategy
/gsd-health --context           # context-utilization quality guard (60% warn, 70% critical)

# Namespace routers (v1.40+) — useful when you don't remember the exact command
/gsd-workflow                   # → discuss / plan / execute / verify / phase / progress
/gsd-project                    # → milestones, audits, summary
/gsd-quality                    # → code review, debug, audit, security, eval, ui
/gsd-context                    # → map, graphify, docs, learnings
/gsd-manage                     # → config, workspace, workstreams, thread, update, ship, inbox
/gsd-ideate                     # → explore, sketch, spike, spec, capture

# Brownfield setup (run once)
/gsd-map-codebase               # ingest existing code into .planning/codebase/
/gsd-ingest-docs                # if you want to formalise SESSION_CHECKPOINT/etc.

# The phase loop (the core 4)
/gsd-discuss-phase N            # capture decisions before planning
/gsd-plan-phase N               # research + atomic plans
/gsd-plan-phase --research-phase N   # research-only mode (replaces deleted /gsd-research-phase)
/gsd-plan-review-convergence N  # cross-AI replan-until-no-HIGH-concerns loop
/gsd-execute-phase N            # run plans, atomic commits
/gsd-execute-phase N --wave M   # run only wave M (useful for hardware-staged rollout)
/gsd-verify-work N              # UAT after execution
/gsd-edit-phase N               # change a phase field without renumbering

# Ad-hoc
/gsd-quick "<task>"             # single managed task, atomic commit, basic verify
/gsd-fast "<task>"              # inline, no planning, no verify (use carefully)

# Investigation
/gsd-debug "<bug>"              # state-persistent debugging session
/gsd-spike "<idea>"             # throwaway feasibility check
/gsd-explore "<topic>"          # Socratic dialogue, no commits

# Quality gates
/gsd-code-review                # review a phase's diff
/gsd-code-review --fix          # …and apply fixes (replaces deleted /gsd-code-review-fix)

# When ready to ship
/gsd-ship N                     # create PR from verified phase
/gsd-complete-milestone         # archive milestone, tag release
```

Forget the rest until you actually need them.

---

## 11. First session with GSD on this project — concrete script

Tomorrow morning, when you sit down to do the production-layer build:

```bash
cd /home/nuc8/01_smarthome/01_HomeAssistant/03_esphome/02_pool_watertemperature/esp32_lolin32_lite
claude --dangerously-skip-permissions
```

Then in Claude:

```text
1. /gsd-map-codebase
   # Wait for completion. Inspect .planning/codebase/*.md briefly to
   # confirm GSD understood the project layout.

2. /gsd-discuss-phase 1
   # Title the phase: "Production-layer ESPHome build (Plan F)"
   # Answer GSD's questions with the answers from the 2026-04-26 checkpoint:
   #   Plan F. No GPIO33 button. Cumulative boot counter. No LED blink.
   #   3.2 V cutoff. Multi-AP: wifi_ssid + wifi_ladi_* + wifi_mobile_wpl_*.

3. /gsd-plan-phase 1
   # Review the 2-3 plans GSD generates. Reject any that don't reference
   # the esphome-expert skill defaults (esp-idf, safe_mode, pinned DS18B20).

4. /gsd-execute-phase 1
   # Watch the atomic commits land. Each plan = ~3-5 commits.

5. # OTA-flash the result manually (or have GSD do it):
   docker exec esphome esphome run /config/pooltemperature.yaml --device 192.168.1.86

6. /gsd-verify-work 1
   # Confirm: device boots, joins WiFi, reads DS18B20, publishes to HA,
   # deep-sleeps within 8 s, current draw < 120 µA on DMM.

7. # Append a paragraph to SESSION_CHECKPOINT.md.
```

If everything goes well, that's one productive 90-minute session — and the
device is ready for waterproofing and pool deployment.

---

## 12. Short example — starting *Pool Temperature Monitor v1* from a clean slate

> Use this section when somebody asks "how do I start a small project
> like ours with GSD?". The brownfield path lives in §3–§5; this is
> the *green-field* version of the same project, condensed to the
> minimum viable command flow. Useful as a teaching script.

### 12.1. The brief (paste this verbatim into `/gsd-new-project`)

```
Project: Pool Temperature Monitor

A battery-powered ESP32 (LOLIN32 Lite) that measures pool water
temperature once an hour using a DS18B20 one-wire sensor and
publishes it to Home Assistant via the ESPHome Native API over
WiFi.

Power: a single 18650 cell (NCR18650B, 4.2 V freshly charged,
verified with a DMM). No second cell, no solar, no USB.

Cycle: wake from deep sleep → DS18B20 conversion → WiFi
associate → publish to HA → deep-sleep again. Wall-clock target
≤ 10 s awake per cycle.

OTA without extra hardware: no GPIO button. Use a persistent
boot counter; every 10th boot the device stays awake for
2 minutes with the OTA service exposed. A "boot" is any cold
start — RST button, battery replacement, or the HA
software-restart-button.

Hard requirement: 9 months on a single charge. If the energy
budget cannot meet that at 1 h cadence, fall back to 2 h cadence
and document the decision in PROJECT.md.

Out of scope for v1.0: bluetooth_proxy, pH/chlorine sensors,
non-HA dashboards, automated waterproofing.
```

### 12.2. The five-command flow

```text
1. /gsd-new-project            # paste the brief above. GSD writes
                               # PROJECT.md, REQUIREMENTS.md, ROADMAP.md, STATE.md.

2. /gsd-discuss-phase 1        # answer the gray-area questions (§12.3).

3. /gsd-plan-phase 1           # research + 2-3 plans, atomic commits.
                               # Reject any plan that doesn't show the energy math (§12.4).

4. /gsd-execute-phase 1        # parallel-wave execute. Each task = one commit.

5. /gsd-verify-work 1          # UAT on the bench (§12.5).

# Ship
/gsd-ship 1                    # PR from the verified phase.
/gsd-complete-milestone        # tag v1.0.
```

That's the whole loop. ~90 minutes of attention split across 1–3
sessions, plus ~7 days of unattended battery-longevity bench-time
between step 5 and `/gsd-ship`.

### 12.3. Discuss-phase answers ready in advance

GSD will ask roughly these questions; here are the right answers for
this brief so you can power through:

| Question                          | Answer                                                                 |
| --------------------------------- | ---------------------------------------------------------------------- |
| OTA trigger?                      | Boot-counter modulo 10; 120 s awake window with OTA service exposed.   |
| What advances the counter?        | Every cold boot — RST button, battery swap, HA software-restart-button.|
| Where is the counter persisted?   | NVS (esp-idf nvs partition); survives deep sleep and power cycles.     |
| Cadence?                          | 1 h primary; fall back to 2 h iff §12.4 math fails.                    |
| Wake budget?                      | ≤ 10 s, including ≤ 750 ms DS18B20 conversion.                         |
| Framework?                        | esp-idf (lower deep-sleep current than Arduino on LOLIN32 Lite).       |
| Multi-AP fallback?                | Yes, SSIDs from `secrets.yaml`.                                        |
| DS18B20 address?                  | Pinned (read once during smoke-test, hard-coded in YAML).              |
| `safe_mode:`?                     | Enabled.                                                               |
| Battery cutoff?                   | 3.2 V — below this, skip the publish, sleep through (avoid cell damage).|
| Time source?                      | HA Native API on each wake (no RTC needed; HA's clock is good enough). |
| Reporting on battery?             | Yes — `voltage` + `boot_counter` + `wake_duration_ms` per cycle.       |

### 12.4. Energy math GSD must include in the plan

Reject any plan that doesn't show numbers in this shape:

```
Cell:         NCR18650B ≈ 3 200 mAh @ 3.6 V nominal
Awake/cycle:  10 s × 80 mA  = 0.222 mAh
Sleep:        120 µA × 24 h = 2.88 mAh/day

1 h cadence:  24 × 0.222 + 2.88 ≈ 8.21 mAh/day
              3 200 / 8.21 ≈ 390 days ≈ 12.8 months  → PASS

2 h fallback: 12 × 0.222 + 2.88 ≈ 5.54 mAh/day
              3 200 / 5.54 ≈ 577 days ≈ 19 months    → PASS with margin
```

If empirical sleep current measures > 200 µA, re-do the math; if
1 h still fits, stay; otherwise drop to 2 h and edit `PROJECT.md`
plus `REQUIREMENTS.md` accordingly (this is a `/gsd-edit-phase` move,
not a code change).

### 12.5. Verify checklist (after `/gsd-execute-phase 1`)

```text
[ ] docker exec esphome esphome config /config/pooltemperature.yaml
    →  "Configuration is valid!"
[ ] First flash: device boots, joins WiFi, reads DS18B20 within 10 s,
    publishes to HA, then enters deep sleep.
[ ] Force OTA window: tap RST 9 more times so boot 10 lands.
    Device stays online ≥ 120 s. Push a no-op YAML change OTA → succeeds.
[ ] DMM in series during deep sleep: ≤ 120 µA (target), ≤ 200 µA (acceptable).
[ ] HA shows boot_counter incrementing across each wake.
[ ] 7-day bench run → linear-extrapolate cell drop. Must project ≥ 9 months
    before /gsd-ship.
```

### 12.6. If verify fails

```text
/gsd-debug "deep-sleep current measures 4 mA, expected ≤ 120 µA after first wake"
```

`/gsd-debug` persists state across context resets, so a multi-day
hardware bug-hunt doesn't lose the thread.

### 12.7. Things to deliberately *not* do in v1

- **Don't** add a button-on-GPIO33 OTA trigger — the user explicitly
  rejected hardware additions; the boot counter is the contract.
- **Don't** wake the device for "diagnostics" between scheduled wakes;
  diagnostics are folded into the same 10 s budget.
- **Don't** add `bluetooth_proxy:`. It alone breaks the energy budget.
- **Don't** commit `secrets.yaml` real values into `.planning/`.
- **Don't** ship before the 7-day bench projection.

Capture the rejected ideas with `/gsd-capture` so they land cleanly in
v1.1's milestone backlog.

---

## 13. Further reading

- Upstream GSD repo: <https://github.com/gsd-build/get-shit-done>
- Local install location: `~/.claude/skills/gsd-*`
- ESPHome expert skill: `~/.claude/skills/esphome-expert/SKILL.md`
- Project-specific memory:
  `~/.claude/projects/-home-nuc8-01-smarthome-01-HomeAssistant-03-esphome-02-pool-watertemperature-esp32-lolin32-lite/memory/`
- Project narrative: `SESSION_CHECKPOINT.md` (this directory).
- German build doc: `poolmeter_DE.md`.

---

*Last updated: 2026-05-09. Bump when GSD or the project changes
substantially. This revision: refreshed for upstream GSD v1.40
(namespace routers), v1.41 (post-merge build/test gate), and v1.51
(slopcheck Package Legitimacy Gate); added §12 green-field starter
example for the 1-h-cadence / 10th-boot-OTA / 9-month-battery brief.*
