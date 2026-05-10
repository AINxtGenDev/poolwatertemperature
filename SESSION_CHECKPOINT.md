# Session Checkpoint — Pool Temperature Monitor

**Last session:** 2026-05-10 (afternoon)
**Working dir:** `/home/nuc8/01_smarthome/01_HomeAssistant/03_esphome/02_pool_watertemperature/esp32_lolin32_lite`

## 2026-05-10 (afternoon) — v2.0.1 hotfix: safe_mode rollback resolved; battery reading still TBD

### What happened today (after morning v2.0.0 flash)

Flashed v2.0.0 → device booted, then **safe_mode auto-rolled back to the
previous always-on firmware** within minutes. Wireless log captured the
smoking gun:

```
[W][safe_mode:058]: OTA rollback detected! Rolled back from partition 'app0'
[W][safe_mode:059]: The device reset before the boot was marked successful
```

Confirming clues in the log:
- Sensor update intervals showed `10.0s` / `60.0s` (smoke-test values),
  not v2.0.0's `never`
- No `deep_sleep:` block in the component setup list
- `safe_mode: Successful after: 60s` was active

### Root cause (my bug in v2.0.0)

`safe_mode.boot_is_good_after: 1min` (60 s) is longer than the awake
window (~5–10 s). Every normal `deep_sleep.enter` looked to safe_mode
like "device reset before boot marked successful." After
`num_attempts: 10` such "fast reboots," safe_mode triggered partition
rollback. v2.0.0 never had a chance to stabilise.

Classic safe_mode ↔ deep_sleep interaction trap. **Generalisable
ESPHome rule worth remembering:** for any `deep_sleep` device,
`safe_mode.boot_is_good_after` MUST be shorter than the awake window
or rollback fires.

### Fix (v2.0.1, commit `51d3fc1`)

```yaml
safe_mode:
  num_attempts: 10
  reboot_timeout: 5min
  boot_is_good_after: 1s   # was 1min — must be < awake window
```

ESPHome setup completes in ~500 ms (verified from log). 1 s timer
fires at ~1.5 s after boot. Earliest possible sleep is ~4.1 s
(delays in script alone sum to 3.5 s + sensor reads + wait_until). 
Margin: 2.6 s.

Also bumped `project.version` `2.0.0` → `2.0.1` and updated the
header comment to match (user spotted the comment was stale).

### Verification (post-flash)

User flashed v2.0.1. Log captured:
```
[I][app:156]: Project homelab.pooltemperature version 2.0.1
[I][deep_sleep:057]: Beginning sleep
[I][deep_sleep:059]: Sleeping for 7200000000us   ← 2 h, correct
INFO Processing expected disconnect from ESPHome API
```

✓ v2.0.1 running (no rollback)
✓ deep_sleep component active (was missing in v2.0.0 log)
✓ 2 h cadence confirmed
✓ API disconnect = device asleep

The 11.246 s "Successfully connected" delay before the boot log
appeared is normal — log tool reconnect lag after OTA. The "missing"
`Real reboot — counter now N` and `Awake — waiting for API` lines
happened during that gap, before the log connection re-established.

### Open issue picked up at end of session — battery reading wrong

User reported "battery level and battery voltage does not be correct"
after v2.0.1 deployed. Showed me their old (working) ADC config from
the smoke-test build. **Could not finish diagnosing today — three
questions sent, awaiting answers tomorrow:**

1. What value is HA actually showing for `Battery Voltage`?
   - If ~ballpark cell voltage but jumpy → software (multi-read in
     script, average within wake)
   - If ~0.18 / 0.37 V or wildly off → hardware (no cell, divider
     unsoldered, 1N5817 in pigtail blocking path, GPIO35 wire loose)
2. Is a real charged NCR18650B in the JST pigtail, or USB-only test?
   (USB-only with no cell → BAT floats → divider reads garbage; was
   the symptom in the v2.0.0 log too: `Voltage=0.1840V`.)
3. Multimeter reading at GPIO35 pin → should be ½ × Vcell ≈ 2 V for
   a 4 V cell.

**Important for next session — why I did NOT just port the old
filters:** in deep-sleep mode, `sliding_window_moving_average:
window_size: 5, send_every: 5` is actively WORSE than v2.0.1's
single-shot read. With `update_interval: never` and one reading per
wake, the filter would accumulate one reading per 2 h → publish only
every 10 h with samples 0.5 h apart. Internal `samples: 16` already
provides per-call averaging. The right deep-sleep equivalent is to
take multiple `component.update: vbat_raw` calls within a single
wake (with small delays) and let the filter average them — but only
worth doing if the readings are noisy. If they're flat-wrong (~0.18 V),
no amount of averaging fixes it.

### State at end of session

- v2.0.1 flashed, sleeping, on `Outdoor_AP` priority WiFi (or Aruba
  fallback). Static IP `192.168.1.86`. Next wake ~2 h after each
  sleep entry.
- Container `/config/pooltemperature.yaml` byte-identical to host
  repo (verified via `docker exec -i esphome diff /config/... -`).
