# Pending Tasks

This file contains unfinished work only. Earlier peripheral test results are
recorded in `PROGRESS.md`. The default complete cooperative application now
fits the G8R6 Flash and RAM regions. Maximum-volume plain/ENC quality,
playback counters, integrated hardware flow, astronomy timing, and electrical
timing remain to be verified.

## Build and Target Validation

- [ ] Flash the restored pre-WAV default full-profile image and confirm `Loading...`
  advances to the reference idle/date display after the 600 ms service
  interval; then run the complete menu, schedule, Panchang, and audio flow on
  CH32V203G8R6 hardware. The clean ELF has a strong `TIM3_IRQHandler` and its
  vector entry points to that handler; the remaining check requires hardware.
- [ ] Inspect the runtime main/ISR stack high-water mark under Panchang plus
  nested audio DMA/TIM activity; the static link leaves 668 bytes of RAM.
- [ ] Measure `calc_panchang()` execution time on target.

## Post-Integration Peripheral Validation

- [ ] Resolve the confirmed ESP-vs-CH32 loudness gap before further gain edits.
  With the same file and load, measure RMS and peak-to-peak voltage at ESP
  GPIO25/GPIO26, CH32 PA8, LM358 pin 1 before the 10 uF output capacitor,
  `AUX_OUT`, and the downstream amplifier input/output.
  Record the amplifier IC/module, supply, load impedance, gain-setting parts,
  and whether the ESP DAC channels are summed or used differentially.
- [ ] Verify the R1 populated audio values against the schematic: 4.7 kOhm,
  10 kOhm, two 10 nF shunts, 1 uF input coupling, 1 kOhm input, 10 kOhm
  feedback, 10 kOhm/10 kOhm bias, 10 uF output coupling, and 100 nF supply
  decoupling.
- [ ] Flash V10 and confirm the first LCD screen is
  `ENC+MP3 V10 / AUTO GAIN VOL255`; verify boot proceeds directly from
  `ARITH PASS` to `MOUNTING SD` with no RAW PA8 or DMA sine test.
- [ ] Press DOWN and test representative low-level downloaded `.mp3` files:
  confirm significantly louder, clear output at normal speed/pitch, with no
  clipping, distortion, pumping, hiss, or unwanted noise. Record `P:`, `D:`,
  and underflow count; require zero production underflows.
- [ ] Test short spoken/bell clips specifically. Confirm the new 32x initial
  envelope makes the first syllable/tone clearly audible and that the
  block-look-ahead reduction prevents overload when a file starts loudly.
- [ ] Press UP and test the selected ENC through the shared bounded gain path.
  Confirm louder quiet passages without clipping, distortion, pumping, hiss,
  speed/pitch changes, decryption/decoding errors, or nonzero production
  underflows. Record final metadata/counters.
- [ ] Exercise V10 error handling with a missing folder, empty folder, wrong-key
  `.enc`, truncated ID3/file, unsupported codec/stereo/rate, oversized frame,
  mid-stream format change, SD read interruption, and forced DMA timeout.
- [ ] Measure the V10 PA8 carrier near 375 kHz and the 32 kHz sample-update
  timing during file playback. Measure a representative 44.1 kHz asset
  before accepting the fractional ARR clock path.
- [ ] Measure DC at LM358 pins 3 and 1 with idle PWM duty 128; both should be
  near the nominal 2.5 V bias without oscillation.
- [ ] Scope PA8/AUX and LM358 pin 1 during representative MP3 playback. Record
  PWM carrier, filtered amplitude, DC bias, rail clipping,
  ringing/oscillation, and `AUX_OUT` amplitude.
- [ ] Document the actual downstream amplifier/speaker connected to `AUX_OUT`;
  LM358 is a signal-conditioning op amp, not the speaker power stage.
- [ ] Verify that floating LM358 channel-B pins 5/6/7 do not oscillate or inject
  noise. Record a future-board correction that biases pin 5 at 2.5 V and ties
  pin 7 to pin 6; do not bodge the manufactured PCB without review.
