# Progress

Hardware note: entries describing the former TLV9061 circuit are retained as
historical R0 investigation records. They are superseded by the final
"Git-Ready Cleanup and R1 LM358 Hardware" entry and must not be used as the
current schematic definition.

## Date Not Recorded

### Development Kit and Final MCU

- Initial peripheral development was performed on a CH32V203C8T6 development
  kit.
- The target custom PCB uses a CH32V203G8R6.
- Final-PCB behavior is being verified separately.

### Serial Monitoring

- Serial monitoring was tested.
- Result: **PASS**.
- Repository debug code supports USART1 TX on PA9 with caller-selected baud,
  8-N-1, and no flow control.
- Exact test baud and PC terminal application were not recorded.

### LCD on G8R6

- LCD operation was tested on the G8R6 PCB.
- Working mapping:
  - PB10 = D4.
  - PB11 = D5.
  - PB12 = D6.
  - PB8 = D7.
  - PB13 = RS.
  - PB14 = EN.
- Result: **PASS**.

### Keypad Driver

- Developed the three-button keypad driver.
- PA0 = MENU, PA1 = UP, and PA2 = DOWN.
- Buttons are active LOW with internal pull-ups.
- Key-event detection and delay-based debounce are implemented.
- Final integrated G8R6 keypad testing is not confirmed.

### SD Card

- Developed SPI and SD block drivers with PA4 CS, PA5 SCK, PA6 MISO, and PA7
  MOSI.
- Added SD initialization, block read, and block write support.
- Added a FatFs disk-I/O adapter.
- Initialization reported `SD INIT ERROR / CHECK SPI`.
- Result: **FAIL / PENDING DIAGNOSIS**.

### RTC / DS1307

- Developed a DS1307 I2C driver.
- PB6 = I2C1 SCL and PB7 = I2C1 SDA.
- Device address = 0x68.
- Tests reported `RTC ERROR NO RESPONSE` and `DS1307 NOT FOUND`.
- A time-setting/display flow was implemented in the current application.
- Result: **FAIL / PENDING DIAGNOSIS**.
- RTC persistence has not been verified.

### PWM Standalone Test

- Implemented PWM on PA8 using TIM1_CH1.
- Configuration: prescaler 0, ARR 959, 960 timer counts per period.
- Source assumes an approximately 96 MHz timer clock.
- Standalone oscilloscope testing produced approximately 100 kHz.
- Implemented a 0%-100% duty-cycle sweep.
- Result: **PASS for standalone 100 kHz generation**.

### PWM + LCD + Keypad Integration

- Combined PWM with LCD and keypad initialization.
- Intended behavior was to retain 100 kHz while keypad input changed duty.
- Integrated testing measured approximately 9 kHz.
- The standalone `pwm.c` implementation remains the known-good reference.
- Result: **FAIL / IN PROGRESS**.

## 2026-08-22

### CH32V203 Helix MP3 Port

- Reference source:
  `C:\OfficeWorks\esp_bell_idf\components\libhelix-mp3`.
- Created the separate `Firmware/LibHelixMP3` directory.
- Ported the fixed-point Helix Layer III decoder sources and retained the
  supplied license files.
- Added `ch32_mp3_player.c/.h` as a project-level wrapper for FatFs streaming,
  decoding, PCM buffering, and PWM playback.
- Adapted hardware output to the schematic `AUX` signal on PA8/TIM1_CH1.
- Used existing schematic-matched SD pins: PA4 CS, PA5 SCK, PA6 MISO/SD_DO,
  and PA7 MOSI/SD_DI.
- Added TIM1 8-bit PWM output with ARR 255, nominally 375 kHz at the configured
  96 MHz timer clock.
- Added TIM2 update interrupts for decoded sample timing.
- Added two 576-sample PCM buffers and an underflow counter.
- Added explicit file, decoder, stereo, MPEG-1, and sample-rate status results.
- Restricted supported input to mono MPEG-2/2.5 Layer III at 8-24 kHz to fit
  the CH32V203G8R6's 20 KB SRAM design target.
- Replaced decoder heap allocation with one statically allocated instance.
- Added early rejection of channel counts larger than the configured decoder.
- Corrected signed high-word handling in the RISC-V `MADD64` path.
- Added Helix include paths to both checked-in MounRiver project files.
- Validated that both `.wvproj` files remained valid JSON and that the patch had
  no whitespace errors.
- Build result: **PENDING** because the WCH/MounRiver GCC executable was not
  available in the working environment.
- Hardware audio result: **PENDING**.

### MP3 Port Decisions and Observations

- PA8/TIM1_CH1 is shared between the legacy 100 kHz PWM test and MP3 audio.
- The two timer configurations are not intended to run simultaneously.
- SD/FatFs operation is a prerequisite for end-to-end MP3 playback and remains
  unresolved.
- Link-map verification is required before the low-memory design is accepted.

## 2026-08-24

### Documentation Baseline and MP3 Synchronization

- Consolidated the project documentation into exactly five Markdown files in
  `document/`.
- Preserved the earlier peripheral implementation and test history.
- Added the Helix MP3 port architecture, supported formats, memory constraints,
  timer ownership, source modifications, tests, and acceptance criteria.
- Added MP3 build, link-map, format-rejection, timing, underflow, and `AUX_OUT`
  verification tasks.
- Marked all uncompiled and untested MP3 behavior as pending rather than PASS.
- Removed the separate library README after incorporating its valid information
  into the five maintained project documents.

## 2026-08-24 (Full-Port Documentation Baseline)

### `esp_bell_idf` Reference Review

- Fully reviewed the `esp_bell_idf` reference project's non-radio source:
  `main/main.cpp`, `main/bell.cpp`, `main/menu.cpp`, `main/mydef.h`,
  `main/config.hpp`, and drivers `astro`, `eeprom`, `keys`,
  `lcd`, `mp3_player` (local path), `rtc`, `storage`. Reviewed
  `wifi_player` only to identify what to exclude.
- Confirmed with the project owner that Bluetooth and WiFi (including HTTP/
  RTSP streaming and the `l` playlist token) are explicitly out of scope for
  this port.
- Fixed an `AGENT.md` inconsistency: the documentation rules referenced a
  `document/` subdirectory that does not exist in this repository; the five
  Markdown files live at the repository root. Corrected the two references.

### Documentation Updates

- Added an "Adopted Reference Project" section and an intended full
  application flow to `README.md`.
- Added a full-port scope summary table, detailed requirements for the
  scheduled playlist engine, the Panchang astronomy engine, the EEPROM-style
  configuration store, relay/light control, and `.enc` local-audio decoding
  to `REQUIREMENTS.md`, along with new full-port memory/platform constraints,
  testing requirements, and acceptance criteria.
- Added a "Full Functional Port from `esp_bell_idf`" checklist to
  `PENDING_TASKS.md` covering groundwork decisions, the Panchang engine, the
  scheduling engine, the front-panel menu, the config store, relay/light
  control, and full-port validation. Existing pending items (SD, RTC, PWM
  integration) were left unchanged since none are complete.

### Open Decisions Flagged (not yet resolved)

- Physical mechanism for the EEPROM-style persistent config store: external
  I2C EEPROM (possibly already present on a DS1307 carrier module) vs.
  CH32V203 internal flash emulation.
- Whether the ported Panchang engine's software-floating-point performance
  on CH32V203 (no confirmed hardware FPU) is acceptable.
- Combined SRAM footprint of all adopted modules against the 20 KB budget.

No implementation of the full-port scope has started; this update is
documentation-only, per the project's "update documents first" instruction.

## 2026-08-24 (FreeRTOS Integration and DMA-Driven Audio)

### Decisions Made

- EEPROM-style persistent config store: CH32V203 internal flash emulation
  (no external EEPROM assumed).
- FreeRTOS source: vendor from the user's own working `CH32V203-FreeRTOS`
  MounRiver project rather than fetching/reconstructing a kernel port from
  scratch, since it is WCH's own verified port for this exact chip family.
- Debug UART baud rate set to 115200 (previously undefined/TBD in this
  project) so `configASSERT()` and the new FreeRTOS hooks have somewhere to
  report.

### FreeRTOS Integration

- Vendored the FreeRTOS kernel (`list.c`, `queue.c`, `tasks.c`, `timers.c`,
  `event_groups.c`), `heap_4.c`, and the GCC RISC-V/PFIC port (`port.c`,
  `portASM.S`, `portmacro.h`, the `RV32I_PFIC_no_extensions` chip-specific
  header) unmodified under `Firmware/FreeRTOS`, copied byte-for-byte from
  the reference project. `croutine.c` and `stream_buffer.c` were not
  vendored (unused features).
- Added `Firmware/User/FreeRTOSConfig.h`, written for this project (not
  copied): 1000 Hz tick, `configTOTAL_HEAP_SIZE` 6 KB (unmeasured starting
  point for the 20 KB SRAM budget), software timers disabled, stack-overflow
  checking (method 2) and the malloc-failed hook both enabled and reporting
  over the debug UART before halting.
- Added `vApplicationStackOverflowHook()` and `vApplicationMallocFailedHook()`
  to `Firmware/User/ch32v20x_it.c`, alongside the existing NMI/HardFault
  handlers.
- Confirmed no startup-file changes were needed: `Firmware/Startup/*.S`
  already declare `SysTick_Handler`/`SW_Handler` as weak (standard WCH
  template); FreeRTOS's port supplies the strong definitions.
- Restructured `Firmware/User/main.c`: peripheral init (LCD/keypad/RTC,
  plus the new relay/light GPIO init) still happens in `main()` before the
  scheduler starts; the existing clock-display/time-set loop moved into
  `ClockMenuTask` with every `Delay_Ms()` inside it replaced by
  `vTaskDelay()` so it yields to the scheduler. Behavior is intended to be
  identical to the previous bare-metal loop -- not yet build- or
  hardware-verified.
- Added `Firmware/User/relay_light.c/.h`: GPIO init and `Relay_Set()`/
  `Light_Set()` for `RLY_IN` (PA3) / `LGT_IN` (PB0). GPIO plumbing only; no
  scheduling logic yet (depends on the not-yet-implemented config store).
- Updated both `Firmware/CH32V203C8T.wvproj` and `Firmware/CH32V203G8R.wvproj`
  (the active MounRiver build configs) with the new FreeRTOS include paths
  for the C compiler and assembler, and set their `chipInfo.rtos` field to
  `"FreeRTOS"` for accuracy. Mirrored the same include paths into
  `Firmware/.cproject` for IDE/indexer consistency. Verified both `.wvproj`
  files remain valid JSON and `.cproject` remains valid XML after editing.
  Neither project file appears to enumerate source files explicitly (both
  rely on directory scanning with an exclude list), so the newly added
  `Firmware/FreeRTOS/*.c` and `Firmware/User/relay_light.c` should be
  picked up automatically -- unconfirmed until a real build is run.

### DMA-Driven Audio Path

- Rewrote `Firmware/LibHelixMP3/ch32_mp3_player.c`'s audio output: TIM2's
  sample-rate update event now drives a DMA1 Channel2 request that writes
  each prepared sample directly into `TIM1->CH1CVR`, replacing the previous
  per-sample `TIM2_IRQHandler`. Volume scaling and the PWM-duty bias now run
  once per 576-sample decoded frame (`Audio_PrepareSlot()`, right after
  `MP3Decode()`) using the exact same formula as the old ISR, instead of
  once per sample -- numeric output for a given sample is unchanged. A
  small circular silence buffer keeps the PWM carrier alive with no
  stall/glitch when no decoded data is ready. Underflow-counting semantics
  were preserved deliberately: `gapAfterPlayback` is only set when real
  playback runs dry into silence, not on every normal slot-to-slot handoff
  (an earlier draft of this change had that bug; caught and fixed before
  committing).
- Flagged, not resolved: the TIM2_UP -> DMA1 Channel2 request-line mapping
  used is the standard STM32F103-compatible assignment, not yet confirmed
  against the CH32V203 reference manual.
- No RAM footprint change: the DMA source reuses the existing
  `pcmBuffer[2][576]` array in place (converted to ready-to-DMA values
  before being marked ready) rather than adding a second buffer.

### Not Done in This Session

- No build was attempted (WCH/MounRiver toolchain unavailable in this
  working environment). Nothing above has been hardware-tested.
- The scheduling engine, Panchang engine, menu extensions, and the
  flash-emulated config store itself (only its storage mechanism was
  decided) are still not implemented -- see `PENDING_TASKS.md`.

## 2026-08-24 (Full Application Port)

### Scope

Implemented the complete non-radio `esp_bell_idf` feature set as C source
on this target, on owner instruction, and removed the old test code. New
code follows the owner's style rule now recorded in `AGENT.md`: short
comments only where necessary, grouped statements.

### New / Rewritten Source

- `Firmware/Astro/tamil_panchangam.c/.h`: copied verbatim from the
  reference; only `detect_special_day()`/`detect_festival()` adapted from
  POSIX stdio to FatFs (plus a local line reader -- `f_gets` is disabled in
  this FatFs config). All astronomy math untouched.
- `Firmware/User/bell.c`: settings load (all timezone formats), holiday
  skip, playlist engine (types `1/2/3`; tokens `f g m h w d M W D s b F S
  p`; `l` skipped as WiFi-only), panchang announcement sequence, group
  rotation with power-loss resume, light task, welcome scroll, clock/date
  display.
- `Firmware/User/menu.c`: full setup chain (bool/range screens, playlist
  editor from `menu.txt`, date/time set, light on/off times, bell/starting
  music track selection with live preview). The esp custom LCD edit glyph
  was replaced with `>` (no CGRAM helper on this driver).
- `Firmware/User/eeprom.c/.h`: flash-emulated config store. Initial design
  used two 1 KB pages with `FLASH_ErasePage`; discovered during review
  that `FLASH_ErasePage` on CH32V20x erases 4 KB, so the store was
  redesigned onto eight 256-byte fast pages (0x0800F800..0x0800FFFF,
  round-robin, checksummed, sequence-numbered). Verified with a host-side
  simulation: reboot persistence, rotation through all slots, and
  corrupt-newest-record fallback all pass. Payload 240 bytes; rotating
  group ids limited to 1..94.
- `Firmware/User/storage.c/.h`, `player.c/.h`, `rtc_util.c/.h`,
  `mydef.h`: FatFs mount/file helpers, async audio-task wrapper over the
  blocking decoder, date math (Zeller weekday, week/day-of-year, 12-hour,
  time/date strings), shared definitions.
- `Firmware/User/main.c`: rewritten as the bell application (boot checks
  with visible LCD errors, task creation, idle key handling, relay settle
  logic). The RTC/keypad/LCD test application it previously contained is
  gone.
- `Firmware/LibHelixMP3/ch32_mp3_player.c/.h`: added `.enc` XOR decode
  (key `BK26`, applied after `f_read`), `CH32_MP3_RequestStop()`, replaced
  busy-spin waits with `vTaskDelay(1)`, and removed the internal
  `f_mount` (storage.c owns the mount).
- Driver touch-ups: `lcd.c/.h` gained cursor/blink/number/clear-region
  helpers; `keypad.c` `KEY_WaitKey` now polls with `vTaskDelay` instead of
  `Delay_Ms`; `relay_light.c` reduced to plain GPIO writes (settle delays
  moved to `setRelay()` in main.c, matching the reference).

