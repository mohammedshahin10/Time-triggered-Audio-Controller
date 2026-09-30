# AGENT.md

## Purpose

These rules apply to developers and AI coding agents modifying the HAPPY BELL
repository.

## General Development Rules

### DO

- Keep code readable and understandable.
- Follow the existing repository structure.
- Preserve existing working functionality.
- Make the smallest change needed for the task.
- Use meaningful names and consistent formatting.
- Keep comments short and only where necessary; group related statements
  together instead of separating every statement with blank lines and a
  per-statement comment block (owner-set style rule, 2026-08-24).
- Use macros or helper functions when they improve configuration and clarity.
- Keep comments short and focused on intent or non-obvious constraints.
- Keep hardware-specific definitions centralized.
- Keep configuration values easy to locate.
- Keep peripheral initialization organized.
- Separate hardware access, library wrappers, and application logic where
  practical.
- Validate changes against the CH32V203G8R6 schematic and current PCB.
- Confirm the target MCU/package and pin assignment before changing drivers.
- Verify the clock tree before calculating serial, timer, PWM, or sample rates.
- Consider interrupt, timing, RAM, stack, and flash effects before changing
  embedded code.
- Preserve visible error reporting and use bounded communication timeouts.
- Review the five documentation files under `document/` before starting a
  change.
- Synchronize the five documentation files whenever implementation or task
  status changes.
- Add completed work chronologically to `document/PROGRESS.md`.
- Remove completed items from `document/PENDING_TASKS.md`.
- Add new libraries, constraints, and test requirements to
  `document/REQUIREMENTS.md`.

### DON'T

- Do not invent pin mappings, test results, clock values, library versions, or
  electrical behavior.
- Do not change unrelated code.
- Do not silently change clock, timer, pin, or hardware behavior.
- Do not scatter pin definitions or configuration constants through the code.
- Do not hard-code configuration values without a clear hardware reason.
- Do not create unnecessary functions, deep call chains, or excessive nesting.
- Do not place excessive logic in one function.
- Do not use nested functions where the language/toolchain does not require or
  support them.
- Do not introduce unused code, macros, variables, or libraries.
- Do not add comments that merely repeat the code.
- Do not duplicate existing driver or library functionality.
- Do not remove working behavior without documenting the reason.
- Do not ignore compiler warnings without written justification.
- Do not mark a hardware test PASS without an actual observation or
  measurement.
- Do not create additional project Markdown files. Maintain exactly these five
  files under `document/`: `README.md`, `REQUIREMENTS.md`, `AGENT.md`,
  `PENDING_TASKS.md`, and `PROGRESS.md`.

## Embedded / Firmware-Specific Rules

- Keep interrupt handlers short and bounded.
- Do not perform file I/O, logging loops, blocking waits, or MP3 decoding inside
  an interrupt.
- Avoid unnecessary dynamic allocation; confirm any allocation against the
  20 KB SRAM limit.
- Check both static RAM and worst-case stack use in the linker map.
- Avoid long blocking delays where an event or timer can be used safely.
- Protect data shared between interrupt and foreground contexts when required.
- Configure GPIO mode and alternate function explicitly.
- Confirm the real peripheral timer clock before calculating reload values.
- Re-measure timing after changes to clocks or peripheral initialization.
- Preserve startup-file interrupt naming and WCH interrupt attributes.

## Build Profile Rules

- Treat `C:\OfficeWorks\esp_bell_idf` as the source of truth for application
  flow, file formats, sequencing, menu behavior, scheduling, and local audio.
  Preserve like-for-like non-radio behavior unless a CH32 hardware/API
  adaptation or a measured capacity limit requires a documented difference.
- Do not add connection credentials, radio drivers, streaming tokens, online
  time synchronization, radio callbacks/states/tasks, or their build
  dependencies to the CH32 project. Unknown/non-local playlist tokens must be
  consumed safely and ignored rather than mapped to a fabricated local file.