- Git: clean except for pre-existing modifications to `00-prompt.txt`
  and `HOW-TO-USE-GSD.md` (uncommitted, predate this session, NOT
  touched in any of today's work).
- Hardware: battery wiring needs validation (likely USB-only test
  causing the 0.18 V reading; confirm with a real cell).

### Resume tomorrow with

1. Answer the three diagnostic questions above (HA value, real cell
   connected y/n, multimeter at GPIO35).
2. Based on answer:
   - **Hardware path:** check 1 MΩ resistors, 1N5817 orientation, JST
     continuity. No firmware change needed.
   - **Software path (only if hardware is fine):** add a 200 ms
     settling delay before `component.update: vbat_raw`, and/or take
     3–5 readings 100 ms apart within the wake script and average.
3. Once battery reading is correct, exercise the OTA recovery paths
   on the bench:
   - 10× RST → expect `OTA window: 10-th real reboot` and 2 min
     stay-awake
   - HA action `esphome.pooltemperature_enter_ota_mode` → expect
     `OTA window: HA-requested via enter_ota_mode` on next wake
4. Field deployment to the pool when bench tests pass and 1N5817 +
   100 nF caps are soldered per the BOM.



## 2026-05-10 — Production-rewrite plan delivered (no code yet)

Reviewed the current `pooltemperature.yaml` (smoke-test build: arduino
framework, single SSID `Aruba_HPE_WLAN`, always-on, GPIO16 DS18B20
power switch wired but `ALWAYS_ON`, no deep_sleep, no safe_mode, no
fast_connect, no min_version, linear 4.2→3.2 V battery curve) against
the ESPHome production checklist and the user's brief.

Delivered as a structured plan (no YAML written this turn):

1. **Battery feasibility analysis.** Lolin32 Lite ~150 µA sleep,
   NCR18650B usable ~3000 mAh above 3.2 V cutoff. 1-hour interval is
   *plausible* (1875–2680 mAh / 9 mo) under optimistic-to-realistic
   assumptions, but *fails* (~4000 mAh) if awake window stretches to
   10 s with 150 mA average. 2-hour fallback is comfortably safe
   (~2000 mAh). Plan: build for 1 h, measure for a week, fall back
   to 2 h if real consumption projects > 2700 mAh / 9 mo.

2. **Reliability pillars to add.** `min_version`, `safe_mode` as
   separate component, `fast_connect: true`, BSSID pinning,
   `output_power: 17dB`, multi-AP fallback, NCR18650B 5-point curve
   (4.1→3.2 V, linear-spaced), DS18B20 address pinning, diagnostic
   sensors (RSSI, reset_reason, reboot_count, version), sensors
   moved to `update_interval: never` + script-driven read in
   `on_boot`. Skip captive_portal on battery device.

3. **OTA recovery — three layered paths:**
   - **NVS-persisted reboot counter** (`globals` with
     `restore_value: true`) survives battery swap. Differentiates
     `ESP_RST_DEEPSLEEP` (don't increment) from
     `ESP_RST_POWERON/SW/PANIC/BROWNOUT` (increment). Every 10th
     real reboot → skip deep sleep, stay awake 2 min for OTA.
   - **HA-callable action `enter_ota_mode`** sets a one-shot flag
     for next-wake OTA window — convenient routine path.
   - **`safe_mode`** independent crash-loop recovery — bad OTA that
     crashes within 1 min auto-boots into permanent OTA listener.

4. **Phased implementation, atomic commits, verifiable per phase:**
   (1) pin DS18B20 address; (2) reliability scaffolding still
   always-on; (3) framework switch arduino → esp-idf; (4) NCR
   battery curve; (5) reboot counter + OTA window; (6) HA action
   `enter_ota_mode`; (7) deep_sleep enable + script-driven publish;
   (8) one-week field measurement → keep 1 h or fall back to 2 h.

5. **Open questions surfaced for user before any code:**
   - Which AP is in range at the pool location? Current config uses
     `Aruba_HPE_WLAN` (house). Likely candidates from secrets:
     `Outdoor_AP`, `Poolschacht`. Battery life depends critically
     on RSSI / time-to-associate.
   - Framework switch arduino → esp-idf? Recommended for stability
     and power on deep-sleep cycles, but bigger change. User to
     confirm.

Status: plan delivered, awaiting user reply on AP choice + framework
preference. No files modified except this checkpoint.

### 2026-05-10 (later) — User locked in two inputs

- **Cadence committed to 2 hours** (skip the "1 h then measure" trial
  step). Battery budget now ~2000 mAh / 9 months under realistic
  assumptions — comfortably below 3000 mAh usable. No fallback step
  needed.
- **DS18B20 address pinned: `0x8448695309646128`** (DS18B20 on the
  GPIO4 1-Wire bus, captured from current firmware boot log).

Phase plan collapses from 8 steps to 7 (drop former phase 8
field-measurement decision). Still awaiting AP choice + framework
decision; defaults proposed: keep `Aruba_HPE_WLAN` primary +
`Outdoor_AP` fallback, stay on `arduino` (esp-idf less compelling
once cadence dropped to 2 h).

### 2026-05-10 (later) — All decisions locked + production rewrite committed

User confirmed:
- **Primary AP: `Outdoor_AP` (priority 10)**, fallback
  `Aruba_HPE_WLAN` (priority 5). Both on same `192.168.1.0/24`,
  static IP `192.168.1.86` retained.
- Framework: stayed on `arduino` (no objection raised to default).

Wrote production `pooltemperature.yaml` (v2.0.0) replacing the
smoke-test build. Single comprehensive rewrite, ESPHome 2026.4.5
validated `Configuration is valid!` after one fix:

- **Hit 2026.4.x deprecation:** `num_attempts:` is no longer valid
  on the OTA platform — moved entirely to the `safe_mode:` component.
  Removed it from `ota:` (kept on `safe_mode:` where it now lives).

What's in v2.0.0 (vs smoke-test build):
- `min_version: 2026.4.0`, `project.version: "2.0.0"`
- `safe_mode:` as separate top-level component
- WiFi `networks:` list (Outdoor_AP/Aruba), `fast_connect: true`,
  `output_power: 17.0`, `wifi.reboot_timeout: 2min`,
  `enable_btm/rrm: true`. No `captive_portal` (battery device).
- DS18B20 address pinned to `0x8448695309646128`,
  `update_interval: never`, triggered from script.
- NCR18650B 5-point linear SOC curve (4.1 V → 100 %, 3.2 V → 0 %).
- Globals (NVS-persisted, survive battery swap):
  `reboot_count: uint32_t`, `ota_request_pending: bool`.
- API action `enter_ota_mode` — HA-callable, sets one-shot OTA flag.
- `deep_sleep:` (id `ds`, sleep_duration 2h, no run_duration —
  manual control via script).
- Two scripts:
  - `read_and_publish` (mode: single) — wait_until api.connected
    timeout 30 s → component.update on temp+vbat → 1.5 s wait
    (DS18B20 conversion) → update derived sensors → 2 s drain →
    deep_sleep.enter
  - `ota_window` (mode: single) — same publish flow then
    `delay: 2min` then deep_sleep.enter
- `on_boot` priority 800: increment `reboot_count` IFF
  `esp_reset_reason() != ESP_RST_DEEPSLEEP` (deep-sleep wakes don't
  count).
- `on_boot` priority -100: dispatcher — if
  `(reboot_count > 0 && reboot_count % 10 == 0) ||
   ota_request_pending` → consume flag, run `ota_window`; else run
  `read_and_publish`.
- OTA platform: `on_begin: deep_sleep.prevent`,
  `on_error: deep_sleep.allow` (prevent bricking mid-upload).
- Diagnostic sensors: wifi_signal, uptime, reset_reason text,
  reboot_count, wakes_to_ota, ESPHome version, wifi_info trio.

NOT done (left to user):
- `git commit` — file is uncommitted in working tree.
- Flash to device. Recommended: OTA from current always-on smoke
  firmware (one-shot transition). After sleep, OTA only possible
  during 10-min/HA-triggered windows.
- Bench-test counter logic before deploying to pool: 10× RST
  should open a 2-min OTA window; HA action `enter_ota_mode`
  should open one on next wake.

Container backup: `/config/pooltemperature.yaml.bak.<timestamp>`.

## 2026-05-09 — `HOW-TO-USE-GSD.md` refreshed for upstream GSD v1.40/1.41/1.51

Updated the project's GSD how-to in place:

- Folded in v1.40's six namespace meta-skills (`/gsd-workflow`,
  `/gsd-project`, `/gsd-quality`, `/gsd-context`, `/gsd-manage`,
  `/gsd-ideate`) — now the recommended router-style entry points.
- Replaced references to deleted micro-skills:
  `/gsd-research-phase` → `/gsd-plan-phase --research-phase N`,
  `/gsd-do` / `/gsd-next` → `/gsd-progress --do` / `--next`,
  `/gsd-code-review-fix` → `/gsd-code-review --fix`.
- Added `/gsd-edit-phase`, `/gsd-plan-review-convergence`,
  `/gsd-execute-phase --wave N`, and `/gsd-health --context` to the
  cheat-sheet (§10).
- Skill-surface count corrected from "86" to "59 (post-1.40
  consolidation)" in the `--minimal` install paragraph.