### Removed

- Old test `main.c` application and legacy 100 kHz `pwm.c/.h` (owner
  instruction). The pwm files are parked in `_to_delete/` at the repo root,
  outside the build, pending final deletion. This retires the PA8 mode
  conflict and the unresolved integrated ~9 kHz anomaly as work items.

### Project / Linker Changes

- `Ld/Link.ld`: FLASH length 64K -> 62K, reserving the top 2 KB for the
  config store.
- Both `.wvproj` files and `.cproject`: added `Astro` include path.
- Validated `.wvproj` JSON and `.cproject` XML after editing; brace/paren
  balance checked on every touched C file; no ESP-IDF identifiers remain
  in the ported sources.

### Not Done

- No toolchain build, no hardware testing (toolchain unavailable in this
  environment). DS1307 and SD failures from earlier testing still block
  boot-through. See `PENDING_TASKS.md`.

## 2026-08-24 (Post-Port Audit)

Audited the full port on owner request. Checks run and their results:

- Cross-module symbol check (every `LCD_/KEY_/RTC_/EEPROM_/Player_/
  Storage_/Relay_/Light_/CH32_MP3_/calc_panchang` call in the new sources
  resolved against its header): all resolved.
- All `mydef.h` prototypes have exactly one definition; shared globals
  defined once.
- `rtc.c` field semantics confirmed against my assumptions: year 0-99,
  24-hour clock, DS1307 day register 1-7.
- `calc_panchang()` confirmed to fill `festival`/`special_day` internally
  (calls `detect_special_day`/`detect_festival` at the end).
- No stale references to the removed pwm test or old test application.
- Brace/paren balance re-verified on every touched C file; project files
  re-validated (JSON/XML).

Defects found and fixed:

- Math library was not linked: added `m` to both `.wvproj` linker
  configurations (the panchang engine uses `sin/cos/atan2/fmod/floor/pow`;
  newlib does not link libm implicitly).
- `main()` was missing `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1)` and
  `SystemCoreClockUpdate()`, both present in the working WCH FreeRTOS
  reference main and needed by the port's software-interrupt yield and
  `configCPU_CLOCK_HZ`.