- `Firmware/User/happybell_build.h` is the single build-profile switch. Keep
  `HAPPYBELL_PROFILE_FULL` as the production default. The temporary 2 kHz
  WAV/PWM and combined ENC/WAV paths were removed by owner instruction; do not
  reintroduce them without a new explicit requirement.
- Preserve the current 2,048-byte main/ISR stack. It was reduced only after
  removing a 256-byte live copy from the deepest full-profile schedule call
  chain; runtime stack-guard/high-water testing is still required.
- The diagnostic profile must remain bare-metal and limited to the LCD,
  keypad, RTC-presence check, SD/FatFs, and `TEST.TXT` viewer path.
- Keep the MP3-test profile bare-metal. It performs a bounded FatFs scan of
  folder `001`, selects the alphabetically first `.enc` short filename, and
  auto-plays the ENC. UP replays ENC and DOWN plays root `mp3_song.mp3` for
  regression comparison. Do not add a blocking
  text pager or unbounded recursive scan. Playback reports path, status,
  PCM/DMA/underflow counters, sample rate, bit depth, channels, bitrate, and
  frame/sample sizes.
- The current owner-authorized physical-speaker test initializes PA3/RLY_IN and
  asserts it only around file playback. Keep it LOW
  otherwise. The schematic exposes only the low-side RLY_OUT driver; document
  the actual external relay/amplifier behavior after hardware observation.
- RAW PA8 and DMA sine-wave test code was removed by owner instruction in V10.
  Do not reintroduce test generators, buffers, APIs, screens, or startup delays
  into the production playback path without a new explicit requirement.
- Preserve `SYSCLK_FREQ_96MHz_HSI` for the G8R6 PCB. The reviewed schematic
  shows no external HSE crystal; selecting the HSE PLL path can leave the MCU
  at the 8 MHz reset clock. Preserve the V8 `SystemCoreClock == 96000000U`
  guard before any audio test.
- With HCLK 96 MHz and APB1 divided by two, use WCH's automatic x2 timer clock
  when calculating TIM2: PCLK1 is 48 MHz and the TIM2 counter clock is 96 MHz.
- Preserve the V8 image's signed fixed-point self-test until Helix PCM is verified on
  target. `ARITH FAIL` must stop before SD/MP3 playback; do not hide it.
- The historical V6/V7 buzzing sine and slow/distorted playback occurred while
  the absent HSE source was selected. Treat those audio-quality observations as
  clock-contaminated; retest at confirmed 96 MHz before attributing remaining
  distortion to the R1 LM358/filter hardware.
- V8 plain playback is confirmed normal-speed and free of audible distortion,
  and V9 ENC playback is confirmed working through the physical speaker.
  Preserve the HSI clock selection, granule handoff, volume 255, shared bounded
  all-file gain, and PA8 DMA/PWM path. Completed-song counters, zero
  underflows, carrier measurement, and maximum-volume plain/ENC quality remain
  pending until explicitly observed.
- Always call `TIM_OCStructInit()` before configuring TIM1 output compare.
  WCH's advanced-timer initializer reads complementary-output and idle-state
  members even when only CH1 is used; uninitialized fields can suppress PA8.
- Reset TIM1, TIM2, and DMA1 Channel2 before reinitializing the audio path.
- Keep blocking UART `printf()` out of the bare-metal audio startup path. The
  2026-08-25 image stopped between its build marker and tone checkpoint at the
  first print; use the LCD checkpoints for this constrained test.
- Preserve the bounded 500 ms DMA wait timeout. A missing TIM2/DMA completion
  must return `DMA TIMEOUT` instead of leaving the LCD on `PLAYING:` forever.
- Preserve `HAPPYBELL_PROFILE_FULL` as the default complete cooperative
  Astro/MP3 integration. Its 2026-08-27 clean production link fits both the
  configured Flash region and RAM; remeasure after every capacity change.