- [ ] Add/check a runtime high-water guard during repeated MP3 playback and
  full schedule/Panchang execution; the stack is now 2,048 bytes, but interrupt
  nesting is not measured on hardware.
- [ ] Reflash the standalone diagnostic and revalidate `TEST.TXT` first-page,
  UP/DOWN paging, MENU exit/reopen, and missing/empty/open/read-error screens
  after the bare-metal key-wait correction.
- [ ] Revalidate RTC detection, read/write, scheduling use, and persistence in
  the merged cooperative application, including the light service's independent
  one-second reads during menu waits and audio DMA waits.
- [ ] Revalidate SD/FatFs access with `settings.txt`, `playlist.txt`,
  `holiday.txt`, `menu.txt`, `festival.txt`, `special.txt`, and audio assets.
- [ ] Confirm `RLY_IN`/`LGT_IN` active-level polarity against the PCB stages.
- [ ] Characterize the R1 AUX/LM358 path across required sample
  rates and confirm adequate PWM-carrier rejection.
- [ ] At the new maximum digital level, scope PA8, LM358 pin 1 before
  the coupling capacitor, and `AUX_OUT`; verify no flat-topping or bias/swing
  violation. Measure downstream amplifier supply current and amplifier/speaker
  temperature during a sustained worst-case asset. Reduce the digital target
  if any hardware limit or audible distortion is observed.
- [ ] Compare ESP DAC and CH32 `AUX_OUT` RMS voltage with the same decoded file
  and load. The ESP enables both internal DAC channels while CH32 has one PA8
  PWM output; document how the external amplifier is connected before
  attributing any remaining level difference to software.

## Functional Testing

- [ ] Verify the internal-flash config store on target, including power-loss
  persistence, slot rotation, corrupt-newest fallback, and writes during audio.
- [ ] Test `settings.txt` parsing including all supported timezone formats,
  the exact `HappyBell 2025 - ` welcome prefix, and ignored extra lines.
- [ ] Test holiday skip, playlist types `1`/`2`/`3`, every supported sequence
  token, group rotation/wrap, and trigger-window power-loss resume.
- [ ] Compare Panchang output with `C:\OfficeWorks\esp_bell_idf` for known
  dates, including Tamil solar-month rollover at sunset, and verify
  festival/special-day matching.
- [ ] Exercise every menu screen, verify the correct config-store locations,
  confirm `menu.txt` labels retain their original case, and verify that
  date/time editors show the reference CGRAM slot-2 cursor glyph.
- [ ] Verify `.enc` playback matches the equivalent `.mp3`.
- [ ] Re-run the host PCM comparison after deployed-profile specialization,
  covering the actual mono MPEG-1 Layer III 32-kHz 64/128-kbps corpus,
  padding, CRC-protected headers if present, ID3v2, plain MP3, and decrypted
  ENC input. The asset files were unavailable during the final clean build.
- [ ] On hardware, verify representative transition-boundary announcements.
  Host comparison found maximum tithi/nakshatra end-time differences below two
  minutes, but five-minute spoken rounding differed in 57/41 of 612 monthly
  cases; confirm the product tolerance before release.
- [ ] Validate PA8 audio carrier/sample timing, `AUX_OUT`, stop behavior, and
  repeated playback; require zero underflows for production assets and resolve
  any residual granule decode-time gap.
- [ ] Test light scheduling for normal and midnight-spanning windows.

## System Integration

- [ ] Confirm the intended load connected to R1 `AUX_OUT`. Do not connect a
  4/8-ohm passive speaker directly to LM358; qualify the actual downstream
  power stage or high-impedance input.
- [ ] Run a full scheduled bell/announcement day including Panchang playback
  and a holiday-skip day.
- [ ] Document and verify the final UART terminal setup and whether RX is used.
- [ ] Perform long-duration stability and repeated-playback testing.
- [ ] Resolve remaining hardware and software `TBD` items needed for release.