- Light-task stack raised 96 -> 128 words (interrupt context is saved on
  the running task's stack in this port; 96 was too thin).
- `loadSettings()` now drops the trailing character of the panchang-order
  string, matching esp_bell_idf exactly (its writer app appends a
  terminator character).
- `scrollText()` bounds check reordered so the welcome buffer is never read
  past its end (latent out-of-bounds read inherited from the reference).

Also corrected during this session: a stale copy of `main.c` was briefly
delivered to the conversation before the audited version; the file written
to the repository is the corrected one.

Not fixed (unchanged risks): no toolchain build yet; SD/DS1307 hardware
failures still block boot-through; RAM/flash budgets unmeasured; DMA
request mapping unconfirmed. All tracked in `PENDING_TASKS.md`.

## 2026-08-24 (Local and Remote Change Integration)

### Remote Changes Preserved

- Fetched and fast-forwarded local `main` through the three latest remote
  commits: `4a76b24`, `d109c9f`, and `45a70ad`.
- Preserved the remote FreeRTOS application, full non-radio `esp_bell_idf`
  port, Panchang engine, scheduler/menu, flash-emulated settings, relay/light
  logic, `.enc` support, audited fixes, and DMA-driven MP3 audio changes.
- Preserved the remote decision to remove legacy `pwm.c/.h` from the build and
  assign PA8/TIM1_CH1 to MP3 audio.

### Earlier Local Hardware Results Preserved

- Keypad MENU/UP/DOWN test: **PASS**.
- DS1307 detection/read/write test: **PASS**.
- RTC backup persistence test: **PASS**; exact duration/setup was not supplied.
- SD initialization and FatFs file-read path: **PASS**.
- `TEST.TXT` content display on the 16x2 LCD: **PASS**.
- `TEST.TXT` UP/DOWN paging and MENU exit: **PASS**.
- `TEST.TXT` viewer error handling: **PASS**; exact cases exercised were not
  supplied.
- Latest legacy PWM test before removal: **PASS**; exact final measurement was
  not supplied. The earlier approximately 9 kHz failure remains above as
  historical information.
- These observations were made before the full FreeRTOS application merge and
  therefore do not constitute post-integration hardware validation.

### SD-to-LCD Diagnostic Integration

- Retained `Firmware/User/sd_text_viewer.c/.h` and adapted it to use the
  `storage.c`-owned FatFs mount.
- Kept the reader blocking and pre-scheduler so its `Delay_Ms()` waits do not
  block running FreeRTOS tasks.
- Added a non-conflicting startup entry: hold MENU after power-up to open
  root-directory `TEST.TXT`; UP/DOWN page and MENU exits.
- Moved the startup-only FatFs file object and 32-character page buffer from
  static RAM to the already reserved main stack.
- Preserved printable-ASCII formatting, CR/LF handling, bounded 32-character
  pages, previous-page rescanning, and visible mount/file/read errors.
- The new startup trigger is implemented but not yet hardware-tested.

### Project and Linker Integration

- Kept FatFs, Helix, FreeRTOS, and Astro include paths in both MounRiver
  `.wvproj` files and synchronized them into `Firmware/.cproject`.
- Added `__freertos_irq_stack_top` at the top of the reserved main stack in
  `Firmware/Ld/Link.ld`, as required by the imported FreeRTOS RISC-V port.
- Validated both `.wvproj` files as JSON and `.cproject` as XML.
- The checked-in generated `Firmware/obj` makefiles are stale and still need
  regeneration by MounRiver before they can represent the merged source tree.

### Integrated Build Attempt

- Toolchain: MounRiver Studio 2 WCH RISC-V Embedded GCC 8.2.0.
- Compiled all 64 C sources and assembled the WCH startup plus FreeRTOS port
  assembly: **PASS**.
- Warning: the existing unused `RTC_WaitEvent()` helper remains in `rtc.c`.
- Final G8R6 link: **FAIL** due to target capacity.
- FLASH overflow: 58,240 bytes beyond the configured 62 KB application region.
- Static RAM from the map: `.data` 1,208 bytes + `.bss` 25,464 bytes =
  26,672 bytes before the 2,048-byte main/ISR stack and FreeRTOS task stacks.
- The full feature set cannot fit the current 64 KB flash / 20 KB SRAM target
  without a documented memory and feature redesign.

### MounRiver Managed Build Confirmation

- MounRiver Studio regenerated the managed build files and ran the G8R6
  incremental build.
- Generated source lists include Astro, FreeRTOS, the FreeRTOS RISC-V port and
  heap, Helix, FatFs, all 16 current `Firmware/User` sources, and
  `sd_text_viewer.c`; removed `pwm.c` is not included.
- The build reaches the linker, confirming that source enumeration and include
  paths are no longer the blocker.
- Managed-build result: **FAIL** because the image does not fit the target.
- FLASH overflow: 57,968 bytes beyond the configured 62 KB application region.
- Managed map: `.data` 1,208 bytes + `.bss` 25,452 bytes = 26,660 bytes before
  the 2,048-byte main/ISR stack and dynamically allocated task stacks.
- Removed managed-makefile regeneration from `PENDING_TASKS.md`; flash/RAM
  redesign remains pending.

### Linkable G8R6 Diagnostic Profile

- Added `Firmware/User/happybell_build.h` as the centralized build-profile
  selector.
- Selected a bare-metal SD/LCD diagnostic as the G8R6 default while preserving
  the complete FreeRTOS/Astro/MP3 application behind the full profile.
- Updated the G8R MounRiver project metadata to `NoneOS` for the default
  bare-metal profile while retaining the FreeRTOS/Astro include paths and the
  math library needed by the preserved full-profile sources.
- The default profile initializes LCD, keypad, RTC, and SD/FatFs, then opens
  root `TEST.TXT` automatically. UP/DOWN page, MENU exits, and MENU can reopen
  the viewer from the idle prompt.
- Rebuilt through the generated MounRiver makefiles with WCH GCC 8.2.0:
  **PASS**.
- Size result: `text=14,888`, `data=160`, `bss=5,212` bytes. The size tool's
  BSS total includes the reserved 2,048-byte main/ISR stack; total reported RAM
  use is 5,372 bytes.
- The diagnostic `.elf` and `.hex` were produced without FLASH overflow or
  RAM/stack overlap.
- Hardware flashing and post-profile peripheral/viewer revalidation remain
  pending; no new hardware PASS claim was made.

### Standalone G8R6 MP3 Hardware-Test Profile

- The user reported that the preceding capacity-error result was no longer
  occurring after the diagnostic-profile change. The exact observed hardware
  screen/result was not specified, so no additional peripheral PASS was
  inferred from that message.
- Added `HAPPYBELL_PROFILE_MP3_TEST` and selected it as the default bounded
  G8R6 test. MENU opens `TEST.TXT`; UP plays root `00001.mp3`, falling back to
  `00001.enc`, and displays the decoder result and DMA underflow count.
- Made `ch32_mp3_player.c` use `Delay_Ms(1)` in bare-metal profiles while
  retaining `vTaskDelay()` for the full FreeRTOS profile.
- The first MP3-test link missed SRAM by 468 bytes. Reduced the input buffer
  from 1536 to 1024 bytes and added the explicit
  `CH32_MP3_FRAME_TOO_LARGE` result so an oversized encoded frame is rejected
  instead of causing an input-underflow loop.
- Confirmed in WCH's CH32FV2x/V3x reference-manual DMA table that TIM2_UP maps
  to DMA1 Channel2. The channel is shared with SPI1_RX/TIM1_CH1; the current
  SD path is polled.
- Removed the unused `RTC_WaitEvent()` helper that caused the remaining
  compiler warning.
- Forced managed rebuild: **PASS with no warnings**. All three profile paths
  also pass direct syntax checks.
- MP3-test size: `text=50,036`, `data=824`, `bss=19,612` bytes. The BSS total
  includes the reserved 2,048-byte stack; FLASH use is 50,860 of 63,488 bytes
  and total reported RAM use is 20,436 of 20,480 bytes, leaving 44 bytes.
- Compiled all 64 C sources with GCC `-fstack-usage`. The MP3 foreground frame
  is 784 bytes, `MP3Decode` is 64 bytes, and the largest reported Helix
  subroutine is 272 bytes. These static results fit the reserved 2,048-byte
  stack; runtime stack-guard validation under DMA interrupts remains pending.
- Produced a new `.elf` and `.hex`. Flashing, playback, PWM/AUX_OUT, format,
  and underflow validation remain pending.

### Plain MP3 / `.enc` A/B Test Preparation

- The user reported that output appeared from the MP3-test image. The report
  did not specify whether this meant LCD output or audible `AUX_OUT`, nor the
  LCD status, file type, or underflow count; those details remain pending.
- Changed the MP3-test controls so UP plays only SD-root `00001.mp3` and DOWN
  plays only SD-root `00001.enc`. This avoids the previous fallback behavior
  and permits a direct comparison of the same audio bytes.
- Added `tools/encode_mp3.ps1`, which applies the firmware's repeating `BK26`
  XOR key, refuses accidental overwrite unless `-Force` is supplied, and
  checks input/output paths.
- Host validation: encoded a file twice and compared SHA-256 with the original;
  hashes matched (**PASS**).
- Incremental managed build: **PASS with no warnings**. Updated size:
  `text=50,096`, `data=824`, `bss=19,612` bytes. FLASH use is 50,920 of
  63,488 bytes; total reported RAM remains 20,436 of 20,480 bytes.
- Flashing the A/B image and comparing MP3/`.enc` LCD result, underflows, and
  `AUX_OUT` remain pending.

### Documentation Consolidation

- Merged the remote full-application documentation with the local test history.
- Retained exactly five Markdown files under `document/` and removed duplicate
  root-level copies.
- Updated current status and pending work so pre-integration PASS results are
  not confused with validation of the latest merged application.

### Automatic SD Audio Selection

- Replaced the standalone MP3 test's fixed `00001.mp3`/`00001.enc` selection
  with a root-directory scan using FatFs `f_opendir()`/`f_readdir()`.
- The test chooses the first `.mp3` or `.enc` entry case-insensitively, shows
  `PLAYING:` and its FAT short filename on the 16x2 LCD, then sends decoded
  audio through the existing TIM1/TIM2/DMA path to the PCB `AUX_OUT` stage.
- Playback starts automatically after SD mounting. UP rescans/replays; MENU
  retains the `TEST.TXT` viewer.
- Directory objects, file information, and the 13-byte FAT short path remain
  on the main stack so the profile does not consume additional static SRAM.
- MounRiver/WCH GCC incremental build: **PASS with no warnings**. Size is
  `text=50,428`, `data=824`, and `bss=19,612` bytes. Static RAM remains
  20,436 of 20,480 bytes, leaving the prior 44-byte margin unchanged.
- A fresh GCC `-fstack-usage` compile reports a 160-byte `main()` frame with
  the directory scan inlined; the reserved main/ISR stack remains 2,048 bytes.
- Hardware result: **PENDING**. Audible output, LCD result, and DMA underflow
  count must be reported after flashing.

### `menu.txt` and Folder `001` ENC Browser

- Updated the bounded MP3 test for the supplied SD layout: root `menu.txt`
  plus numbered audio folders.
- The LCD pager now opens root `menu.txt` automatically before playback; MENU
  exits and can reopen it later. Generic `FILE MISSING`/`FILE EMPTY` messages
  replaced the old `TEST.TXT`-specific error text.
- Folder `001` is scanned for `.enc` entries only. The first encrypted file
  plays automatically, UP/DOWN browse next/previous entries with wraparound,
  and `.mp3` files are ignored.
- The selected `001/<short-name>.ENC` path is displayed during playback,
  followed by the existing decoder status and DMA underflow count.
- Made `.enc` recognition in the Helix wrapper ASCII case-insensitive so a FAT
  short filename returned as `.ENC` is decrypted correctly.
- MounRiver/WCH GCC incremental build: **PASS with no warnings**. Size is
  `text=50,352`, `data=824`, and `bss=19,612` bytes; static RAM remains 20,436
  of 20,480 bytes with the same 44-byte margin.
- All three profile syntax checks pass. GCC stack reports are 48 bytes for
  `main()`, 128 bytes for the ENC scan, and 64 bytes for selection dispatch;
  the existing playback/decode frames and 2,048-byte reserved stack remain.
- Hardware result: **PENDING**. The contents of `menu.txt`, the SD `.enc`
  encoding parameters, audible output, LCD result, and underflow count have
  not yet been observed with this image.

### Mono MPEG-1 SD Asset Enablement

- Hardware observation from the folder-`001` image: LCD displayed
  `MPEG1 REJECTED` and `UNDERFLOW: 0` for a selected `.enc` asset. This
  confirms the old guard reached a valid mono MPEG-1 header after decryption;
  audible output was not reported.
- Confirmed the result is unrelated to the conditional headers in `main.c`.
  FreeRTOS/task/application headers remain guarded because their complete
  profile still exceeds G8R6 FLASH and RAM; including them would not change
  Helix frame output size.
- Enabled mono MPEG-1 Layer III without adding static RAM. Helix writes each
  1,152-sample frame across the two existing contiguous 576-sample PCM slots,
  which DMA streams sequentially.
- Extended accepted sample rates from 8-24 kHz to 8-48 kHz. Stereo and encoded
  frames larger than the 1,024-byte input buffer remain rejected.
- The next MPEG-1 frame must wait until both PCM slots are free, so a real
  frame-boundary gap may be audible and is counted as an underflow. MPEG-2/2.5
  retains the existing double-buffered path.
- MounRiver/WCH GCC incremental build: **PASS with no warnings**. Size is
  `text=50,476`, `data=824`, and `bss=19,612` bytes; static RAM remains 20,436
  of 20,480 bytes with the same 44-byte margin.
- MounRiver later saved an older open `main.c` editor buffer, restoring the
  removed `CH32_MP3_UNSUPPORTED_MPEG1` case and the obsolete root-file scan.
  This caused an undeclared-enum compile error. Restored the current
  `menu.txt` + `001/*.enc` browser, removed the stale case, and repeated the
  managed build with the same passing size result.
- All three `main.c` profile syntax checks pass. `CH32_MP3_PlayFile()` remains
  a 784-byte static stack frame.
- New MPEG-1 hardware result: **PENDING**. Audible output, playback quality,
  result text, and the expected boundary-underflow count need to be observed.

### Immediate ENC Playback and Amplifier-Relay Enable

- User hardware observation: SD file information is displayed on the LCD, but
  no audio was heard through the connected speaker. The exact post-playback
  decoder status, underflow count, PA8 waveform, and speaker/amplifier type
  were not supplied, so decoded audio is not marked PASS.
- Re-inspected all three pages of `happybell_schematic.pdf`. PA8/TIM1_CH1 is
  connected to `AUX`, followed by the PCB op-amp/filter and AC-coupled two-pin
  `AUX_OUT` connector. The schematic does not contain a speaker power
  amplifier, so a powered speaker or suitable external amplifier is required
  for the audible-output test.
- Confirmed PA3/RLY_IN drives the relay-output transistor stage. The retained
  ESP reference asserts this signal before playback, but the bounded CH32 MP3
  test previously initialized relay GPIO only in the full profile.
- Changed the default MP3 test to initialize relay/light GPIO, drive PA3 HIGH,
  wait 300 ms before decoding, and drive PA3 LOW after playback.
- Removed the blocking automatic `menu.txt` pager from startup. The test now
  scans and plays the first `001/*.enc` immediately after SD mounting;
  `menu.txt` remains available by pressing MENU afterward.
- Managed MounRiver/WCH GCC incremental build: **PASS**. Size is
  `text=50,652`, `data=824`, and `bss=19,612` bytes. FLASH use is 51,476 of
  63,488 bytes; RAM remains 20,436 of 20,480 bytes with the same 44-byte
  margin.
- Hardware result for this new image: **PENDING**. Flash it, use an amplified
  AUX load, and record the LCD result, underflows, PA3 state, PA8 PWM, and
  `AUX_OUT` waveform/audio.

### Playback-Startup Reset Isolation

- User hardware result for the relay-enabled image: LCD showed `PLAYING:` and
  `001/0001.ENC`, no sound was heard, and the display then returned to
  `Loading...`.
- `Loading...` is written only by the boot path in `main.c`. The project
  HardFault handler also calls `NVIC_SystemReset()`, so this observation is a
  reset during playback startup rather than a normal decoder return. No
  `PLAYBACK OK`/error status or underflow result was reached.
- Removed relay/light initialization and PA3 relay switching from the bounded
  MP3 test. The schematic keeps RLY_OUT separate from AUX_OUT, so this image
  isolates SD/XOR/Helix/DMA/PA8 playback from an external relay or power-load
  reset. Full-profile relay behavior remains unchanged.
- Managed MounRiver/WCH GCC incremental build: **PASS**. Size is
  `text=50,468`, `data=824`, and `bss=19,612` bytes. FLASH use is 51,292 of
  63,488 bytes; RAM remains 20,436 of 20,480 bytes with a 44-byte margin.
- No copy of `001/0001.ENC` exists in the workspace. If this relay-free image
  still resets, the asset is required for host-side decryption/frame
  validation and the decoder/HardFault path must be instrumented.

### Bare-Metal RTOS Removal and HardFault Staging

- User hardware result: the relay-free image produced the same sequence as
  before (`PLAYING:` / `001/0001.ENC`, no sound, then boot `Loading...`).
  This excludes PA3 relay activation as the reset trigger and places the fault
  in file/decode/audio startup.
- Link-map inspection found that the bare-metal image still contained the
  FreeRTOS strong SysTick handler, `xTaskIncrementTick()`, scheduler lists, and
  ISR-stack symbol because the managed project compiles `port.c` and
  `portASM.S` for every profile.
- Added matching build-profile guards to the FreeRTOS C and assembly port
  sources and to the FreeRTOS-only hooks. The MP3 ELF now contains none of
  `pxReadyTasksLists`, `xTaskIncrementTick`, FreeRTOS `SysTick_Handler`, or
  `xISRStackTop`. The full profile still emits the original port.
- Increased the shared main/interrupt stack from 2,048 to 2,304 bytes using
  recovered bare-metal RAM.
- Added an 8-byte `.noinit` HardFault record. The MP3 wrapper records file,
  read, sync, decode, audio-init, PCM-prepare, and DMA-output stages; after
  `NVIC_SystemReset()`, boot shows `HARDFAULT` and the retained stage for five
  seconds.
- Managed MounRiver/WCH GCC build: **PASS**. Size is `text=44,352`,
  `data=712`, and `bss=19,664` bytes. FLASH use is 45,064 of 63,488 bytes;
  total RAM use is 20,376 of 20,480 bytes, leaving 104 bytes.
- C syntax checks pass for diagnostic, MP3-test, and full profiles. The
  profile-gated assembly compiles in both bare-metal and full modes.
- Hardware result for this instrumented/enlarged-stack image: **PENDING**.

### FatFs Stack Reduction After `HARDFAULT UNKNOWN`

- User hardware result: the enlarged-stack/staged image displayed
  `PLAYING:` / `001/0001.ENC` and then `HARDFAULT UNKNOWN`.
- A valid retained magic value with an out-of-range stage showed that the
  stage byte next to the lower stack boundary had been overwritten before the
  HardFault reset, strongly indicating stack overflow.
- Found `FF_FS_TINY=0` in `ffconf.h`. In that mode, the local FatFs `FIL`
  object contributes a private 512-byte sector buffer to
  `CH32_MP3_PlayFile()` even though the mounted `FATFS` object already owns a
  512-byte sector window.
- Enabled read-only `FF_FS_TINY=1`. A new GCC `-fstack-usage` measurement
  reduced `CH32_MP3_PlayFile()` from 784 to 272 bytes while keeping the
  2,304-byte reserved main/interrupt stack.
- Moved the 8-byte `.noinit` HardFault record from immediately below the stack
  to immediately after `.data` and before normal `.bss`. A future stack fault
  can no longer corrupt the diagnostic record first.
- Managed MounRiver/WCH GCC build: **PASS**. Size is `text=44,308`,
  `data=712`, and `bss=19,664` bytes. FLASH use is 45,020 of 63,488 bytes;
  total RAM remains 20,376 of 20,480 bytes, leaving 104 bytes.
- Hardware result for the FatFs-tiny image: **PENDING**.

### Valid MP3 Decode Fault Stage

- User hardware result for the FatFs-tiny image: LCD displayed `HARDFAULT` /
  `MP3DECODE` after selecting `001/0001.ENC`.
- The relocated retained record therefore works and confirms the remaining
  reset occurs inside `MP3Decode()`, before `AUDIO INIT`, `PCM PREPARE`, or
  `DMA OUTPUT`.
- The supplied asset path `D:\001\0001.enc` was initially inaccessible because
  the removable drive was not mounted.

### Mounted ENC Asset Validation and Helix Substage Instrumentation

- The user mounted the SD/removable drive, making the exact failing asset
  `D:\001\0001.enc` available for read-only host analysis.
- File size: 428,013 bytes. SHA-256:
  `C261CFF4085067513016C3FF74E49773B22CCD17FE11BE51FC20FD52D13FB516`.
- Repeating `BK26` XOR decryption produces an ID3v2.4 header containing the
  encoder string `Lavf58.26.100`; the first MPEG frame begins at offset 45.
- Frame scan: **PASS**. The asset contains 743 contiguous mono MPEG-1 Layer III
  frames at 32 kHz and 128 kbps. Every frame is 576 bytes, no bytes were
  skipped between frames, and the last frame ends exactly at EOF.
- The file is within the wrapper's 1,024-byte encoded input-buffer limit and
  uses the already-enabled two-slot 1,152-sample MPEG-1 output path. The
  evidence rules out missing media, incorrect XOR phase, truncated content,
  unsupported channel count/rate, and oversized frames as the fault cause.
- Added persistent checkpoints inside `MP3Decode()` for header, side-info,
  main-data, scale-factor, Huffman, dequantization, IMDCT, and subband phases.
  The next boot after a target fault will display the exact phase on row 2.
- Managed MounRiver/WCH GCC build: **PASS**. Size is `text=44,572`,
  `data=712`, and `bss=19,664` bytes. FLASH use is 45,284 of 63,488 bytes;
  total RAM remains 20,376 of 20,480 bytes, leaving 104 bytes.
- Hardware result was pending at build time and is recorded in the next
  activity after the user completed the test.

### `DEC SUBBAND` Root Cause and Compact Mono Fix

- User hardware result for the fine-grained image: **FAIL**; LCD displayed
  `HARDFAULT` / `DEC SUBBAND` while playing `001/0001.ENC`.
- The completed header, side-info, main-data, scale-factor, Huffman,
  dequantization, and IMDCT checkpoints isolated the fault to the synthesis
  filterbank (`FDCT32()` / `PolyphaseMono()`).
- Root cause: setting `MAX_NCHAN=1` reduced `SubbandInfo.vbuf` to 1,088
  integers, but upstream mono synthesis continued using the two-channel
  interleaved block stride (64 integers) and mirrored-half offset (1,088).
  Exhaustive address calculation over all 18 subband blocks found accesses
  through index 2,127, beyond the allocation's last index 1,087.
- Added a compact mono layout: 32-integer block stride and 544-integer
  mirrored-half offset when `MAX_NCHAN=1`. The original constants remain in
  effect when `MAX_NCHAN=2`.
- Repeated the address calculation: **PASS**; the corrected mono FDCT and
  polyphase paths touch indices 0..1,071, leaving 16 integers of tail margin.
- RISC-V syntax/warning checks for affected files with both
  `HELIX_CH32_MAX_CHANNELS=1` and `=2`: **PASS**.
- Managed MounRiver/WCH GCC link: **PASS**. Size is `text=44,556`, `data=712`,
  and `bss=19,664` bytes. FLASH use is 45,268 of 63,488 bytes; total RAM
  remains 20,376 of 20,480 bytes, leaving 104 bytes.
- Hardware playback result is recorded in the next activity.

### Subband-Fixed Playback and Audio-Pipeline Diagnostics

- User hardware result: the compact-mono image displayed `PLAYING:` /
  `001/0001.ENC` without the previous `DEC SUBBAND` HardFault. No audio was
  audible, and no final playback status or underflow count was supplied.
- The validated 743-frame MPEG-1 file has an expected duration of about 26.75
  seconds, so the `PLAYING:` screen is normal during that interval. A screen
  that remains indefinitely can instead indicate that DMA never released the
  two PCM slots.
- Re-inspected all three schematic pages. PA8 is the `AUX` PWM source feeding
  an op-amp/filter and AC-coupled `AUX_OUT`; the schematic contains no speaker
  power amplifier. PA3/RLY_IN drives a separate external low-side relay output.
  PA3 remains idle because its external relay/load and rail have not been
  reported as independently measured.
- Added a 500 ms timeout to both PCM-slot waits and the final DMA drain. A
  stalled TIM2/DMA path now returns `CH32_MP3_AUDIO_TIMEOUT` and displays
  `DMA TIMEOUT` instead of remaining on `PLAYING:` indefinitely.
- Added bounded audio telemetry: maximum absolute decoded PCM sample (`P:`)
  and completed real DMA-slot count (`D:`), followed by the existing
  underflow screen. These values distinguish decoder silence, DMA failure,
  and an external analog/amplifier problem.
- Managed MounRiver/WCH GCC build: **PASS**. Size is `text=44,804`, `data=712`,
  and `bss=19,668` bytes. FLASH use is 45,516 of 63,488 bytes; total RAM use
  is 20,380 of 20,480 bytes, leaving 100 bytes.
- `main.c` syntax checks for diagnostic, MP3-test, and full profiles, plus
  `ch32_mp3_player.c` checks for MP3-test and full profiles: **PASS**.
- Hardware result for the audio-pipeline diagnostic image: **PENDING**.

## 2026-08-25

### Direct `mp3_song.mp3` Playback Image

- Confirmed the mounted SD asset at `D:\mp3_song.mp3` (2,880,985 bytes,
  SHA-256 `0CA7024D1AA17D4D6AD071C3B89FF82D0047F542D07EB3579CC07A9489E16A92`).
- Host frame scan: **PASS**. The 859,399-byte ID3v2 tag is followed by 9,673
  contiguous MPEG-1 Layer III frames at 44.1 kHz, 64 kbps, independent stereo;
  frame sizes are 208/209 bytes and duration is approximately 253 seconds.
- Changed the default MP3-test startup to open root `mp3_song.mp3` directly;
  UP replays the same file. Folder browsing, `.enc` selection, and the text
  pager are not part of this focused image.
- Added bounded ID3v2 seeking so embedded metadata/artwork is not fed through
  the MP3 frame scanner.
- Adapted the one-channel CH32 port to parse both independent-stereo side-info
  channels, decode the left channel, and skip the right channel's exact
  `part2_3` payload. Joint stereo and dual channel remain rejected.
- Fixed the Helix side-info handoff so each parsed `part23Length` reaches the
  decoder state used by Huffman processing.
- Reduced the input buffer from 1,024 to 768 bytes to accommodate two-channel
  side information within the 20 KB SRAM limit.
- Forced MounRiver/WCH GCC rebuild: **PASS**. Size is `text=43,560`,
  `data=712`, `bss=19,584`; FLASH use is 44,272/63,488 bytes and reported RAM
  use is 20,296/20,480 bytes, leaving 184 bytes.
- Target playback and audible `AUX_OUT` result: **PENDING**.

### First Direct-Playback Hardware Result and False-Sync Fix

- Hardware displayed `STEREO REJECTED` with `P:0 D:2`. This proves two DMA
  slots completed, but the first accepted PCM frame was silent and playback
  later stopped at an unsupported-looking sync candidate.
- The mounted file's real frames are all independent stereo. Therefore the
  rejected mode came from a false MP3 sync pattern in compressed payload or
  trailing data, not from the validated frame sequence.
- Changed the wrapper to advance one byte and resume scanning at such a
  candidate. Unsupported stereo is now returned only if no valid frame was
  decoded.
- Forced rebuild after the fix: **PASS** (`text=43,564`, `data=712`,
  `bss=19,584`; 184 bytes reported RAM margin). Target retest is required.

### Full-Buffer False-Header Recovery

- The next hardware image displayed `FRAME TOO LARGE`, `P:0 D:2`, and
  `UNDERFLOW:0`.
- This cannot describe a real frame in the validated asset: every real frame
  is 208/209 bytes and the firmware input buffer is 768 bytes.
- Corrected the full-buffer input-underflow branch to discard the false sync
  candidate byte and continue frame scanning rather than returning
  `CH32_MP3_FRAME_TOO_LARGE`.
- Forced rebuild: **PASS** (`text=43,572`, `data=712`, `bss=19,584`; 184 bytes
  reported RAM margin).
- Target retest: **PENDING**.

### Stale Firmware Image Identification and LCD Build Marker

- The reported target screen reverted to `PLAYING: / 001/0001.ENC`. The
  current ELF was inspected and contains `mp3_song.mp3` but no `001/0001`
  string, proving that screen came from an older image.
- Found an older firmware tree at
  `C:\Users\Elakkiya S\Desktop\SHAHIN\ch32v203\ch32v203_firmware`; its HEX is
  dated 2026-08-24. The active project/output for this work is
  `C:\OfficeWorks\ch32v203_firmware\Firmware\obj\CH32V203G8R.hex`.
- Restored LCD initialization/status reporting in the merged non-full
  `main.c`, removed relay activation from the focused image, and added the
  unique startup marker `MP3SONG BUILD / DIRECT MP3 TEST`.
- Forced rebuild: **PASS** (`text=49,500`, `data=816`, `bss=19,596`; 68 bytes
  reported RAM margin). The ELF contains `MP3SONG BUILD` and `mp3_song.mp3`
  and contains no `001/0001` string.
- HEX SHA-256:
  `FC433D51ED42662ACDD9AC19DDCCEA4B7050A29BB8ED718FCCFEA1E0DDDB75A2`.

## Current Direction

Flash the tone-plus-direct-play image. Confirm the 700 ms tone at the physical
speaker, then record the last startup checkpoint, MP3 status, `P:` value, `D:`
value, and underflow count. If the tone is absent, test the external
relay/amplifier path; if the tone works but MP3 is absent, continue decoder/DMA
diagnosis. In parallel, decide between a larger compatible MCU and a documented
feature/memory redesign for the complete application.

### Native-Mono Asset and End-to-End Audio Path Test

- The correct image reached `MP3SONG BUILD / DIRECT MP3 TEST` but did not show
  the subsequent playback screen, so the single 1,500 ms startup delay and SD
  entry were replaced by visible, non-blocking-style stage checkpoints.
- Re-read all three schematic pages. PA8 feeds the on-board filter/op-amp and
  line-level `AUX_OUT`; PA3 drives Q1/RLY_OUT for an external relay/amplifier.
  No speaker power amplifier is shown on the PCB schematic.
- Installed host-only `miniaudio` and `lameenc` tooling and converted the stereo
  SD asset to native mono MPEG-1 Layer III at 32 kHz/64 kbps. The original is
  preserved as `D:\mp3_orig.mp3`; the playable FAT 8.3 path remains
  `D:\mp3_song.mp3`.
- Converted-file validation: **PASS**. Size 2,021,184 bytes; SHA-256
  `233CACC745347BF021ADAF5D397747D7D04535806A1D9D03C37C6CC6A0AE3C8B`;
  7,018 contiguous mono frames of 288 bytes; host decode produced 8,084,736
  samples at 32 kHz, peak 32,768, and 8,068,766 nonzero samples.
- Added a 700 ms, 1 kHz PCM tone using the exact TIM1/TIM2/DMA/PA8 path. PA3 is
  enabled around the tone and MP3 playback and held LOW otherwise.
- Added LCD checkpoints: `AUDIO PATH TEST`, `MOUNTING SD`, `SD MOUNTED`,
  `FILE FOUND`, and `PLAYING:`.
- Forced build: **PASS** (`text=50,268`, `data=816`, `bss=19,596`; FLASH
  51,084/63,488 bytes; RAM 20,412/20,480 bytes, 68 bytes remaining).
- HEX SHA-256:
  `E66CD66BECAC3FD07AAAE2500845B4A52C2647472E87CA4B7219CA8CA338C28B`.
- MounRiver OpenOCD probe attempt: **BLOCKED BY HARDWARE CONNECTION**. The
  local programmer reported `WLink Open Error`; no WCH-Link device was present,
  so the new image could not be flashed or heard from this session.
- Physical tone and MP3 playback results: **PENDING**.

### Bare-Metal UART Startup Stall Removal

- Hardware remained on `MP3SONG BUILD / DIRECT MP3 TEST`, which places the
  stop before the following `AUDIO PATH TEST` screen.
- Removed retained-fault reporting and all blocking UART initialization/prints
  from the non-FreeRTOS audio startup. The first `printf()` waited indefinitely
  for USART transmit-complete on this hardware run.
- Changed the unique image marker to `TONE+MONO V2 / PA3 AMP ENABLE`; LCD stage
  reporting is now the only bring-up diagnostic before playback.
- Forced UART-free rebuild: **PASS** (`text=43,276`, `data=688`,
  `bss=19,584`; FLASH 43,964/63,488 bytes; RAM 20,272/20,480 bytes, leaving
  208 bytes).
- HEX SHA-256:
  `026EBE4E5D03A2C0CF81B27E3331D8318304F04D5CAC2B3E39B9AE356814360E`.

### Dual Digital/Analog Path Isolation

- The UART-free image reached the V2 marker, DMA tone, SD mount, and
  `PLAYING: / MP3_SONG.MP3`, but neither tone nor song was audible.
- Confirmed the ESP reference asserts its relay GPIO HIGH before playback;
  this matches PA3 HIGH turning on the schematic's Q1 low-side RLY_OUT driver.
- Added a three-second 500 Hz direct GPIO square wave on PA8 while PA3 is HIGH.
  This bypasses TIM1, TIM2, DMA, Helix, and SD.
- Extended the DMA/PWM tone to three seconds and added an LCD result containing
  its generated peak and completed-slot counter before SD mounting.
- If both tones complete and are silent, firmware decoding is excluded from
  the silence diagnosis; PA8/AUX_OUT and the external amplifier/speaker path
  require electrical measurement.
- Forced rebuild: **PASS** (`text=43,552`, `data=688`, `bss=19,584`; FLASH
  44,240/63,488 bytes; RAM 20,272/20,480 bytes, leaving 208 bytes).
- HEX SHA-256:
  `7E2357DE150FC805C266F05F9B27D1E079147E73081C16940BE9845836E7BBB8`.

### TIM1 Advanced-Output Initialization Fix

- Hardware result: raw PA8 500 Hz tone **AUDIBLE**; TIM1/DMA 1 kHz tone
  **INAUDIBLE**; generated telemetry `P:20000 D:167`.
- This proves PA8, AUX_OUT, PA3, the external amplifier, and speaker work, and
  that TIM2/DMA transfers complete. The fault is isolated to TIM1 PWM output.
- Found `TIM_OCInitTypeDef pwm` was not initialized before `TIM_OC1Init()`.
  WCH's TIM1 implementation also reads `TIM_OutputNState`, `TIM_OCNPolarity`,
  `TIM_OCIdleState`, and `TIM_OCNIdleState`, so stack garbage could alter the
  advanced timer's PA8 output configuration.
- Added `TIM_OCStructInit(&pwm)`, explicit default TIM1 remap selection, and
  TIM1/TIM2/DMA1 Channel2 resets before every audio-path initialization.
- Forced rebuild: **PASS** (`text=44,224`, `data=688`, `bss=19,584`; FLASH
  44,912/63,488 bytes; RAM 20,272/20,480 bytes, leaving 208 bytes).
- HEX SHA-256:
  `39945294BD7C1BA1CF45598337685859321AC3C87AC533261423D4CFD1797F71`.
- Hardware retest: **PENDING**.

### Audible DMA Tone and Level Increase

- Hardware retest after deterministic TIM1 initialization: **PASS**. The
  DMA-driven 1 kHz tone is audible through the physical speaker, proving the
  PA8 PWM carrier and compare updates now reach the analog path. The reported
  volume was too low.
- Increased the diagnostic PCM tone from +/-20,000 to +/-32,767 so it exercises
  almost the full 0-255 PWM-duty range.
- Added a clipped 4x digital preamp after caller-volume scaling for MP3 PCM.
  Values saturate at -128..127 before the +128 PWM midpoint, preventing wrap.
- Forced rebuild: **PASS** (`text=44,240`, `data=688`, `bss=19,584`; RAM
  margin 208 bytes). HEX SHA-256:
  `CB7FD1566B4E46FA47E9F3FE2D02EEA14A87692D5FB629F90B0F3590E01FE85E`.
- Loudness/distortion and song playback retest: **PENDING**.

### Distortion Result and Unity-Gain Correction

- Hardware result: the full-scale DMA tone is loud and `mp3_song.mp3` is now
  audible, confirming end-to-end SD/decoder/PWM/amplifier/speaker operation.
- Tone telemetry was `P:32767 D:167` as expected for the three-second
  full-scale diagnostic.
- Song audio was reported as heavily distorted. Because the converted source
  itself reaches peak 32,768, the temporary 4x preamp clipped much of its PCM.
- Returned the digital preamp to unity while retaining signed saturation and
  the full-scale diagnostic tone. Clean-audio hardware retest: **PENDING**.
- Forced rebuild: **PASS** (`text=44,216`, `data=688`, `bss=19,584`; RAM
  margin 208 bytes). HEX SHA-256:
  `AC76D9112C30AEDFBB7D080B16FBAE359A16A3EB59B8BC8498019FEF4D7ED08E`.

### Actual TLV9061 Circuit and Gain-Aware Test Image

- The owner supplied the actual analog circuit: a TLV9061 powered from 3.3 V,
  configured as a Sallen-Key-style active filter with two 1.2 kOhm series
  resistors and a non-inverting gain of 2 from the 1.2 kOhm/1.2 kOhm feedback
  divider. The input and `AUX_OUT` are each AC-coupled by 1 uF.
- The drawing labels one shunt and two parallel feedback capacitors as 4.7 uF
  and shows no explicit 1.65 V single-supply reference. These are recorded as
  hardware-verification items, not assumed PCB facts.
- Calculated from the shown populated values, the two parallel feedback
  capacitors produce 9.4 uF and an ideal natural frequency of approximately
  19.95 Hz with the 4.7 uF shunt capacitor and 1.2 kOhm resistors. If the
  intended unit is nF, the same topology scales to approximately 19.95 kHz.
  The gain/component ratio also places the ideal damping term at or near zero,
  so PCB population, response, and stability require measurement.
- Reduced `HAPPYBELL_MP3_TEST_VOLUME` from 255 to 96 and applied the same
  bounded volume to the DMA tone. The unity digital preamp and signed
  saturation remain. At a full-scale PCM peak, the new setting requests about
  0.62 V peak at the filter input and nominally 1.24 V peak after analog gain
  2, leaving output-swing headroom on 3.3 V.
- Added the unique LCD marker `TLV9061 AUDIO V3 / GAIN2 VOL96`.
- Managed MounRiver/WCH GCC build: **PASS** (`text=44,220`, `data=688`,
  `bss=19,584`; FLASH 44,908/63,488 bytes; RAM 20,272/20,480 bytes, leaving
  208 bytes). HEX SHA-256:
  `F8B2E4597EA9BF3CD7E800A5894C845A33E8E68B5C0C4B7295052C2891F3C60C`.
- Clean-audio target retest and TLV9061 DC/waveform measurements: **PENDING**.

### Volume-96 Hardware Result and Full-Volume Restoration

- Hardware result for `TLV9061 AUDIO V3 / GAIN2 VOL96`: the song played but
  retained the same distortion; both the song and DMA PWM test were too quiet.
- Reported LCD telemetry: `P:32767 D:167`. In the current startup sequence this
  screen follows the three-second DMA tone; 167 completed 576-sample slots is
  consistent with that generated 32 kHz test path.
- Per owner instruction, restored `HAPPYBELL_MP3_TEST_VOLUME` to `255U`. The
  DMA tone uses the same macro, so both tone and song return to full digital
  level. Unity digital preamp and signed PWM saturation remain unchanged.
- Updated the unique LCD marker to `TLV9061 AUDIO V4 / FULL VOL255`.
- Managed MounRiver/WCH GCC build: **PASS** (`text=44,216`, `data=688`,
  `bss=19,584`; FLASH 44,904/63,488 bytes; RAM 20,272/20,480 bytes, leaving
  208 bytes). HEX SHA-256:
  `8CDD5BB384FC9C6DA8780A4FDE6624D6BD49133DF8E71D53606F4F9651BAE838`.
- Full-volume hardware retest and analog distortion measurements: **PENDING**.

### Full-Volume Result and Mono Polyphase Stride Fix

- Hardware result for `TLV9061 AUDIO V4 / FULL VOL255`: the DMA PWM test is
  loud, proving the full-level TIM1/TIM2/DMA path reaches the speaker. MP3
  playback is active but remains too quiet and too distorted to understand.
- This difference isolates a PCM-content/decoder issue from basic DMA carrier
  generation; the full-scale square-wave tone is not a decoder-quality test.
- Audited the compact mono synthesis history. `FDCT32()` and most
  `PolyphaseMono()` addressing used `VBUF_BLOCK_STRIDE=32`, but the main
  convolution still began at literal `vbuf + 64`, inherited from upstream's
  stereo-interleaved layout. It therefore read the next compact history block
  for 30 of each 32 output samples and generated corrupted PCM while continuing
  to report valid peaks and DMA completions.
- Replaced the literal start with `vbuf + VBUF_BLOCK_STRIDE`. The two-channel
  configuration expands that macro back to 64, so upstream layout compatibility
  is preserved. Normal one-channel build and an explicit
  `HELIX_CH32_MAX_CHANNELS=2` syntax check: **PASS**.
- Updated LCD marker: `MONO PCM FIX V5 / FULL VOL255`.
- Managed MounRiver/WCH GCC build: **PASS** (`text=44,212`, `data=688`,
  `bss=19,584`; FLASH 44,900/63,488 bytes; RAM 20,272/20,480 bytes, leaving
  208 bytes). HEX SHA-256:
  `1EC0DD68D629FE4664F0C8D30B9ED9E1B7CC70BEAA36600C094A02237267E382`.
- Corrected-PCM hardware result: **PENDING**.

### V5 Result and Sine/Arithmetic Isolation Image

- Hardware result after the V5 mono polyphase start correction: MP3 audio
  remained too quiet and too distorted to hear properly. This proves that the
  corrected history offset was not the only remaining fault.
- The existing loud DMA diagnostic was a full-scale square wave. Because a
  square wave is intentionally harmonic-rich, loudness alone cannot validate
  reconstruction-filter or amplifier linearity.
- Added `CH32_MP3_ArithmeticSelfTest()` with known negative signed
  `MULSHIFT32` and `MADD64` results. Boot now displays `ARITH PASS` or stops on
  `ARITH FAIL` before mounting SD.
- Replaced the DMA square-wave PCM generator with a full-scale, 32-entry sine
  lookup: 32 kHz / 32 samples = 1 kHz. The lookup is const/flash-resident and
  adds no SRAM use.
- Updated LCD marker: `SINE+ARITH V6 / FULL VOL255` and diagnostic label
  `DMA SINE TEST`.
- Managed MounRiver/WCH GCC build: **PASS** (`text=44,444`, `data=688`,
  `bss=19,584`; FLASH 45,132/63,488 bytes; RAM 20,272/20,480 bytes, leaving
  208 bytes). The mono build and explicit two-channel compatibility syntax
  check pass. HEX SHA-256:
  `084534BC3414E322DC245A59E8973BA7F70DC9CC8D59746FF4B432DA2E675571`.
- Arithmetic LCD result, sine quality, and MP3 comparison: **PENDING**.

### V6 Isolation Result: Analog Reconstruction Failure

- Fixed-point LCD result: **PASS** (`ARITH PASS`). The tested signed RISC-V
  `MULSHIFT32` and `MADD64` primitives match their fixed expected results.
- Decoder-free DMA sine result: **FAIL**. The full-scale 1 kHz sine is loud but
  heard as buzzing instead of a clean steady tone.
- MP3 result: **FAIL**. Playback runs, but the song remains too quiet and too
  distorted to hear properly.
- Because the sine bypasses FatFs and Helix while using the same PCM scaling,
  TIM1/TIM2/DMA, PA8, TLV9061, `AUX_OUT`, and downstream speaker path, its
  distortion proves that clean MP3 output cannot be achieved by further
  decoder or volume changes alone.
- Next action is electrical measurement and correction of the actual TLV9061
  stage: verify capacitor units/population, idle DC at pins 1/3/4, mid-supply
  bias, pre-coupling sine waveform, output clipping, and filter stability.

### V7 Decoder Reference Audit and MPEG-1 Granule Handoff

- Regenerated the exact current SD-root mono asset from the retained source.
  The generated file is 2,021,184 bytes with SHA-256
  `233CACC745347BF021ADAF5D397747D7D04535806A1D9D03C37C6CC6A0AE3C8B`,
  matching the documented `mp3_song.mp3` exactly.
- Added `tools/helix_host_decode.c` and the minimum CH32 fault-diagnostic host
  stub, then compiled the untouched `esp_bell_idf` Helix reference and the
  compact CH32 port as independent Windows host decoders.
- Both decoders completed all 7,018 mono MPEG-1 frames at 32 kHz and produced
  8,084,736 signed-16 samples. Both PCM files have SHA-256
  `ADC72D6CD76A89317DAD3BD107764806C3F5808BF1DB715CA1D9E2167AA36F4A`.
  Sample comparison found zero differences, maximum absolute difference 0,
  correlation 1.0, and identical RMS 8,481.2616. The compact Helix decoding
  result is therefore bit-identical to the untouched reference for this asset.
- The audit exposed undefined signed left shifts in the portable/RISC-V
  `CLZ()` and RISC-V `MADD64` C assembly. `CLZ()` now shifts an unsigned work
  value; `MADD64` assembles its signed high word through defined arithmetic.
- The one-channel build still compiled unreachable stereo dequantization and
  subband branches that indexed channel 1 outside `MAX_NCHAN=1` arrays. Those
  paths are now compiled only when `MAX_NCHAN > 1`. The normal mono build and
  explicit `HELIX_CH32_MAX_CHANNELS=2` host decode both remain PCM-identical to
  the reference.
- Found a separate decode-to-DMA continuity defect: every MPEG-1 frame waited
  for both 576-sample slots to empty before decoding the next 1,152 samples,
  guaranteeing a silence gap once per frame. Added the optional
  `MP3DecodeWithGranuleHook()` API while preserving the original
  `MP3Decode()`. The wrapper now prepares and queues each synthesized granule,
  begins the next frame as soon as row 0 is free, and waits for row 1 only
  immediately before overwriting it. No additional static RAM is used.
- Host hook test: 14,036 before events and 14,036 after events across the exact
  song; output remained byte-for-byte identical to the reference PCM.
- Updated LCD marker: `DECODER FIX V7 / FULL VOL255`.
- Forced MounRiver/WCH GCC rebuild: **PASS with no warnings**.
  `text=38,896`, `data=688`, `bss=19,584`; FLASH 39,584/63,488 bytes; RAM
  20,272/20,480 bytes, leaving 208 bytes. No 64-bit multiply/shift helper was
  linked. HEX SHA-256:
  `31EA8EDCA300B9ECD920469200491A5AE1DB5880D6B57990EFB339C863ECD007`.
- Target-compiler stack report: `CH32_MP3_PlayFile` 176 bytes,
  `MP3DecodeInternal` 96 bytes, and `Audio_GranuleOutputHook` 16 bytes; the
  reserved main/ISR stack remains 2,304 bytes.
- V7 physical-speaker quality and underflow result: **PENDING**. The prior
  buzzing decoder-free sine remains independent evidence that the TLV9061/PWM
  reconstruction path also requires electrical verification.

### V7 Hardware Result and V8 Internal-Clock Correction

- V7 hardware result: playback improved slightly after the granule handoff,
  but remained distorted and sounded slow. The reported `P:32767 D:167`
  matches the preceding three-second 32 kHz sine diagnostic (96,000 samples /
  576 = 166.67 slots), so it is not yet a complete-song DMA/underflow result.
- Rechecked all three pages of `happybell_schematic.pdf`. The CH32V203G8R6
  symbol and PCB circuitry show no external HSE crystal or oscillator
  connection. The only crystal shown is the DS1307's 32.768 kHz RTC crystal.
- Firmware nevertheless selected `SYSCLK_FREQ_96MHz_HSE`. WCH's initialization
  routine times out if HSE is absent and does not switch to the PLL, which can
  leave the MCU at the approximately 8 MHz reset HSI clock. At that clock,
  TIM1 ARR 255 produces about 31.25 kHz PWM instead of 375 kHz and Helix has
  only one-twelfth of the intended CPU throughput, explaining the low-quality
  carrier and slow/gapped playback without contradicting the host PCM match.
- Confirmed from WCH's CH32FV2x/V3x clock tree that APB1 is 48 MHz under the
  configured divide-by-two prescaler and the TIM2 counter clock is
  automatically doubled to 96 MHz. Therefore the existing
  `SystemCoreClock / sampleRate` reload formula is correct once the 96 MHz PLL
  is actually running.
- Switched `system_ch32v20x.c` from `SYSCLK_FREQ_96MHz_HSE` to WCH's supported
  `SYSCLK_FREQ_96MHz_HSI` path (internal 8 MHz HSI x12 PLL).
- Added an early runtime guard: the MP3 test halts with
  `CLOCK ERROR / NOT 96MHZ` unless `SystemCoreClockUpdate()` reports
  96,000,000 before the arithmetic, tone, SD, or MP3 stages.
- Updated LCD marker: `HSI CLOCK V8 / 96MHZ VOL255`.
- Forced MounRiver/WCH GCC rebuild: **PASS with no warnings**.
  `text=38,916`, `data=688`, `bss=19,584`; FLASH 39,604/63,488 bytes; RAM
  20,272/20,480 bytes, leaving 208 bytes. HEX SHA-256:
  `D01753602C3A59C7D3FAFA6A6659C1CDCB5855D7F6FD09E736D6F16CACAFD643`.
- V8 carrier frequency, sine quality, playback speed, song counters, and
  audible distortion retest: **PENDING**.

### V8 Physical Speaker Playback

- User-reported hardware result: the V8 image now plays audio through the
  device's physical speaker.
- Physical speaker audio path: **PASS**.
- The result confirms that the corrected internal-HSI clock build reaches MP3
  playback without stopping at the V8 clock guard.
- Carrier frequency, playback speed, intelligibility/distortion, completed-song
  `P:`/`D:`, and underflow count were not reported and remain **PENDING**.

### V8 Playback Quality Confirmation

- Follow-up user report: plain `mp3_song.mp3` now plays at normal speed through
  the physical speaker with no audible distortion.
- Playback speed/pitch: **PASS**.
- Audible distortion check: **PASS**.
- This confirms that selecting the internal-HSI 96 MHz clock resolved the
  previously slow/distorted target behavior without changing the proven Helix
  PCM output.
- Completed-song `P:`/`D:`, underflow count, and electrical carrier/filter
  measurements were not supplied and remain pending.

### V9 Encrypted-Audio Format Audit and Firmware Integration

- Re-read the original ESP implementation at
  `esp_bell_idf/main/driver/mp3_player/mp3_player.cpp`. Its ENC format is the
  complete MP3 file XORed from byte zero with repeating ASCII `BK26`; it has no
  IV, salt, nonce, padding, authentication, wrapper header, or external
  metadata.
- Mounted-card inventory found 1,586 `.enc` files across the numbered folders.
  Folder `001` contains 25 files totaling 12,582,693 bytes.
- Fully decrypted and frame-scanned every `001/*.enc` file: all 25 have a
  45-byte ID3v2.4 tag (`TSSE=Lavf58.26.100`) followed by mono MPEG-1 Layer III,
  32 kHz, 128 kbps, 16-bit decoded PCM, 576 compressed bytes/frame, and 1,152
  PCM samples/frame. All 21,843 frames are contiguous and end exactly at EOF.
- `001/0001.enc` verification: encrypted SHA-256
  `C261CFF4085067513016C3FF74E49773B22CCD17FE11BE51FC20FD52D13FB516`;
  decrypted MP3 SHA-256
  `A03F88121ACC627FDBC2219BC838D59C7F05390F0AAD3336DCE9770F3868BC66`;
  743 frames; XOR-twice round-trip reproduced the original encrypted bytes.
- Added a bounded FatFs extension finder. V9 scans folder `001`, selects the
  alphabetically first case-insensitive `.enc` short filename, auto-plays it,
  uses UP for ENC replay, and uses DOWN for root `mp3_song.mp3` A/B playback.
- Replaced mutable XOR-phase state with absolute-offset decryption. Buffer
  refills, short reads, ID3 seeks, and retained bytes can no longer shift the
  repeating key.
- Added encrypted/plain ID3 and MPEG preflight validation; decoded stream
  parameter and mid-stream format checks; and distinct visible errors for SD
  reads, invalid ENC, unsupported format, corruption, oversized frames,
  unsupported rate/stereo, invalid audio clock, and DMA timeout.
- Added LCD reporting for sample rate, decoded bit depth, channel count,
  bitrate, MPEG layer/version, compressed frame bytes, and samples per frame.
- Preserved HSI x12 and APB1 `/2`. The folder-`001` 32 kHz stream uses exactly
  3,000 TIM2 ticks/sample. Added a bounded fractional-period accumulator for
  44.1/22.05/11.025 kHz; mathematical simulation of every standard rate from
  8-48 kHz produced zero long-term sample-rate error. BCLK/MCLK are not used
  because the output is PWM, not I2S.
- All three build-profile syntax checks: **PASS**.
- Forced MounRiver/WCH GCC V9 build: **PASS with no warnings**.
  `text=41,328`, `data=688`, `bss=19,620`; FLASH 42,016/63,488 bytes; RAM
  20,308/20,480 bytes, leaving 172 bytes. HEX SHA-256:
  `9DBD7C065A916B8D7A63C6A1755990AFF2E6F5896A503FACDA0BC0DBF2412380`.
- V9 target stack report: `CH32_MP3_PlayFile` 192 bytes, `main` 80 bytes,
  extension finder 128 bytes; reserved main/ISR stack remains 2,304 bytes.
- Command-line OpenOCD flash attempt: **BLOCKED BY HARDWARE CONNECTION** with
  `WLink Open Error`. ENC physical-speaker playback, metadata screens, and
  final counters/underflows remain **PENDING**.

### V9 Encrypted Playback Hardware Confirmation

- The user subsequently confirmed that `.enc` file playback is working
  correctly on the physical device.
- SD read -> absolute-offset `BK26` XOR -> Helix decode -> exact-rate TIM2/DMA
  PWM -> amplifier/speaker path: **PASS**.
- Exact completed-song `P:`/`D:`, underflow count, and metadata screens were not
  supplied and remain pending as production telemetry, but encrypted playback
  itself is no longer pending.

### V10 Plain-MP3 Gain and Production Test Cleanup

- New hardware request: preserve the working ENC path, raise low-level
  downloaded plain MP3 files, prevent clipping/distortion/noise, and completely
  remove the RAW PA8 and DMA sine-wave diagnostics.
- Kept the caller volume at the owner-selected `255U`. Added plain-`.mp3`-only
  Q8 block gain in `Audio_PrepareSlot()`: 1x to 8x (+18 dB maximum), target
  peak 28,672, +0.125x maximum rise per 576-sample block interpolated across
  its samples, immediate reduction before loud material, and no gain tracking
  below decoded peak 512. Final
  signed/PWM saturation remains active.
- ENC files bypass the new gain and retain the V9 unity conversion. Exhaustive
  host arithmetic compared all 65,536 signed-16 input values and found zero
  output differences between the old and new ENC conversion. A modeled
  full-scale transient returned gain to 1x before PWM conversion.
- Removed `CH32_MP3_PlayRawPinTone()`, `CH32_MP3_PlayTestTone()`, their public
  declarations, the full-scale sine lookup, test-only locals and loops, relay
  activation for tones, and all RAW/DMA tone LCD screens and startup delays.
  Source and ELF searches found no remaining test-tone functions or data.
- The focused image now identifies itself as
  `ENC+MP3 V10 / AUTO GAIN VOL255`, runs the retained clock and arithmetic
  guards, then proceeds directly to SD mounting and file playback.
- Diagnostic, MP3-test, and full-profile source syntax checks: **PASS**.
- Forced MounRiver/WCH GCC V10 build: **PASS with no warnings**.
  `text=40,748`, `data=688`, `bss=19,620`; FLASH 41,436/63,488 bytes; RAM
  20,308/20,480 bytes, leaving 172 bytes. HEX SHA-256:
  `520F7F20DB3666AF80062BE461835F91593435AA090BC5B71C04CC92BA6AF446`.
- V10 target stack report: `CH32_MP3_PlayFile` 272 bytes and `main` 80 bytes;
  the reserved main/ISR stack remains 2,304 bytes.
- Direct V10 OpenOCD programming attempt was blocked with `WLink Open Error`.
  The SD volume was also not mounted in the host session, so the new downloaded
  MP3 assets could not be measured offline.
- V10 physical listening result for plain-MP3 loudness, clarity, clipping,
  pumping/noise, and ENC regression: **PENDING**.

## 2026-08-26

### `esp_bell_idf` Source-of-Truth Flow Audit and CH32 Parity Update

- Audited the reference project at `C:\OfficeWorks\esp_bell_idf`, including
  root/component build configuration, `main.cpp`, `bell.cpp`, `menu.cpp`,
  `mydef.h`, `config.hpp`, and the astro, EEPROM, keypad, LCD, local-MP3, RTC,
  storage, and radio-only driver paths.
- Mapped the reference initialization sequence, idle key handling, setup-menu
  chain, 600 ms schedule evaluation, holiday gate, playlist types `1`/`2`/`3`,
  duration handling, sequence tokens, group rotation, Panchang announcements,
  relay delays, one-second light scheduling, settings/menu formats, and local
  MP3/ENC playback onto the CH32 full-profile source.
- Confirmed the CH32 application/build has no radio driver, credential, online
  time-sync, callback, state-machine, task, or dependency. Removed the last
  explicit online-stream token branch; unsupported tokens are now consumed and
  ignored safely rather than being treated as local audio paths.
- Aligned `settings.txt` behavior to the reference welcome prefix
  (`HappyBell 2025 - `) and full timezone parser validation. Restored
  case-preserving `menu.txt` labels.
- Aligned the Tamil solar-month calculation to the current reference rule:
  sidereal Sun sign at local sunset. Retained the necessary FatFs adaptations
  for `special.txt` and `festival.txt`.
- Restored the standalone diagnostic profile, which had unintentionally fallen
  through to the MP3-test implementation. It again checks the DS1307, mounts
  FatFs, and pages root `TEST.TXT`; it does not initialize PA8 audio.
- Added full-profile RTC serialization around high-level reads/writes. The
  light task now performs an independent DS1307 read once per second and leaves
  the previous GPIO state unchanged on an RTC read failure, preventing stale
  bell-task time and concurrent I2C transactions.
- Preserved CH32-specific requirements that cannot be copied directly: direct
  SD/FatFs control-file reads instead of SPIFFS staging, rotating internal-flash
  settings instead of NVS, WCH GPIO/I2C/SPI drivers, PA8/TIM1 + TIM2/DMA audio
  instead of ESP I2S, and FreeRTOS tasks created from WCH `main()`.
- The verbose reference Panchang UART dump and unused MCU LED helper were not
  copied. They are diagnostic-only, no MCU-controlled LED pin is confirmed on
  the target schematic, and the full image already exceeds device capacity.

### Strict Parity Closure

- Extended the audit to the reference build/Kconfig files, Panchang sample and
  host-reference assets, and Helix wrapper/test tree. These auxiliary files do
  not add runtime behavior; the required Panchang calculations and SD file
  formats are already represented by the CH32 Astro module and documentation.
- Added `LCD_CreateChar()` and restored the exact reference eight-byte setup
  cursor in HD44780 CGRAM slot 2. Date/time editors again display character 2
  instead of the earlier `>` substitute.
- Restored the reference one-tick bell idle yield, 256-byte schedule/sequence
  buffers, the extra 100 ms fixed-announcement settling delay, and the
  playback-row clearing/order used by the local schedule path.
- Corrected `KEY_WaitKey()` for the retained bare-metal profiles: the full
  profile yields with FreeRTOS, while diagnostic/MP3 profiles use `Delay_Ms()`
  and never call `vTaskDelay()` before a scheduler exists.
- Removed stale documentation claims that the reference-style full profile
  can enter the blocking `TEST.TXT` pager. That viewer is standalone
  diagnostic functionality only.
- Repeated the radio exclusion scan over CH32 application and managed-build
  inputs. No WiFi/Bluetooth driver, credential, callback, state, task,
  streaming token handler, NTP path, or dependency is present. The only text
  matches are unrelated website URLs in upstream MCU/FreeRTOS comments.

### Build and Capacity Verification

- Standalone diagnostic profile: **LINK PASS**, `text=8,576`, `data=48`,
  `bss=5,284` bytes.
- Default V10 ENC/MP3 profile: **LINK PASS**, warning-free,
  `text=40,748`, `data=688`, `bss=19,620` bytes. The output HEX SHA-256 remains
  `520F7F20DB3666AF80062BE461835F91593435AA090BC5B71C04CC92BA6AF446`,
  confirming that the working default audio image is unchanged.
- Full FreeRTOS/Astro/MP3 profile: every source compiled, but final link remains
  **FAIL - TARGET CAPACITY**. Loadable flash is 117,400 bytes, exceeding the
  reserved 63,488-byte application region by 53,912 bytes. Static data through
  BSS reaches 26,124 bytes; adding the fixed 2,304-byte main/ISR stack requires
  28,428 bytes, exceeding 20 KB RAM by 7,948 bytes.
- Hardware execution of the full reference-style flow was not claimed. A
  larger compatible target or an explicitly approved feature reduction remains
  required before full-flow hardware validation.

### Immutable Helix Table Placement and Capacity Re-audit

- Audited the complete current working tree, both build profiles, linker
  layout, linked symbols, logging sites, interrupt/startup references, FreeRTOS
  heap/tasks, FatFs state, Helix buffers, and Panchang/math-library footprint.
- Marked the dequantizer and IMDCT lookup tables in `dqchan.c` and `imdct.c`
  `static const`. No code writes these 640 bytes; every table value and lookup
  remains identical. This moves them from startup-copy `.data` to read-only
  flash and does not change the loadable flash byte count.
- Production mono and explicit `HELIX_CH32_MAX_CHANNELS=2` syntax builds pass
  with `-Wall -Wextra -Werror`. The changed lines contain no logging, register,
  volatile, interrupt, callback, control-flow, or arithmetic modification.
- Refreshed default MP3-test link: **PASS**, 41,588 bytes loadable flash and
  19,672 bytes RAM including the fixed stack, leaving 808 bytes. Relative to
  the same source/link order before the qualifier change, flash is unchanged
  and RAM is reduced by exactly 640 bytes.
- Refreshed standalone diagnostic link: **PASS**, 8,580 bytes loadable flash
  and 5,332 bytes RAM.
- Refreshed complete FreeRTOS/Astro/MP3 link: every source compiles, but final
  link remains **FAIL - TARGET CAPACITY**. It requires 117,704 bytes loadable
  flash (54,216 bytes over 63,488) and 27,792 bytes RAM including the fixed
  2,304-byte main/ISR stack (7,312 bytes over 20,480).
- Exploratory LTO reduced the full loadable flash footprint by only about
  4.1 KB and RAM by about 0.3 KB, still far from fitting. It was not enabled
  because whole-program inlining/reordering is not provably timing-neutral for
  this timing-sensitive firmware.
- Larger proposed savings require decoder buffer/lifetime redesign, a smaller
  astronomy/numeric implementation, code overlays, reduced RTOS allocations,
  or feature removal. Those can affect PCM/timing, numeric results, error paths,
  storage dependencies, or supported behavior and were intentionally not
  applied under the strict parity requirement.

### FreeRTOS Removal and Cooperative Full-Profile Optimization

- Audited the complete task architecture before changing it: bell priority 2
  with a 20 ms idle delay and 600 ms schedule cadence; light priority 1 with an
  independent one-second RTC read; audio priority 4 woken by a binary semaphore;
  a shared RTC mutex; and the decoder's existing 1 ms DMA-slot wait yields.
- Replaced the scheduler tick with a 1 kHz TIM3 counter. Its fast ISR only
  clears update state and increments `milliseconds`, at a lower priority than
  TIM2/DMA audio interrupts.
- Replaced bell/light/audio tasks with cooperative services. Bell/menu remains
  at 20 ms and schedule/display at 600 ms. Light performs the same independent
  RTC read and GPIO decision once per second, with the interval starting after
  each service completion as the old task delay did. Audio starts only after a
  request instead of waiting in a permanent task.
- Kept Helix decode blocking, matching the former high-priority audio task.
  During each existing 1 ms DMA wait, the cooperative hook services light and
  the active stop condition: UP/deadline every 10 ms for scheduled playback or
  all keys every 100 ms for menu preview. A one-event key queue preserves the
  event for the caller after decode returns.
- Removed application task creation/start, tick APIs, delay APIs, task handles,
  binary semaphore, RTC mutex, kernel error hooks, task stacks, RTOS heap, and
  scheduler/context-switch linkage. `.cproject` and both `.wvproj` files
  identify a NoneOS build, exclude `Firmware/FreeRTOS`, and contain no FreeRTOS
  include path. The archived vendor tree remains for license/history purposes
  and is guarded for stale generated makefiles.
- Moved an additional 56 bytes of immutable clock, ADC, XOR-key, and Huffman
  metadata from startup-copy data to explicit read-only sections. The tables
  have no writers and retain identical types, values, indices, and results.
- Consolidated the separate 12-byte time and 16-byte date formatting arrays
  into one 16-byte RTC text buffer. Every audited caller formats and consumes
  one string before requesting the other, so output text and call order are
  unchanged while static RAM falls by another 12 bytes.
- Full ELF audit found no task/queue/semaphore/heap/context-switch symbol. Only
  the WCH startup's normal weak `SysTick_Handler` and `SW_Handler` remain.
- WCH GCC profile builds excluding the kernel: diagnostic **LINK PASS**, 8,616
  bytes loadable flash and 5,292 bytes RAM; default MP3 **LINK PASS**, 41,616
  bytes loadable flash and 19,632 bytes RAM, leaving 848 bytes; full source
  **COMPILE/LARGE-LINK PASS** but G8R6 capacity link **FAIL**, requiring 107,160
  bytes flash and 21,252 bytes RAM including the 2,304-byte stack. Exact full
  overflow is 43,672 bytes flash and 772 bytes RAM.
- Relative to the fresh pre-optimization full image (117,704 bytes flash,
  28,432 bytes RAM), the cooperative/read-only result saves exactly 10,544
  bytes flash (8.96%) and 7,180 bytes RAM (25.25%). FreeRTOS removal accounts
  for the major reduction; immutable data placement accounts for 696 bytes of
  the total RAM reduction across both table-placement stages.
- A stricter `-Wall -Wextra` all-source compile reports only the existing
  unused-parameter warnings in astronomy/debug/Helix and one existing Helix
  signedness warning. Changed runtime/table sources compile cleanly. The
  configured managed warning set passes.
- Exploratory cooperative LTO produced 103,872 bytes loadable flash but raised
  RAM to 22,332 bytes; it still cannot fit and was not enabled because it also
  permits whole-program inlining/reordering of timing-sensitive code.
- Intentionally did not shrink the 2,304-byte stack (a smaller stack previously
  faulted on hardware), decoder input/PCM/history buffers, FatFs volume window,
  Panchang structures, numeric/parser/formatter libraries, astronomy tables,
  logs, or error paths. Those changes can alter supported files, PCM samples,
  arithmetic rounding, SD/decode timing, stack safety, formatting, or visible
  behavior. No full-profile hardware behavior is claimed until capacity is
  resolved and the cooperative timing/flow regression checklist is run.

### Aggressive Fixed-Format, LTO, and RAM Closure Pass

- Replaced every live `snprintf()`/`sscanf()`/`atoi()`/`atof()`/`strtod()` use
  in the full profile with bounded project-local fixed-format builders and
  documented decimal/list parsers. LCD strings, filenames, zero padding,
  welcome truncation, and accepted timezone forms remain represented without
  pulling formatted-I/O, locale, allocator, or reentrancy code into the image.
- Enabled LTO with `-msmall-data-limit=0`. The zero small-data limit is
  deliberate: LTO with the old value 8 moved 1,080 bytes of constants into
  startup-copied SRAM; the selected combination retains them in Flash.
- Removed the unmodified 256-byte schedule copy, reused one settings line
  buffer at the original per-field input limits, reused `Storage_ReadLine()`
  for astronomy data, reduced the repeated 32-sample silence array to one DMA
  sample while preserving a 32-transfer circular completion interval, and
  replaced a 296-byte Panchang temporary with two direct integer outputs.
- Reduced the main/ISR reservation from 2,304 to 2,048 bytes only after the
  256-byte object was removed from the deepest live schedule call chain.
  Static RAM now fits; runtime stack high-water testing under nested DMA/TIM
  activity remains mandatory.
- Called Newlib's identical IEEE `acos`/`asin` kernels directly for finite,
  range-checked inputs, so unused `errno`/reentrancy state is not linked.
  Replaced fixed calendar `floor` call sites and bounded 360-degree remainder
  calls with domain-specific equivalents, and kept the high-precision trig,
  VSOP/ELP calculations, MP3 Huffman/IMDCT tables, PCM, decoder state, error
  paths, logs, and protocol behavior unchanged.
- Added a targeted `noinline` boundary around `calc_panchang()`: global LTO had
  expanded `playSequence()` by embedding the complete calculation. The source
  call/control-flow boundary is restored while all other LTO remains enabled.
- Clean default MP3 profile: **LINK PASS**, `text=39,228`, `data=16`,
  `bss=19,308`; loadable Flash **39,244 bytes**, RAM **19,324 bytes**, leaving
  1,156 bytes. HEX SHA-256:
  `7B7F761A593CE8BA384A1810D22A2F9E7DD697591121723D927F3437F9E0F65B`.
- Complete full profile: **COMPILE PASS / CAPACITY LINK FAIL**,
  `text=89,624`, `data=16`, `bss=20,140`; loadable Flash **89,640 bytes** and
  RAM **20,156 bytes** including the 2,048-byte stack. RAM leaves 324 bytes;
  Flash exceeds the reserved 63,488-byte application region by **26,152 bytes**
  and physical 64 KB by **24,104 bytes**.
- Relative to the original measured complete image (117,704-byte Flash,
  28,432-byte RAM), total savings are **28,064 bytes Flash (23.84%)** and
  **8,276 bytes RAM (29.11%)**. Exact functionality-preserving optimization
  cannot close the remaining 26,152-byte application-region gap: the residual
  footprint is dominated by required Helix MP3 decode/tables and the required
  high-precision astronomy engine plus libm/libgcc soft-double support.

### Helix Format-Truth and Table Specialization Pass

- Rebuilt the complete profile with the shipping `-Os`, LTO,
  `-msmall-data-limit=0`, section GC, newlib-nano, and nosys configuration.
  Default LTO measured smaller than `-flto-partition=one`,
  `-flto-partition=none`, and alignment-sorted variants, so no inferior linker
  option was adopted.
- Confirmed from the active source path that playback is Layer III only, the
  12-bit sync mask accepts MPEG-1/2 but not MPEG-2.5, every application decode
  call uses normal file framing (`useSize=0`), and the wrapper rejects a
  free-bitrate first frame. Indexed CBR/VBR, mono, independent stereo rendered
  from its left channel, all MPEG-1/2 sample-rate indices, padding,
  CRC-header skipping, ID3v2, and ENC decryption remain present.
- Removed only unreachable Layer I/II bitrate rows, MPEG-2.5 rows, and
  free-bitrate scanning/state. Replaced the Layer III slot table with its exact
  integer definition, `floor((144 or 72) * bitrate / sample_rate)`. The retained
  MPEG-1/2 table values and decoder mathematics are unchanged.
- The fresh before/after full-profile measurements are 89,552 to **88,576
  bytes Flash** and 20,156 to **20,148 bytes RAM**, saving exactly **976 bytes
  Flash** and **8 bytes RAM** in this pass. The image still exceeds the
  63,488-byte application region by **25,088 bytes** and physical 64 KiB by
  **23,040 bytes**.
- Clean default MP3 profile: **LINK PASS**, `text=38,208`, `data=16`,
  `bss=19,300`; loadable Flash **38,224 bytes**, RAM **19,316 bytes**, leaving
  1,164 bytes. HEX SHA-256:
  `E0BEB78B6C74A89C45A1DAECFF05BB632248A2FD5B0C422FC8D2A06D700A99D5`.
- A non-LTO attribution build (used only to retain object ownership) measured
  Helix/wrapper 29,632 bytes, Panchang/astronomy 18,386, user application
  16,562, libm 11,932, libgcc soft-double 8,540, FatFs 3,112,
  core/peripheral/startup 3,200, and newlib-nano/nosys 862. Shipping LTO removes
  3,896 bytes across those boundaries.
- No Panchang coefficient, trig/math kernel, calculation, output field, MP3
  Huffman/IMDCT/polyphase table, error path, log, or timing path was removed.
  Doing so cannot be called behavior-neutral without a new compatibility or
  accuracy decision and a reference regression corpus.

### 60-KiB Full-Application Closure

- Replaced the general double-precision Panchang port with a compact
  application-specific single-precision engine for 2000-2050. It retains all
  values consumed by scheduling/playback: tithi, nakshatra, yoga, primary
  karana, signs, sunrise/sunset, moonrise/moonset, timing windows, calendar
  fields, and festival/special-day matching. Unconsumed secondary karana,
  yoga-end, Abhijit, second durmuhurta, and adhika-masa fields were removed.
- Added `tools/panchang_compact_compare.py`. Across 2,662 weekly dates, maximum
  Sun/Moon longitude differences were 0.011/0.296 degrees; tithi, nakshatra,
  and yoga mismatches were 27/15/15 at category boundaries. Across 120
  date/location cases, sunrise/sunset differences were at most 0.829/0.848
  minute. Independent moonrise/moonset and on-target timing checks remain open.
- Audited the deployed audio corpus: the root plain asset is mono MPEG-1 Layer
  III, 32 kHz, 64 kbps; the 25 encrypted assets are mono MPEG-1 Layer III,
  32 kHz, 128 kbps. Specialized Helix header, side-info, scalefactor-band, and
  bitrate paths for that exact 64/128-kbps profile. Padding, CRC-header skip,
  ID3v2, plain/ENC input, Huffman, IMDCT, polyphase, granule handoff, reservoir,
  errors, and DMA/PWM output remain. MPEG-2/2.5, stereo, other sample rates,
  free bitrate, and other bitrate indices are now rejected/absent.
- Removed accidental double promotions and the final libm dependency; the
  production symbol audit finds no double helpers, libm trigonometry,
  formatted I/O, allocator, or FreeRTOS symbol.
- Clean default full-profile result with WCH GCC `-Os`, LTO, function/data
  sections, section GC, newlib-nano/nosys, and no `-lm`: `text=60,132`,
  `data=16`, `bss=19,796`; loadable Flash **60,148 bytes**, RAM **19,812
  bytes** including the 2,048-byte stack. This leaves 1,292 bytes below 60 KiB,
  3,340 bytes in the 63,488-byte application region, and 668 bytes of RAM.
  HEX SHA-256: `347A5ABC3B45D3169D5C0E9EB18AAFB56A5F86A213ABBFF30D48B72EC9ED7087`.
- Versus the preceding 88,576/20,148-byte full image, this pass saved exactly
  **28,428 bytes Flash** and **336 bytes RAM**. Versus the original measured
  117,704/28,432-byte complete baseline, total savings are **57,556 bytes
  Flash (48.90%)** and **8,620 bytes RAM (30.32%)**.
- A non-LTO ownership build measured Helix/wrapper 27,844 bytes, user
  application 16,324, compact Panchang 6,590, libgcc soft-float/integer 5,560,
  MCU core/peripheral/startup 3,200, FatFs 3,112, and newlib-nano/nosys 862.
  Shipping LTO further reduces cross-module overhead.
- Full-profile hardware regression, production MP3 corpus PCM comparison,
  moonrise/moonset comparison, Panchang execution timing, and stack high-water
  measurement remain pending; no hardware PASS is inferred from the link.

### Golden-Reference Output Restoration Within 60 KiB

- Re-audited the compact Panchang implementation against the actual
  `esp_bell_idf/main/driver/astro/tamil_panchangam.c` VSOP/ELP implementation.
  The earlier Python surrogate used only eight lunar terms and was not a
  sufficiently authoritative golden reference.
- Located the first material divergence at lunar longitude: the former compact
  orbital model differed by up to 0.170 degrees and omitted the ESP engine's
  UT-to-TT Delta-T conversion. Its single-pass solar event approximation also
  differed from the reference two-pass NOAA sequence by up to about 0.85
  minute.
- Replaced the former lunar orbit with 30 leading ELP longitude and 20 leading
  latitude terms stored as compact millidegree coefficients. Identical
  quantized latitude arguments are consolidated. Restored the 2000-2050
  Delta-T polynomial and the largest ELP auxiliary longitude correction.
- Restored the reference two-pass NOAA sunrise/sunset flow using the existing
  compact trig kernels. Corrected intermediate UTC wrap at zero minutes and
  retained timezone/longitude handling. Added split whole/fractional-day phase
  accumulation so target binary32 evaluation does not lose lunar phase bits in
  a 240,000-degree intermediate product.
- Application initialization, cooperative cadence, schedule state flow,
  Panchang token order, file formats, festival/special matching, MP3 path,
  peripheral initialization, and logging were not changed in this pass.
- `tools/panchang_compact_compare.py` now parses the golden C coefficient
  tables directly. Across 2,662 weekly sunrise samples, tithi/nakshatra/yoga
  mismatches are **1/0/4**; maximum Sun/Moon longitude errors are
  **0.012/0.019 degrees**. Across 120 date/location cases, maximum
  sunrise/sunset errors are **0.024/0.024 minute**, moonrise/moonset errors are
  **0.085/0.089 minute**, and no event-presence mismatch occurred.
- Across 612 monthly transition cases, maximum tithi/nakshatra end-time errors
  are **1.968/1.946 minutes**. Existing five-minute spoken rounding differs in
  57/41 cases near rounding boundaries. These residual differences are
  documented rather than hidden or hardcoded.
- Clean production LTO link: `text=61,264`, `data=16`, `bss=19,796`; loadable
  Flash **61,280 bytes**, RAM **19,812 bytes**. The result remains 160 bytes
  below 60 KiB and leaves 2,208 bytes in the configured application region.
  Restoring golden-critical output cost 1,132 bytes over the 60,148-byte
  compact image. It still saves 56,424 bytes Flash versus the original
  117,704-byte complete baseline. HEX SHA-256:
  `616CADA93625EBE5E0359F13A0650BE25E1629C37079E42732B935B384576CA1`.

### Cooperative `Loading...` Stall and LTO Vector Fix

- Traced the reported permanent `Loading...` display through the reference
  boot/loop sequence. Initialization returned into the cooperative foreground,
  but its first `App_DelayMs(1)` could never finish because the final LTO ELF
  bound `TIM3_IRQHandler` to the startup file's weak infinite-loop handler.
  The millisecond counter therefore never advanced.
- Confirmed the same startup/LTO resolution defect had discarded the intended
  NMI and HardFault diagnostic handlers. The audio TIM2 and DMA handlers were
  already strong and were retained.
- Removed the local weak NMI, HardFault, and TIM3 definitions from all three
  checked-in CH32V20x startup variants. Their vector entries are now resolved
  by the project C handlers. Added explicit `used`/`externally_visible`
  attributes and retained a bounded default TIM3 handler for non-full build
  profiles.
- Kept the reference non-radio boot order and 600 ms idle/date refresh flow:
  RTC check/read, SD mount, persistent settings, player/runtime initialization,
  then cooperative player/light/bell services. No MP3 mathematics, Panchang
  calculation, output format, timing interval, log, or peripheral pin changed.
- Placed the MPEG-2-only `NRTab` table under the same existing compile guard as
  its sole MPEG-2 consumer, removing the managed build warning without changing
  the deployed MPEG-1-only decoder or linked footprint.
- Clean MounRiver managed build: `text=62,156`, `data=16`, `bss=19,796`;
  loadable Flash **62,172 bytes**, RAM **19,812 bytes**, leaving 1,316 bytes in
  the 63,488-byte application region and 668 bytes of RAM. Managed HEX SHA-256:
  `18E3EEF7103DB37765DB93A78D99702E70AB7F1826D9F87B8DC9C2CCDBEE0AB8`.
  Compact reproducible
  full link: `text=61,360`, `data=16`, `bss=19,796`; loadable Flash **61,376
  bytes**, 64 bytes below 60 KiB and 2,112 bytes below the application limit.
- Final symbol audit reports strong `T` bindings for NMI, HardFault, TIM3,
  TIM2, and DMA1 Channel2. Diagnostic and MP3-test profiles also link
  successfully at 6,968 and 36,680 loadable Flash bytes respectively.
- Re-ran the golden Panchang comparison with unchanged results: 1/0/4
  tithi/nakshatra/yoga classification mismatches over 2,662 samples, maximum
  sunrise/sunset error 0.024 minute, and maximum moonrise/moonset error
  0.085/0.089 minute. On-target confirmation that the LCD leaves `Loading...`
  remains pending; no hardware PASS is inferred from static/link validation.

### Maximum-Safe Digital Audio Level

- Audited `esp_bell_idf/main/driver/mp3_player/mp3_player.cpp`. Its local MP3
  path has no programmable volume or amplifier-gain setting: signed-16 PCM is
  shifted directly into the full 8-bit ESP DAC range. Amplifier control in the
  application is relay on/off with the existing 100 ms pre-switch and 300 ms
  post-switch delays; those timings and all CH32 call sites remain unchanged.
- Kept the CH32 caller volume at its existing maximum value 255. Replaced the
  former plain-file-only 87.5%-target gain with one bounded peak normalizer for
  both plain and encrypted playback. It starts at unity, rises by at most
  0.125x per 576-sample block (interpolated), falls immediately for loud
  blocks, ignores peaks below 512, and is limited to 254/256x through 8x with
  target peak 32,512.
- The 254/256 minimum prevents a full-scale decoded sample from reaching the
  PWM endpoints. Final saturation remains. Exhaustive host modeling of every
  block peak 1..32,768 and both signed-16 endpoints produced duties 1..254
  with zero out-of-range cases. The largest pre-shift product is 67,108,864,
  safely within signed 32-bit arithmetic.
- Relative to the former 28,672 target, the new target permits about 1.09 dB
  more digital level. Quiet files may receive up to 8x/about 18.06 dB gain;
  full-scale blocks receive a negligible 254/256 attenuation to retain PWM
  rail margin. No MP3 decoding, sample timing, relay timing, protocol, state
  flow, output format, or Panchang behavior changed.
- Clean MounRiver managed build: `text=62,128`, `data=16`, `bss=19,796`;
  loadable Flash **62,144 bytes**, RAM **19,812 bytes**, leaving 1,344 bytes in
  the 63,488-byte application region and 668 bytes of RAM. Managed HEX SHA-256:
  `ABA99DA36DA34E3C4AC9D1263C84CF7EF66486481FBC50B39D7D169CDA51992F`.
  Compact reproducible full link: `text=61,336`, `data=16`, `bss=19,796`;
  loadable Flash **61,352 bytes**, 88 bytes below 60 KiB and 2,136 bytes below
  the application limit.
- Neither repository contained a representative MP3/ENC/PCM test corpus, and
  target instruments are unavailable. Therefore maximum *digital* range is
  proven, but distortion-free and hardware-safe operation of the gain-2
  TLV9061 stage, unknown downstream power amplifier, and speaker is not yet a
  hardware PASS. Scope/listening/current/temperature validation is recorded as
  a release requirement rather than inferred.

### Short-Clip and Quiet-Asset Loudness Recovery

- Recompared the full output chain with the ESP-IDF reference. ESP installs
  the built-in 8-bit DAC through I2S, enables both DAC channels, and maps
  signed-16 PCM directly to unsigned 8-bit full scale. It contains no playback
  volume, codec gain, or programmable amplifier setting. Its `setRelay()` is
  the same on/off control with 100 ms before and 300 ms after the GPIO change.
- CH32 already uses caller volume 255 and essentially the same 8-bit modulation
  depth. Its hardware differs: one PA8 PWM output feeds a gain-2 TLV9061 active
  filter and an undocumented downstream speaker power stage. There is no CH32
  I2S codec or firmware amplifier register to copy from ESP.
- Identified avoidable software attenuation in the previous bounded envelope:
  each file reset to 1x, quiet gain was capped at 8x, and recovery advanced
  only 0.125x per 576-sample block. Short bell/voice assets could finish before
  reaching useful makeup gain.
- Raised the quiet-source ceiling to 32x/about +30.1 dB, starts every file at
  that ceiling, and increased smooth recovery to +1x per block. This adds up to
  12 dB over the former 8x ceiling. The complete block is peak-scanned before
  DMA, so loud material reduces immediately to the unchanged 32,512 target;
  no nonlinear waveshaper or intentional clipping was added.
- Exhaustive modeling over block peaks 1..32,768 and both signed endpoints
  again produced PWM duties 1..254 with zero out-of-range cases. The absolute
  theoretical 32x product is 268,435,456, within signed 32-bit range; the
  peak-controlled endpoint model reached only 8,323,072.
- Clean compact full link: `text=61,328`, `data=16`, `bss=19,796`; loadable
  Flash **61,344 bytes**, RAM **19,812 bytes**, leaving 96 bytes below 60 KiB
  and 2,144 bytes in the application region. MounRiver managed link:
  `text=62,120`, `data=16`, `bss=19,796`; loadable Flash **62,136 bytes** with
  1,352 bytes of application-region margin. Managed HEX SHA-256:
  `A2D120920563F561C9A7739B2AEFB1E6D22808EC2D9A46DA72459A1227C03C08`.
- Songs, assets, and target instruments were unavailable locally, so decoder
  correctness is preserved by leaving Helix and sample timing untouched, but
  actual loudness, noise-floor amplification, pumping, AUX_OUT clipping, and
  amplifier/speaker thermal behavior still require the recorded target test.

### Post-32x Hardware Result and First Electrical Divergence

- User hardware result: the high-makeup image improves sound volume, but the
  physical speaker remains noticeably quieter than the ESP-BELL IDF device.
  Songs continue to play correctly.
- Re-audited the digital paths sample by sample. ESP converts signed-16 PCM to
  unsigned 8-bit DAC code with `(pcm >> 8) + 128`, clamped to 0..255. CH32 uses
  volume 255, block look-ahead, final saturation, and duties 1..254. Their
  maximum peak-to-peak spans are 255 and 253 counts respectively: a ratio of
  253/255, approximately -0.07 dB. This is inaudible and cannot explain the
  reported loudness gap.
- Sample rate is 32 kHz in both deployed paths, both consume the same
  bit-identical Helix PCM, and neither implementation contains a codec gain or
  programmable amplifier-volume register. Relay polarity/timing and playback
  sequencing also match. The CH32 32x envelope can already raise quiet blocks
  beyond the ESP software mapping without exceeding its peak envelope.
- The first unverified divergence is electrical. ESP enables both internal DAC
  channels on GPIO25/26. CH32 has one 375 kHz PA8 PWM channel followed by an
  AC-coupled, single-supply TLV9061 filter and an undocumented external power
  amplifier/speaker stage. Whether ESP uses one DAC, sums two channels, or
  drives a differential input is unknown; so are the downstream amplifier
  part, gain network, supply, and load.
- No further digital boost was applied. Full-scale material has no clean
  software headroom; increasing it would require clipping, nonlinear dynamic
  compression, or reduced audio quality and would not correct missing analog
  bias/gain or channel summing.
- Required next measurement with one identical asset/load: RMS and peak-to-peak
  voltage at ESP GPIO25/26, CH32 reconstructed PA8, TLV9061 pin 1, `AUX_OUT`,
  and the power-amplifier input/output, plus idle DC at TLV9061 pins 1/3/4.
  Those readings locate the first voltage drop and determine whether the fix is
  filter bias/component correction, dual-channel summing, or amplifier gain.

### Full Audio Schematic Calculation

- Inspected the supplied full audio schematic. `AUX` is AC-coupled by 1 uF,
  followed by two 1.2 kOhm series resistors into TLV9061 IN+. One 4.7 uF
  capacitor shunts IN+ to ground; two parallel 4.7 uF capacitors connect the
  resistor junction to the op-amp output. IN- has 1.2 kOhm to ground and 1.2
  kOhm to output (`K=2`). `AUX_OUT` is AC-coupled by 1 uF.
- Exact Sallen-Key calculation with the drawn parts gives `C1=4.7 uF`,
  `C2=9.4 uF`, `fc=19.9538 Hz`. With `K=2`, its ideal damping denominator is
  zero (`Q` tends to infinity); increasing `Rf` would create negative damping.
  This is not a safe gain-adjustment point.
- Interpreting the three filter parts as 4.7 nF moves the corner to 19.9538
  kHz, but does not fix the zero damping at gain 2. With the existing 2:1
  capacitor ratio and unity follower gain, the result is exactly `Q=0.7071`
  and about 50.96 dB ideal attenuation at the 375 kHz PWM carrier.
- The input coupling capacitor leaves IN+ without any DC path, while the
  feedback divider references IN- to ground on a 3.3 V single supply. The
  recommended baseline rework adds a decoupled 1.65 V reference and IN+ bias,
  uses 4.7 nF filter capacitors, and configures the op amp as a unity follower.
  This corrects passband loss/bias/stability rather than masking them with gain.
- A conditional higher-gain redesign was calculated: equal 6.8 nF filter
  capacitors, `Rg=1.2 kOhm` to VREF, and `Rf=698 ohm` give `K=1.5817`,
  `fc=19.504 kHz`, and `Q=0.7051`. It must not be populated until measured
  input peaks prove adequate 3.3 V output-swing margin.
- The TLV9061 and 1 uF output coupling network are line-level circuitry. If
  `AUX_OUT` drives a low-impedance speaker directly, low volume is expected and
  resistor gain changes are not a valid remedy; a speaker power amplifier is
  required.

### Direct Speaker Connection Confirmed

- The owner confirmed that `AUX_OUT` is wired directly to the passive speaker.
  For an 8-ohm speaker, the existing 1 uF series capacitor alone has a
  19.89 kHz high-pass corner, which rejects almost the entire audio band.
- The TLV9061 is not a speaker power amplifier. The selected baseline repair
  is to correct the TLV9061 reconstruction filter/bias, retain it as a
  line-level stage, and add a PAM8302A analog-input mono Class-D power stage.
  The speaker must connect across the amplifier's two bridge outputs, not from
  either output to ground.
- The owner then constrained the repair to populated-component replacements
  only. A drop-in high-current SOT-23-5 op amp plus a much larger output
  coupling capacitor may improve level, but the available official AD8531
  audio example validates 32-to-600-ohm headphones and uses 270 uF plus a
  16-ohm protection resistor. It does not establish an 8-ohm direct-speaker
  design, so that substitution cannot be released without load-specific
  electrical and thermal qualification.

### LM358-Only External Amplifier Design

- The owner rejected LM386 and all other amplifier ICs for the external test.
  Added a deterministic connection drawing for one LM358B plus discrete
  BD139-16/BD140-16 emitter followers. U1A provides gain 3.2; U1B closes global
  feedback around the speaker-current stage. A 9 V current-limited supply,
  4.5 V reference, 1000 uF output coupling, emitter ballast, and output Zobel
  network are specified.
- Breadboard electrical, thermal, dummy-load, clipping, and speaker results are
  pending; this design has not been validated on the target hardware.

### LM358 and Passive Components Only

- The owner subsequently prohibited the discrete output transistors too.
  Added `LM358_AUX_OUT_gain_only.svg`, a non-inverting LM358 stage with fixed
  gain 3.2, 4.5 V bias, and AC-coupled input/output. Its output is explicitly
  limited to a load of 10 kOhm or greater; it cannot drive the passive speaker.

### Clean Full-Profile Firmware Rebuild After Hardware Work

- Resumed the firmware path with `HAPPYBELL_PROFILE_FULL` as the production
  default and performed a clean WCH GCC 8 LTO build from every non-FreeRTOS C
  source plus the CH32V203 D6 startup file.
- The fresh ELF resolves `NMI_Handler`, `HardFault_Handler`, and
  `TIM3_IRQHandler` as strong project symbols. The TIM3 vector contains the
  project handler address, so the cooperative 1 ms clock cannot fall into the
  startup weak infinite loop that would leave the LCD at `Loading...`.
- Clean size result: `text=61,336`, `data=16`, `bss=19,796`; loadable Flash is
  **61,352/63,488 bytes** and RAM is **19,812/20,480 bytes**. This leaves 2,136
  bytes of application Flash and 668 bytes of RAM.
- Generated a new ELF, Intel HEX, and raw binary in `Firmware/obj_continue/`.
  No application logic, timing, MP3 decoding, Panchang calculation, output, or
  logging behavior was changed during this rebuild. Hardware flashing and the
  integrated flow/stack tests remain pending.

### WAV File PWM Service Image

- Added `HAPPYBELL_PROFILE_WAV_TEST` without changing the production default.
  The service image scans the SD root for the first `.wav` short-name entry,
  auto-plays it, and assigns UP to replay. This works with the card-generated
  8.3 alias for `tone-2000hz-sine.wav` while keeping FatFs LFN disabled.
- Added a bounded RIFF/WAVE reader for uncompressed PCM mono/stereo, 8/16-bit,
  8-48 kHz. Stereo samples are averaged to mono, converted into the existing
  two 576-sample slots, peak-bounded, and sent through the same PA8 TIM1 PWM,
  TIM2 sample clock, and DMA1 Channel 2 path used by MP3.
- Corrected an LTO interrupt-vector defect found by the small test link:
  removed the D6 startup file's local weak TIM2 and DMA1 Channel 2 handlers so
  those vector references retain and bind the real C ISRs. Both now appear as
  strong `T` symbols. This correction also restores the intended audio handler
  binding in the full image.
- Clean WAV service link: `text=9,860`, `data=16`, `bss=5,768`; loadable Flash
  **9,876 bytes**, RAM **5,784 bytes**. HEX SHA-256:
  `EA48F6D2495366FAC5D42F9D5DE57D91465A6C47C24C46356FE73CF233779BDD`.
- Corrected full-profile link: `text=61,536`, `data=16`, `bss=19,796`;
  loadable Flash **61,552/63,488 bytes**, RAM **19,812/20,480 bytes**, leaving
  1,936 bytes of application Flash and 668 bytes of RAM. No target result is
  claimed until the WAV and integrated full images are flashed and measured.

### WAV Test Asset Folder Correction

- The supplied SD-card view confirmed `tone-2000hz-sine.wav` is stored inside
  folder `001` alongside the encrypted announcement files, not in the root.
- Changed only the WAV service profile's discovery folder from the root to
  `001`. Its `.wav` filter ignores the neighboring `.enc` and JSON entries;
  FatFs continues to open the tone through its generated short-name alias.
- Refreshed link: `text=9,900`, `data=16`, `bss=5,768`; loadable Flash
  **9,916 bytes**, RAM **5,784 bytes**. The final TIM2 and DMA1 Channel 2
  handlers remain strong `T` symbols. HEX SHA-256:
  `9A56ED4BE835505BC43F54504F8613130DBFC74FD12130A40A8DEBA715EECB0D`.
- Target playback and the 2 kHz reconstructed-output measurement remain
  pending.

### WAV Test Made the Temporary Project Default

- The board continued running the normal application because MounRiver's
  Build/Download path selected its standard `obj/CH32V203G8R.hex`, while the
  WAV test had been built as a separate artifact and the source default still
  selected the full profile.
- Temporarily changed the source default to `HAPPYBELL_PROFILE_WAV_TEST` so a
  normal clean MounRiver build produces the WAV service image. The production
  full-profile artifact remains preserved and must be restored as the source
  default after the 2 kHz measurement.
- Clean no-override standard-name build: `text=9,900`, `data=16`, `bss=5,768`.
  `obj_wav_default/CH32V203G8R.hex` contains `WAV PWM TEST`, `PLAYING WAV`,
  folder `001`, and `UP: REPLAY WAV`, and does not contain the full-profile
  `Loading...` startup string. HEX SHA-256:
  `5D573F14EA06ADC8FCB2B3762C5CC0FA190A85B5CC67B914280FA9C3CA70332A`.
- Copied that verified HEX to MounRiver's configured download target
  `Firmware/obj/CH32V203G8R.hex`, replacing the stale full-profile HEX there.

### Integrated ENC, WAV, and Plain-MP3 Audio Service

- The owner confirmed the standalone `001/tone-2000hz-sine.wav` service image
  plays correctly on the target hardware.
- Changed the temporary default to the integrated MP3-test profile. It retains
  the old startup clock/fixed-point guards, bounded `001/*.enc` discovery,
  automatic encrypted playback, XOR decryption, Helix decode, gain, relay, and
  reporting flow. It additionally discovers the first `001/*.wav` entry.
- The selection screen maps UP to the chosen ENC, DOWN to WAV, and MENU to the
  former root `mp3_song.mp3` regression path. Each blocking playback completes
  `CH32_MP3_Stop()` before returning; the next mode reinitializes TIM1, TIM2,
  and DMA1 Channel 2. Both formats reuse the same input and PCM/DMA buffers.
- WAV retains the confirmed RIFF PCM path: mono/stereo, 8/16-bit, 8-48 kHz,
  stereo-to-mono averaging, bounded gain, and the production PWM/DMA output.
- Clean no-override LTO link: `text=38,188`, `data=16`, `bss=19,128`;
  loadable Flash **38,204/63,488 bytes**, RAM **19,144/20,480 bytes**, leaving
  25,284 bytes of application Flash and 1,336 bytes of RAM. TIM2 and DMA1
  Channel 2 remain strong handlers. HEX SHA-256:
  `FCC8002BEAE8208A63F9ED21A52A1954400C7456241986F6418F08124C2ED652`.
- Copied the verified combined HEX to MounRiver's configured target
  `Firmware/obj/CH32V203G8R.hex`. Integrated switching remains to be tested on
  the board; no hardware PASS is claimed yet.
- Strict `-Wall -Wextra -Werror` entry-point compilation passes for diagnostic,
  combined MP3-test, full, and standalone WAV profiles. The standard MounRiver
  target and combined artifact have identical SHA-256 hashes, and the final ELF
  contains Helix decode plus strong HardFault, NMI, TIM2, and DMA1 Channel 2
  handlers.

### Exact Pre-WAV Firmware Restoration

- Restored the six firmware source files changed for the 2 kHz WAV/PWM test to
  their exact pre-test contents. This removes the RIFF/WAV player, standalone
  WAV profile, integrated ENC/WAV selection flow, root-folder discovery change,
  and the test-period startup-vector edits. No unrelated application, timing,
  MP3/ENC, Panchang, peripheral, logging, or cooperative-flow code changed.
- Restored `HAPPYBELL_PROFILE_FULL` as the source default. The retained MP3
  service profile has its prior behavior: automatic first `001/*.enc` playback,
  UP for ENC replay, and DOWN for root `mp3_song.mp3` playback.
- Performed a clean WCH GCC 8 LTO rebuild from the restored source. Result:
  `text=61,336`, `data=16`, `bss=19,796`; loadable Flash **61,352/63,488
  bytes**, RAM **19,812/20,480 bytes**, leaving 2,136 bytes of application
  Flash and 668 bytes of RAM.
- The rebuilt HEX SHA-256 is
  `FC3E435FAEBF18C279ACC0EF8A8C70B09BD14D9AF0760D7AC34EBA68AE3FB7CF`,
  exactly matching the preserved pre-WAV full-profile HEX. Copied this verified
  image back to MounRiver's configured `Firmware/obj/CH32V203G8R.hex` target.

### Git-Ready Cleanup and R1 LM358 Hardware

- Audited the managed G8R6 project, compiler/linker configuration, include
  graph, source calls, startup selection, linker script, Helix/FatFs licenses,
  host validation tools, and runtime SD-file dependencies before deletion.
- Retained the complete current firmware source, WCH Core/Peripheral support,
  FatFs, Helix with licenses, D6 startup, G8R6 linker/project/debug metadata,
  host regression and ENC-generation tools, and all five project documents.
  Vendor peripheral files remain because they are configured managed-build
  inputs and pruning them would alter the SDK configuration for no firmware
  Flash benefit after LTO/section garbage collection.
- Removed the unused FreeRTOS source/configuration, non-G8R6 D8/D8W startup
  files, obsolete C8T project/launch files, six tracked optimization-output
  trees, all untracked WAV/combined/restore build trees, MounRiver local state,
  PDF-render scratch files, and Python bytecode cache. All removed tracked
  content remains recoverable from Git history; no SD runtime asset existed in
  the repository.
- Added `tools/build_g8r6.ps1`. It performs the compact WCH GCC 8 LTO build,
  writes only to ignored `Firmware/obj`, prints size, and rejects any HEX whose
  SHA-256 differs from the working reference. Preserved the verified release
  HEX and checksum under `Firmware/release`.
- Ran the new script from the cleaned repository with PowerShell 7: **PASS**.
  It reproduced `text=61,336`, `data=16`, `bss=19,796`, loadable Flash 61,352
  bytes, RAM 19,812 bytes, and the exact reference HEX SHA-256. The script
  explicitly rejects Windows PowerShell 5 because its native argument passing
  produced a different LTO layout.
- Inspected all three pages of `HappyBell-CH32V203G8R6_R1.pdf` and retained it
  under `hardware/`. R1 replaces the historical TLV9061 circuit with a 5 V
  LM358 channel-A inverting stage: 4.7 kOhm/10 nF and 10 kOhm/10 nF passive
  sections, 1 uF coupling, 1 kOhm input, 10 kOhm feedback (gain -10), 2.5 V
  bias from 10 kOhm/10 kOhm, 100 nF supply decoupling, and 10 uF output
  coupling to `AUX_OUT`.
- No firmware pin, timer, GPIO, sample format, gain algorithm, timing, control
  flow, or output was changed for LM358. Hardware-only risks are retained as
  pending: LM358 is not a speaker power amplifier, positive swing is limited
  on 5 V, response is load-dependent, and unused channel-B pins 5/6/7 are
  floating in the R1 drawing.