- The restored pre-WAV high-makeup production measurement requires 61,352
  bytes of loadable Flash and 19,812 bytes of RAM including the fixed main/ISR
  stack. It leaves 2,136 bytes in the application Flash region and 668 bytes
  of RAM. Re-run and
  replace these figures after any capacity-related change; do not treat source
  compilation as a successful G8R6 link.
- Under LTO, audit `NMI_Handler`, `HardFault_Handler`, `TIM2_IRQHandler`,
  `TIM3_IRQHandler`, and `DMA1_Channel2_IRQHandler` in the final ELF. The exact
  restored pre-WAV image intentionally retains its historical weak startup
  bindings for TIM2 and DMA1 Channel 2. Do not silently change those bindings
  while maintaining the requested exact pre-WAV state.
- Keep FreeRTOS absent from managed builds and keep all application/library
  sources independent of its headers and APIs.
- Preserve TIM3 as the full profile's 1 ms clock, below audio interrupt
  priorities. Keep the bell service at 20 ms, schedule evaluation at 600 ms,
  light/RTC service at one second, playback key polling at 10/100 ms as
  configured, and DMA wait service at 1 ms.
- Preserve the 8-byte `.noinit` HardFault record and MP3 stage updates. Do not
  clear that section in startup; `FaultDiag_TakeHardFaultStage()` owns
  consumption after LCD initialization.
- Preserve the fine-grained Helix checkpoints in `mp3dec.c`: header,
  side-info, main-data, scale-factor, Huffman, dequantization, IMDCT, and
  subband. If a future reset occurs, record the exact LCD label before changing
  the implicated decoder phase.
- Preserve the compact mono synthesis-history layout when
  `HELIX_CH32_MAX_CHANNELS=1`: `VBUF_BLOCK_STRIDE=32` and
  `VBUF_HALF_LENGTH=544`. The upstream 64/1,088 addressing assumes the
  two-channel 2,176-integer allocation and overruns this port's 1,088-integer
  buffer. Re-run the bounds audit and both channel-count syntax checks after
  changing `coder.h`, `dct32.c`, `polyphase.c`, or `subband.c`.
- Do not use literal 64-word history strides in the mono FDCT/polyphase path.
  Use `VBUF_BLOCK_STRIDE`/`VBUF_HALF_LENGTH` for every mono start and step; a
  single retained `vbuf + 64` corrupted most PCM samples while DMA continued
  normally. Literal 64 remains valid only inside the explicit stereo routine.
- Keep the `.noinit` fault record before normal `.bss` in the linker script;
  do not move it back next to the downward-growing stack.
- Keep read-only FatFs in `FF_FS_TINY=1` for this constrained target. It
  removes the private 512-byte `FIL` cache from the MP3 call stack. Serialize
  all FatFs access because file objects share the mounted volume's sector
  window.
- Rebuild and inspect the map after changing any profile or its selection.
- Preserve the compact Panchang engine's split phase accumulation, two-pass
  NOAA solar events, 2000-2050 Delta-T correction, and leading quantized ELP
  terms. They restore golden-reference boundary/rise-set behavior while
  keeping the image below 60 KiB; do not replace them with direct large-day
  binary32 multiplication or the former simplified lunar orbit.

## Peripheral Rules

### PA8 / TIM1 Ownership

- PA8/TIM1_CH1 is the final PCB `AUX` signal. The MP3-test and full profiles
  assign it exclusively to MP3 audio (`ch32_mp3_player.c`): 8-bit PWM carrier,
  ARR 255, nominally 375 kHz at a 96 MHz timer clock, into the R1 passive RC
  network and 5 V LM358 stage. The SD/LCD-only diagnostic leaves PA8
  uninitialized.