- Mentioned v1.51's *Package Legitimacy Gate* (slopcheck) — mostly
  irrelevant for firmware-only phases here, but flagged so it doesn't
  surprise us if a future phase pulls a Python helper.
- New §12: a green-field starter walkthrough for "Pool Temperature
  Monitor v1" with the user-stated brief baked in (1 h cadence,
  10th-boot OTA window, no GPIO button, NVS-persisted boot counter,
  9-month battery hard requirement, 2 h fallback if math fails). Also
  includes the energy-budget math GSD's planner must include and a
  7-step UAT checklist for `/gsd-verify-work`.

§11's older "first session" script (Plan F: 3 h sleep, OTA every 56th
boot) was kept *as-is* — it captures the originally-discussed plan
that was carried in this checkpoint. The new §12 captures the
**revised** brief from 2026-05-09 (1 h sleep, 10th-boot OTA). If we
commit to the §12 brief, archive §11 by `/gsd-edit-phase`-ing the
roadmap and updating PROJECT.md to match.

## 2026-05-02 (later) — `HOW-TO-USE-GSD.md` added

User asked for a structured, project-tailored guide on using GSD
(<https://github.com/gsd-build/get-shit-done>). Wrote
`HOW-TO-USE-GSD.md` at the repo root.

Key points the guide bakes in for future sessions in this project:

- **Brownfield entry point is `/gsd-map-codebase`**, not
  `/gsd-new-project`. The latter assumes greenfield.
- The **production-layer ESPHome build** (Plan F: 3-h sleep, OTA every
  56th boot, esp-idf framework, multi-AP, pinned DS18B20 ROM,
  safe_mode, captive_portal) is the one place a real GSD phase pays
  off. Everything else in the open-items list is hardware steps,
  trivial config edits (→ `/gsd-fast` / `/gsd-quick`), or debugging
  (→ `/gsd-debug`).
- Source-of-truth ESPHome YAML lives in the **docker volume**
  (`/config/pooltemperature.yaml`), not in this repo. GSD plans that
  edit YAML must (1) back up inside container, (2) write via
  `docker exec -i esphome tee`, (3) validate with
  `docker exec esphome esphome config`, (4) mirror the validated copy
  into the repo for the commit.
- `SESSION_CHECKPOINT.md` (human narrative, this file) and
  `.planning/STATE.md` (GSD machine-readable state) coexist by
  splitting roles: SESSION_CHECKPOINT for prose summary, GSD docs
  for structured decisions. After each `/gsd-execute-phase`, append a
  one-paragraph link-and-summary entry here. Don't duplicate content.
- Recommended model profile for this project is **`balanced`** (Opus
  plan, Sonnet execute) — escalate to `quality` only if a phase fails
  twice.
- `--minimal` install (~700 system-prompt tokens vs ~12k full) is
  enough for this project. Pass `--minimal` again on every update or
  the full skill set comes back.
- A **concrete first-session script** (§11 of the guide) walks the
  open production-layer phase: `map-codebase` → `discuss-phase 1` →
  `plan-phase 1` → `execute-phase 1` → OTA-flash → `verify-work 1` →
  append SESSION_CHECKPOINT entry. Pre-fills the discuss-phase
  answers from the 5 blocking questions in the 2026-04-26 entry.

10 project-specific anti-patterns documented (don't `/gsd-fast` on
secrets/safe_mode/deep-sleep changes; don't run `/gsd-execute-phase`
without backing up the volume YAML; don't put real secret values in
planning docs since `.planning/` is committed; etc.).

No code or YAML changed this turn — single-file documentation write.

## 2026-05-02 — Repo published to GitHub; toolchain bump

**Git sync — first push to public remote.**
Project directory was not yet a git repo. Initialized and synced with
`git@github.com:AINxtGenDev/poolwatertemperature` (PUBLIC).

- `.gitignore` written before any `git add`. Excludes `secrets.yaml`
  (contained real WiFi/OTA passwords + 5 ESPHome API encryption keys for
  the wider fleet — never reached the index, verified via
  `git check-ignore` and `git ls-files`), plus `*.bak`, PlatformIO build
  dirs (`firmware/.pio*`), ESPHome build artifacts (`.esphome/`, `build/`),
  HA `.storage/`, Python venvs.
- Local repo configured with `core.sshCommand = ssh -i
  /home/nuc8/.ssh/github_werner -o IdentitiesOnly=yes`, so future
  push/pull from this directory uses the right key automatically (no
  global `~/.ssh/config` change needed).
- 16 files + 4.7 MB pushed across 2 commits on top of the remote's
  initial stub. Includes `pooltemperature.factory.bin` (~935 KB
  pre-built firmware blob — kept on purpose so the repo is flashable
  without rebuilding; reconsider if commit history grows).
- Conflict resolved on `README.md`: kept the local 308-line documentation
  over the remote's 45-byte stub. Remote's `LICENSE` preserved.
- **First push was rejected** by GitHub (`GH007: push would publish a
  private email address`) because the `AINxtGenDev` GitHub account has
  email-privacy enabled. Resolved without changing the GitHub setting:
  rewrote both new commits via `git filter-branch --env-filter` to use
  `200944911+AINxtGenDev@users.noreply.github.com` (author name
  `AINxtGenDev`). The local `user.email` is now this noreply too — keep
  using it for any future commits in this repo.

**Toolchain — GSD updated 1.38.5 → 1.39.1.**
Hotfix release. Most relevant fix for this project: `gsd-sdk query
agent-skills` now emits raw text instead of a JSON-quoted string, so
subagents spawned in this dir will get correct `<agent_skills>` blocks.
Also: `/gsd-help` no longer advertises 8 commands removed in #2824
(skill consolidation) — `/gsd-do` is now `/gsd-progress --do`,
`/gsd-note` is `/gsd-capture --note`, `/gsd-check-todos` is
`/gsd-capture --list`, etc. Several skills are now namespaced under
`gsd-ns-*` parents (context, project, ideate, manage, review, workflow).

**Authoring note for this file.**
A previous session pasted a verbatim `<system-reminder>` block into this
checkpoint (likely a copy artifact from the runtime). It was already
edited out before this session started — flagged here so future-Claude
doesn't try to "act on" reminders that appear in the file.

## 2026-04-28 — Battery monitoring added to smoke-test yaml

Layered the GPIO35 battery sensors onto the smoke-test build (still arduino
framework, still no deep sleep — those are the next-session step). All other
production-layer items from the previous session remain open.

### What was done

- Backed up the previous `/config/pooltemperature.yaml` inside the docker
  container as `pooltemperature.yaml.bak.20260428-074706`.
- Added two sensors to the existing `sensor:` block:
  - `vbat_raw` — ADC on GPIO35, `attenuation: 12db`, `samples: 10`,
    `sampling_mode: avg`, filters `multiply: 2.0` → `sliding_window_moving_average`
    (window 5) → `heartbeat: 300s`. `device_class: voltage`,
    `entity_category: diagnostic`. No manual `ADC_CAL_FACTOR` — ESP32 returns
    calibrated mV since 2021.11.
  - `vbat_pct` — template sensor, linear NCR18650B curve (3.2 V → 0 %,
    4.2 V → 100 %), `NaN` passthrough when raw is unavailable, `device_class:
    battery` (visible in main HA UI, not diagnostic).
- Validated with `docker exec esphome esphome config /config/pooltemperature.yaml`
  → `Configuration is valid!` on ESPHome 2026.4.2.

### Verified on hardware (after OTA-flash to 192.168.1.86)

- Both new entities appeared in HA:
  `sensor.pooltemperature_pooltemperature_battery_voltage` and
  `sensor.pooltemperature_pooltemperature_battery_level` (entity_id has the
  device-name prefix doubled because both `esphome.name` and the sensor name
  start with "Pooltemperature" — cosmetic, not fixed).
- HA also auto-displays a **47 % battery icon in the ESPHome integration
  header** (HA picks up `device_class: battery` automatically).
- Linear curve checks out: 3,657 V → expected 45,7 % → HA shows 47 %
  (rounding + small ADC noise). No `calibrate_linear` filter needed.
- Voltage drop observed: 3,870 V → 3,657 V in ~10 min under continuous WiFi
  load. Expected — smoke-test build runs with no deep sleep, ~80–100 mA
  continuous draw → cell empty in ~34 h. **Not a defect**, just confirms why
  the production-layer Plan F (deep-sleep, 3-h cycle) is mandatory before
  field deployment.

### Quirk: "Unknown" for the first 5 min after each boot

The `vbat_raw` ADC sensor uses `sliding_window_moving_average(window=5,
send_every=5)`. First publish happens at t = 5×60 s = 5 min after boot. Until
then `id(vbat_raw).state` is `NaN`, and the template lambda returns `NAN`,
which HA renders as "Unknown" / no value. After the first sliding-window
publish lands, the percentage updates every 60 s and stays valid.

If this becomes annoying on field deployments, change the lambda to
`return {};` (skip publish on NaN) instead of `return NAN;` — that leaves the
last-known good value visible in HA instead of going Unknown. For deep-sleep
builds it doesn't matter — the sliding window never fills inside a 30-s wake.

### USB charging via the onboard TP4054 — does NOT work as wired

The LOLIN32 Lite has a TP4054 LiPo charger on board, but the **1N5817
reverse-polarity Schottky in the JST pigtail blocks the charge current**
(diode passes only cell→board, not board→cell). User decided to keep the
diode for safety and use an **external 18650 charger** (recommended:
Liitokala Lii-100, ~€10, 1 A CC/CV) — Plan F's expected ~18-month interval
makes the swap-and-charge workflow a non-issue.

Two stocked NCR18650B cells (one in device, one charged in spare) plus a
HA push-notification at `Battery Level < 15 %` is the agreed maintenance
loop. Three options were evaluated and parked for later if charging
becomes inconvenient: (a) keep external charger workflow [chosen],
(b) remove 1N5817 — risky if Keystone holder isn't keyed, (c) replace with
AO3401 P-MOSFET reverse protection — passes both directions, ~30 mV drop.

### Still open

1. **Deep-sleep + production layer** — pick Plan A/B/C/D/E/F (recommend F:
   3-h cycle, OTA every 56th boot ≈ weekly, 4-s tight wakes → ~18 months on
   one NCR18650B). User has not yet answered the 5 blocking questions
   below this section in the 2026-04-26 entry.
2. **Switch framework to esp-idf** when production yaml is built.
3. **DS18B20 ROM-pin** — read from boot log, paste into `address:`.
4. **DMM cross-check of battery voltage** still pending; current ADC
   reading agrees with linear curve so no calibration filter has been
   added. Do this once the cell is at rest (no WiFi load) for a clean
   reference.
5. **Multi-AP secrets audit** — `wifi_ssid` is currently the only used
   credential. Decide whether `wifi_ladi_*` / `wifi_mobile_wpl_*` are in
   range at the pool location before adding them as fallbacks.

## 2026-04-26 — ESPHome smoke test session

**Pivot:** moved from the PlatformIO/Arduino `main.cpp` track to **ESPHome** running
in the existing docker container at `/home/nuc8/dockerimages/docker-compose.yaml`
(image `ghcr.io/esphome/esphome:latest`, dashboard at `http://<host>:6052`,
config volume `esphome_config` mounted at `/config`).

### What was done

- Wrote a **minimal ESPHome smoke-test yaml** at `/config/pooltemperature.yaml`
  inside the container. Goal: verify only WiFi + DS18B20, nothing else.
  Old config backed up to `/config/pooltemperature.yaml.bak`.
- Config contents: `arduino` framework, `level: INFO`, single-SSID WiFi
  (`wifi_ssid`/`wifi_password`), API encrypted with `pooltemperature_key`,
  OTA with `ota_password`, GPIO16 always-on rail (internal switch),
  `one_wire` on GPIO4, `dallas_temp` at 12-bit / 10 s, `wifi_info` +
  uptime + restart button. **No** deep sleep, voltage divider, multi-AP,
  fallback AP, captive portal, or web server in this build.
- Validated with `esphome config` → `Configuration is valid!` on
  ESPHome 2026.4.2.

### Verified working on hardware

- **WiFi**: device joined, reachable via mDNS as `pooltemperature.local`,
  picked up DHCP `192.168.1.132` on first boot.
- **Static IP** added on second flash → device now lives at `192.168.1.86`,
  handshake in 4 ms.
- **DS18B20**: bus initially reported `Found no devices!` on first boot;
  after the OTA reflash for the static-IP change it came up clean and is
  publishing **26.6 → 26.7 °C every 10 s**. Wiring on GPIO4 (data) +
  GPIO16 (power) + 4.7 kΩ pull-up to 3V3 confirmed correct.

### Pre-existing issues found in the old `pooltemperature.yaml`

These are *not* fixed (the smoke-test yaml replaces the whole file); they
need to be cleaned up before the multi-AP / production block is restored:

1. `platform: uptime` was nested under `time:` — that's invalid in
   current ESPHome (`Platform not found: 'time.uptime'`). Belongs under
   `sensor:`.
2. The multi-AP block referenced secrets `poolschacht_wifi_ssid`,
   `poolschacht_wifi_password`, `outdoor_wifi_ssid`,
   `outdoor_wifi_password` — **none of these exist in `secrets.yaml`**.
   Available WiFi secrets: `wifi_ssid`/`wifi_password`,
   `wifi_ladi_ssid`/`wifi_ladi_password`,
   `wifi_mobile_wpl_ssid`/`wifi_mobile_wpl_password`. Either rename
   the references or add the missing secrets before re-enabling
   multi-AP failover.

### Open items for the production-layer build (next session)

1. Decide which SSIDs to put in the production multi-AP block and add the
   needed entries to `secrets.yaml`. With current secrets, the practical
   list is `wifi_ssid` (priority 0/main) + optionally `wifi_ladi_*` and
   `wifi_mobile_wpl_*` if the pool location can reach them.
2. Add deep sleep (3 h cycle): wake → power GPIO16 → read sensor → publish
   → hibernate. Mirror the old `main.cpp` behaviour in YAML. **Wrap OTA
   with `deep_sleep.prevent` / `deep_sleep.allow`** — without it OTA
   updates can brick the device mid-upload.
3. Pin the DS18B20 ROM address (grab from boot log) so the sensor skips
   bus enumeration on every wake — saves ~50 ms of active time per cycle.
4. Add the GPIO35 battery voltage divider as an ADC sensor with
   `attenuation: 12db`, `samples: 10`, `sampling_mode: avg`, plus a
   `multiply: 2.0` filter for the 1 MΩ / 1 MΩ divider. ESPHome returns
   calibrated millivolts since 2021.11 — **no manual `ADC_CAL_FACTOR`
   needed**. Layer a `template` battery-percent sensor on top.
5. Add `safe_mode:` as a separate top-level component (it is no longer
   implicit in the OTA block) and `captive_portal:` + AP fallback for
   credential recovery.
6. Keep `logger: level: INFO` and `baud_rate: 0` — INFO (not WARN) is now
   the recommended production level so WiFi/sensor lines stay visible
   for diagnosis without flooding flash.
7. Switch the build to `framework: { type: esp-idf, version: recommended }`.
   esp-idf is now the default for ESP32 in ESPHome and is more stable
   than arduino. Keep arduino only if a third-party library forces it
   (none currently in scope).
8. Add `min_version: 2026.4.0` to the `esphome:` block for reproducible
   builds.
9. Confirm the **3V3 power LED desolder** (still listed in the original
   open items below) and bench-measure deep-sleep current < 120 µA.

## 2026-04-26 — 1-year-runtime plan (power budget + 6 options)

User asked for: hourly temperature measurement, OTA window every 5th boot
for 2 min, boot counter visible to HA, battery percentage in HA, ≥ 1 year on
a single NCR18650B. Discovered the stated plan is **not mathematically
feasible** on this hardware and presented 6 alternatives.

### Power budget assumptions (locked-in)

- Usable cell capacity (80 % DoD + LDO losses): **2720 mAh**
- Deep-sleep current after LED cut: **~90 µA**
- Active-cycle average current: **~200 mA**
- Optimised normal wake (static IP + fast_connect + pinned BSSID + pinned
  DS18B20 address + INFO log): **~4–6 s**
- OTA window (as requested): **120 s**

### Six plans evaluated

| # | Plan | Daily mAh | Runtime |
|---|---|---|---|
| A | User's stated plan: hourly + OTA every 5th boot | 42.7 | ~2 months |
| B | Hourly + OTA every 24th boot (1×/day) | 19.0 | ~4.7 months |
| C | Hourly + OTA every 24th + 4 s tight wakes | 13.9 | ~6.4 months |
| **D** | **Hourly + OTA on physical button only (GPIO33)** | **7.5** | **~12 months ✅** |
| E | 3-hourly + OTA every 8th boot + 4 s wakes | 10.4 | ~8.6 months |
| **F** | **3-hourly + OTA every 56th boot (1×/week) + 4 s wakes** | **4.9** | **~18 months ✅✅** |

### Recommendation

- **D** if user can add a button + 10 kΩ pull-up between GPIO33 and 3V3
  (hardware change of ~€0.50, but matches their "hourly" intent best).
- **F** for zero hardware change with the most generous battery margin.
  Tradeoff: 3-hour cadence instead of 1-hour.

### Architecture (independent of which plan is picked)

1. `globals:` boot counter, `restore_value: yes` (NVS-persisted across deep sleep).
2. `deep_sleep:` with `run_duration` (4–6 s normal, 120 s OTA) and
   `sleep_duration` (1 h or 3 h).
3. `on_boot` priority 200: increment counter, publish, then either sleep or
   stay awake based on `boot_count % N`.
4. DS18B20 with **pinned ROM address** (TBD — read from boot log on first run).
5. Battery voltage on GPIO35 (ADC1) + battery-percent template sensor (linear
   3.2 V → 0 %, 4.2 V → 100 %).
6. OTA + safe_mode with `deep_sleep.prevent` / `.allow` wrapping.
7. Logger `INFO`, `baud_rate: 0`.
8. HA-visible sensors: temperature, battery V, battery %, boot count, WiFi
   RSSI, uptime.

### Open questions (blocking implementation)

1. Which plan: A / B / C / D / E / F? Recommendation: **D** or **F**.
2. For D: is GPIO33 free, will user wire a button + pull-up to it?
3. Blink GPIO22 LED during OTA window? Negligible mA, easier to see device
   is in OTA mode.
4. Battery cutoff voltage (old `main.cpp` had `BATT_CUTOFF_V`). 3.2 V is
   standard NCR18650B "empty" — confirm or override.
5. Should boot counter reset on OTA flash (per-firmware) or persist forever
   (cumulative wakes)?

User to choose 1–5; full yaml + validation + flashing checklist will follow.

## 2026-04-26 — Memory: auto-update SESSION_CHECKPOINT.md

User instructed "always store automatically the session info" — saved as a
project-scoped feedback memory at
`.claude/projects/.../memory/feedback_session_checkpoint.md`. Future
substantive turns in this project should append checkpoint entries
automatically without being asked.

## 2026-04-26 — ESPHome skill optimization (sidetrack)

Same calendar day, separate effort: optimized the `/esphome-expert` Claude
skill against the **current esphome.io documentation** so that future
ESPHome work in this and other projects starts from up-to-date defaults.

- Updated files:
  - `/home/nuc8/.claude/skills/esphome-expert/SKILL.md`
  - `/home/nuc8/.claude/skills/esphome-expert/references/production-template.yaml`
- Added a project-scoped memory at
  `/home/nuc8/.claude/projects/.../memory/user_esphome_setup.md` capturing
  the docker workflow, HA-native-only constraint, and available secrets.
- Notable corrections baked into the skill (relevant to this project's
  next phase):
  - **Legacy `dallas:` platform was REMOVED.** Use `one_wire:` +
    `dallas_temp` only — already done in the smoke test yaml.
  - **`safe_mode:` is now a separate top-level component** (was implicit
    in OTA before).
  - **`framework: esp-idf` is now the default** for ESP32 — switch on
    next build.
  - **`fast_connect: true` works fine with `networks:`** — the prior
    multi-AP block didn't actually need to drop it; it was missing
    secrets, that was the real issue.
  - **OTA platform-list syntax** (`ota: - platform: esphome`) is
    canonical since 2024.6 — the old flat syntax is removed.
  - **API custom services renamed** to `actions:`.
  - **`min_version`** in `esphome:` block recommended for reproducible
    builds.
  - **Validation workflow** documented as the always-do-first step:
    `docker exec esphome esphome config /config/<file>.yaml`.

## Project goal

Battery-powered ESP32 + DS18B20 pool thermometer → MQTT → Home Assistant.
Wakes every 3 h, transmits in ~30 s, then deep-sleeps. Single NCR18650B must
last ≥ 6 months. All user-added components must be through-hole / hand-solderable.

## Locked-in hardware choice

**Wemos/Lolin LOLIN32 Lite v1.0.0** (user already owns this board).
Confirmed from photos:
- ESP32-D0WDQ6 silicon
- ME6211-33 on-board LDO (Iq ≈ 50 µA)
- TP4054 on-board LiPo charger
- CH340 on-board USB-UART
- JST 1.25 mm PH 2-pin battery connector
- Micro-USB connector
- Only RST button (no BOOT) — relies on CH340 DTR/RTS auto-reset
- PCB trace antenna (no u.FL — no external-antenna upgrade path)
- Two green LEDs: 3V3 power LED (**must be desoldered**) + GPIO22 user LED

## Status

- [x] Reviewed original BOM image (`bom_pool_watermeter.png`)
- [x] Verified LOLIN32 Lite board from user photos (front + rear)
- [x] Final BOM: 16 items, all through-hole, ~€23/device (board owned)
- [x] Austrian supplier URLs (Reichelt primary; Amazon.at / Conrad / nkon /
      TME alternates) documented in `poolmeter.md`
- [x] Wiring diagram rewritten for LOLIN32 Lite (JST pigtail + 1N5817,
      DS18B20 on GPIO4/16, divider on GPIO35, no external LDO or caps)
- [x] 5×7 cm perfboard layout rewritten (no HT7333, no RST/BOOT buttons,
      no EN pull-up — all on-board)
- [x] Power budget updated: ~90 µA sleep → ~5.7 mAh/day → ~18-month runtime
- [x] Firmware (`firmware/src/main.cpp`): brown-out disable, read-battery
      before WiFi, hard cutoff → hibernation, static-IP support, 32-sample
      ADC, GPIO22 driven LOW at boot
- [x] Firmware (`firmware/include/config.h`): 1 MΩ divider, `BATT_CUTOFF_V`,
      `USE_STATIC_IP`, `ADC_CAL_FACTOR`, `USER_LED_PIN`
- [x] `firmware/platformio.ini`: board changed from `esp32dev` to
      `lolin32_lite`, upload_speed 921600
- [x] Troubleshooting table updated for LOLIN32 Lite specifics (CH340 flash
      recovery, power-LED check, active-low LED variant note, TP4054 PCM reset)
- [x] Added mandatory "Desolder the 3V3 Power LED" section at the top of
      `poolmeter.md` with tools list, empirical identification procedure,
      Method A (desolder LED), Method B (desolder series resistor),
      Method C (X-Acto trace cut), and µA verification step
- [x] Created `poolmeter_DE.md` — full Austrian-German translation of
      `poolmeter.md` (comma decimals, DACH conventions, code blocks kept
      in English so the build isn't affected)
- [x] Removed all Cirkit Designer material (`cirkit_wiring.md` deleted,
      related status entries and file-table row removed from this
      checkpoint) — user confirmed no further use
- [x] Created `wiring.md` — step-by-step breadboard build guide for
      non-professionals. 8 sequential steps, each with its own ASCII
      diagram: (0) place board, (1) power rails, (2) DS18B20 wires,
      (3) 4.7 kΩ pull-up, (4) 100 nF bypass on GPIO16, (5) 1 MΩ / 1 MΩ
      divider, (6) 100 nF ADC filter, (7) 1N5817 + battery. Includes
      parts list, breadboard primer, USB-only test checkpoint after
      step 6, full combined schematic, and final verification checklist.
      Gently corrects user's "100 n resistor" → "100 nF capacitor (104)"
- [x] Rewrote `poolmeter_DE.md` §Verdrahtungsplan for beginners: replaced
      single dense ASCII block with 4 nested subsections (Akku + Diode,
      DS18B20, Spannungsteiler, Micro-USB), each with its own small
      diagram + connection table. Added colour-coded wire legend
      (🔴 🟡 ⚫ 🟢), a "Pin-Karte" showing the board's actual silkscreen
      layout, a cheat-sheet table with two columns ("Aufdruck am Board"
      vs. "Im Code / Datenblatt"), pre-flight checklist, FAQ, and a
      flashing-recovery mini-section. Resolves a confusion reported by
      user: the DS18B20's "VDD" pin is a *sensor-side* datasheet name,
      not a silkscreen label on the LOLIN32 Lite — the board shows only
      the bare number `16`.

## Files changed this session

| File | Change |
|---|---|
| `poolmeter.md` | Intro, power budget, BOM (down to 16 items), suppliers, wiring, 5×7 perfboard, in-doc platformio.ini, optimization tips, troubleshooting — all rewritten for LOLIN32 Lite |
| `firmware/src/main.cpp` | Brown-out disable, GPIO22 LOW, read battery first, hibernate-on-cutoff, static-IP, 32-sample ADC, hibernation helper |
| `firmware/include/config.h` | 1 MΩ divider, `BATT_CUTOFF_V`, `USE_STATIC_IP`, `ADC_CAL_FACTOR`, `USER_LED_PIN` |
| `firmware/platformio.ini` | `board = lolin32_lite`, `upload_speed = 921600` |
| `poolmeter_DE.md` | §Verdrahtungsplan rewritten for non-professionals: 4 per-section diagrams+tables, wire-colour legend, Pin-Karte of actual silkscreen, two-column cheat sheet (board-silkscreen vs. code/datasheet name), pre-flight checklist, FAQ, flashing-recovery sub-section. "VDD" now clearly flagged as sensor-side, not a LOLIN32 pin. |
| `wiring.md` | New — beginner-friendly breadboard guide, 8 steps with per-step ASCII diagrams, USB-only test checkpoint, full schematic, checklist |
| `cirkit_wiring.md` | **Deleted** (user request — no further Cirkit use) |
| `SESSION_CHECKPOINT.md` | this file |

## Open items for next session

1. **Order components** — single Reichelt cart (~€29 including shipping).
   Details: `poolmeter.md` §"Where to Buy (Austria / DACH)".
2. **Physically desolder the 3V3 power LED** on the LOLIN32 Lite (mandatory
   before battery runtime will meet spec). Follow the step-by-step procedure
   at the top of `poolmeter.md` — choose Method A (desolder LED), B (desolder
   series resistor) or C (X-Acto trace cut). Verification target: deep-sleep
   current < 120 µA on the JST (+) line. Log the measured value here once
   done.
3. **Build the JST pigtail** (Keystone + → 1N5817 → JST +, Keystone − →
   JST −, heat-shrink both joints).
4. **Populate the 5×7 cm perfboard** per layout — female headers, 4.7 kΩ
   pull-up, 2× 1 MΩ divider, 2× 100 nF caps, DS18B20 screw block.
5. **Bench-test before cell install**: bench supply at 4.0 V into the JST,
   confirm 3V3 pin = 3.30 V ± 0.05 V, confirm idle draw < 120 µA.
6. **Flash firmware** via micro-USB. PlatformIO auto-reset via CH340 usually
   works; if not, short GPIO0 → GND and tap RST.
7. **One-shot ADC calibration**: read cell with DMM, compare to firmware-
   reported value, set `ADC_CAL_FACTOR` in `config.h`.
8. **Set real WiFi + MQTT credentials** in `config.h` (currently placeholders).
9. **Set real `STATIC_IP`** or `#undef USE_STATIC_IP` to fall back to DHCP.
10. **WiFi RSSI check at pool location** — no external antenna option on
    this board, so confirm ≥ −75 dBm before sealing the enclosure.
11. **Waterproofing** — silicone plug inside PG7, drip loop on probe cable
    (capillary moisture ingress is the #1 failure mode).

## Decisions (do not revisit unless explicitly asked)

- **Board**: LOLIN32 Lite v1.0.0 (user owns; verified via photos).
- **No external HT7333** — the on-board ME6211 clears the 6-month target
  with ~3× margin. Adding the HT7333 would only push runtime from 18 to 30
  months; not worth the extra complexity.
- **Keep 1 MΩ / 1 MΩ divider + 100 nF ADC cap** — saves ~160 mAh/year vs. 100 k.
- **Keep 1N5817 in the JST pigtail** — cheap reverse-polarity protection.
- **Keep DS18B20 powered via GPIO16** — saves ~4 Ah/year.
- **Sleep interval 3 h** (extendable to 6 h if ever needed; halves active mAh).
- **No external antenna path on this board** — if RSSI fails at the pool, we
  switch boards (ESP32-WROOM-32**U** + SMA), not patch this one.

## Power budget (LOLIN32 Lite with LED cut)

- Deep sleep: ~90 µA (ESP32 + ME6211 Iq + leakage)
- Wake cycle: 200 mA × ~8 s = 0.44 mAh
- Daily: 3.56 mAh active + 2.16 mAh sleep = **~5.7 mAh/day**
- 6 months (180 d): **~1,030 mAh** (30 % of 3400 mAh cell)
- Projected runtime: **~18 months per charge**