- Treat the R1 schematic in `hardware/HappyBell-CH32V203G8R6_R1.pdf` as the
  hardware authority. The path is 4.7 kOhm/10 nF, 10 kOhm/10 nF, 1 uF, 1 kOhm
  into LM358 pin 2; 10 kOhm feedback from pin 1 gives signal gain -10. Pin 3 is
  biased by 10 kOhm/10 kOhm at nominally 2.5 V; pin 8 is 5 V, pin 4 is ground,
  and pin 1 reaches `AUX_OUT` through 10 uF.
- Never treat LM358 as a low-impedance speaker power amplifier. Measure PA8,
  LM358 pin 1 before the 10 uF capacitor, and `AUX_OUT` under the intended load
  when diagnosing level or distortion.
- R1 leaves unused channel-B pins 5/6/7 open. Document this as a hardware risk;
  do not compensate in firmware. A future board may bias pin 5 at the 2.5 V
  reference and connect pin 7 to pin 6 after schematic/layout review.
- The legacy 100 kHz `pwm.c` test was removed on owner instruction
  (2026-08-24), and its parked `_to_delete/` copy was removed during Git-ready
  cleanup. The PA8 mode conflict no longer exists.
- Preserve its reported passing hardware result as historical evidence, but do
  not treat it as validation of the current MP3/DMA audio PWM path.
- Measure frequency separately from duty cycle after every relevant change.

### MP3 / Audio

- Keep application code behind `ch32_mp3_player.h` where practical.
- Preserve the deployed-audio profile unless a new size budget and corpus
  validation expands it: MPEG-1 Layer III, mono, 32 kHz, indexed 64/128 kbps,
  and frames no larger than the 768-byte input buffer.
- Keep stream and output channel checks explicit. The deployed profile is
  mono-only; reject stereo before decoding into the one-channel allocation.
- Preserve the side-info-to-decoder `part23Length` transfer; Huffman decoding
  depends on it and a zero or stale value can produce silent or invalid output.
- Reject unsupported input before decoding into fixed-size arrays.
- Do not terminate playback on a single unsupported-looking sync candidate;
  compressed payload can contain false sync bits. Advance one byte and keep
  scanning until a supported deployed-profile frame is found or input ends.
- In the focused `mp3_song.mp3` test, a full 768-byte buffer combined with
  decoder input-underflow is a false oversized header candidate because the
  current mono asset's verified frames are 288 bytes. Advance one byte and
  rescan.
- Keep the decoder single-instance unless memory ownership is redesigned.
- Maintain the two contiguous 576-sample PCM slots. Preserve
  `MP3DecodeWithGranuleHook()` and
  `Audio_GranuleOutputHook()`: queue granule 0 before decoding granule 1, start
  the next frame when row 0 is free, and wait for row 1 only before overwriting
  it. Waiting for both rows before every frame creates a mandatory audible gap.
- Keep the unhooked `MP3Decode()` API reference-compatible. After changing the
  Helix core or granule hook, use `tools/helix_host_decode.c` to decode the
  validated mono asset and compare every signed-16 sample against the untouched
  reference decoder; output length or a PCM hash alone is not sufficient.
- Keep `CLZ()` shifts unsigned and compile stereo-only dequant/subband paths
  only when `MAX_NCHAN > 1`. Signed left-shift overflow and mono-build channel-1
  indexing are invalid C even when a particular target run appears to work.
- TIM2's sample-rate update event drives PWM duty via DMA1 Channel2 directly
  into `TIM1->CH1CVR`; no ISR may write per-sample duty data. Exact-divider
  rates such as 32 kHz keep `TIM2_IRQHandler` disabled. For 44.1/22.05/11.025
  kHz only, preserve V9's bounded ARR phase-accumulator ISR so the long-term
  sample clock is exact; it must not access PCM, FatFs, Helix, or TIM1 duty.
  Keep the DMA Transfer-Complete ISR (`DMA1_Channel2_IRQHandler`) limited to
  slot handoff (reprogramming the DMA source/count), not per-sample work.
  Do the volume/PWM-bias conversion in bulk, once per decoded frame, before
  the frame is handed to DMA -- never per-sample in the ISR.
- Test `CH32_MP3_GetUnderflowCount()` with every representative audio asset.
- Preserve final saturation so PCM can never wrap around the 0-255 TIM1 compare
  range. Both plain and encrypted playback use the bounded block-peak envelope:
  32x initial gain, 254/256x-32x limits, 32,512 target, block-interpolated +1x
  maximum rise, immediate reduction, and no tracking below peak 512. Starting
  at 32x is safe only because the complete block is scanned before DMA.
  At volume 255 the verified numerical duty range is 1..254. Do not raise these
  limits without a new exhaustive arithmetic analysis plus scope, audible
  distortion/level, amplifier-current, and component-temperature tests. The
  old fixed 4x trial clipped a full-scale song badly.
- The owner-reported 32x-envelope result is improved but still quieter than
  ESP-BELL. Do not add another digital multiplier or waveshaper: at loud peaks
  CH32 duty 1..254 is already within about 0.07 dB of ESP DAC codes 0..255.
  First measure ESP GPIO25/26, CH32 PA8, LM358 pin 1, `AUX_OUT`, and the
  downstream power-amplifier input/RMS output using the same asset and load.
  Confirm whether the ESP hardware sums or differentially uses both DAC
  channels and identify the power-amplifier part/gain network.
- Keep the R1 LM358 hardware test at the owner-selected volume 255 unless a
  hardware observation changes it. Preserve saturation, and do not interpret
  audibility as proof that the 5 V, gain-10 stage is unclipped. The reference
  amplifier control is relay on/off only; there is no firmware gain register.
- Keep `tools/encode_mp3.ps1` synchronized with the firmware's repeating
  `BK26` XOR key. If either changes, run an encode-twice SHA-256 round-trip
  test before generating SD assets.
- Preserve the ENC format exactly: XOR the complete MP3 file from byte zero
  with repeating ASCII `BK26`; there is no IV, salt, wrapper, or extra header.
  Decrypt each FatFs read using its absolute file offset, not a mutable key
  index that can lose phase after a seek or short read.
- Preserve V9 preflight checks for ID3 synchsafe length, MPEG Layer III header,
  decoded stream parameters, mid-stream format changes, clock configuration,
  SD read failure, corruption, and DMA timeout. Do not report a zero-frame ENC
  file as successful playback.
- Keep `.enc` extension detection ASCII case-insensitive because FatFs may
  return an 8.3 filename with an uppercase `.ENC` extension.
- Do not claim real-time playback until SD, decode, PWM, and `AUX_OUT` tests pass.

### SD Card

- PA4 = CS, PA5 = SCK, PA6 = MISO/SD_DO, and PA7 = MOSI/SD_DI.
- Keep SPI initialization speed slow as required by the SD protocol.
- Check card power, signal integrity, wiring, CS behavior, and card format before
  assuming a software protocol fault.
- Preserve the existing block-driver/FatFs separation.
- Keep `sd_text_viewer.c/.h` as the application-facing text-file pager. The
  SD/LCD-only profile uses it for `TEST.TXT`; the focused V10 MP3-test loop does
  not enter the pager, and the reference-style full profile does not call it.
  Do not call its blocking loop from the full foreground without converting
  its waits and preserving cooperative service/file-system ownership.

### LCD

- PB10 = D4, PB11 = D5, PB12 = D6, PB8 = D7.
- PB13 = RS and PB14 = EN.
- Do not change this reported-working G8R6 mapping without a hardware reason.
- Preserve `LCD_CreateChar()` and the full profile's exact slot-2 setup cursor
  (`00 0E 15 17 11 0E 00 00`). Date/time edit screens must display character
  2 followed by a space, matching the reference UI.

### Keypad

- PA0 = MENU, PA1 = UP, and PA2 = DOWN.
- Inputs are active LOW with internal pull-ups.
- Preserve key-event and debounce behavior unless requirements change.
- Keep `KEY_WaitKey()` profile-aware: the full application uses cooperative
  delays so light/audio services continue, while diagnostic/MP3 profiles use
  `Delay_Ms()`.
- Preserve the full bell loop's 20 ms idle cadence, 600 ms schedule cadence,
  256-byte schedule/sequence buffers, and fixed-announcement relay settling
  delay unless the source-of-truth project changes.

### RTC

- DS1307 address is 0x68.
- I2C1 uses PB6 SCL and PB7 SDA.
- Check wiring, pull-ups, power, backup arrangement, address, and bus state
  before rewriting the driver.
- The cooperative foreground is the only high-level I2C1 owner; do not call
  RTC APIs from an ISR. The light service must obtain its own fresh RTC value
  once per second and leave the output unchanged when a read fails.

### Serial

- The project debug helper uses USART1 TX on PA9.
- PA10/RX is routed on the schematic but is not configured by the current debug
  helper.
- Record the caller-selected baud rate before declaring the serial configuration
  complete.
- The full `main()` retains `USART_Printf_Init(115200)` for existing application
  logging. Reconfirm 115200 against the intended terminal setup on hardware;
  this was chosen, not measured.

### Cooperative Runtime

- Do not reintroduce FreeRTOS tasks, queues, semaphores, mutexes, timers, heap,
  tick hooks, or context switching without a new measured requirement.
- Keep `App_DelayMs()` cooperative and keep lengthy work on demand. Do not put
  file I/O or decoding in TIM3 or audio interrupts.
- The former vendored FreeRTOS tree and `FreeRTOSConfig.h` were removed during
  Git-ready cleanup. Do not add them back to include paths/source lists.

## Library Rules

1. Prefer a project-local wrapper around third-party libraries.
2. Keep application code independent from library internals where practical.
3. Do not duplicate functionality already supplied by a library.
4. Preserve third-party source and licenses whenever possible.
5. If a library modification is unavoidable:
   - Explain why it is needed.
   - Keep the patch minimal.
   - Record the exact change in `REQUIREMENTS.md`.
   - Record implementation completion in `PROGRESS.md`.
6. Helix modifications already present and documented are:
   - single-channel configuration;
   - rejection of excessive channel counts;
   - static decoder allocation;
   - defined signed RISC-V multiply accumulation and unsigned `CLZ()` shifts;
   - compile-time exclusion of stereo-only accesses from the mono image;
   - optional per-granule output handoff that removes the forced MPEG-1 gap.
   - application-specific MPEG-1 Layer III mono/32-kHz/64-or-128-kbps tables
     and parser;
   - rejection/removal of unreachable free-bitrate scanning, while retaining
     indexed CBR/VBR, CRC-header skipping, and ID3 handling;
   - exact per-frame Layer III slot calculation in place of its ROM table.
7. Do not replace or remove those changes without a link-map, decode test, and
   documented reason.

## Documentation Rules

Whenever implementation changes:

- Update `document/README.md` for architecture, overview, pin, or status
  changes.
- Update `document/REQUIREMENTS.md` for behavior, dependencies, constraints,
  tests, or acceptance criteria.
- Update `document/PENDING_TASKS.md` so it contains unfinished work only.
- Append completed/historical work to `document/PROGRESS.md` without deleting
  history.
- Update this file if the change introduces a lasting development rule.
- Mark unknown information as `TBD`.
- Do not allow source code and documentation to contradict each other.

## Change Validation

Before declaring a firmware or hardware task complete:

1. Confirm the target MCU and package.
2. Confirm PCB pin mapping.
3. Review clock/timer ownership and peripheral conflicts.
4. Build the intended target with the configured WCH toolchain.
5. Review warnings and the linker memory report.
6. Flash the intended target PCB.
7. Verify behavior with the appropriate instrument or interface.
8. Record the actual result and any limitation.
9. Update `PENDING_TASKS.md` and `PROGRESS.md`.
10. Synchronize overview and requirements when behavior changed.
