# HAPPY BELL

## Short Description

HAPPY BELL is embedded firmware for a CH32V203-based controller. Initial
peripheral development used a CH32V203C8T6 development kit; the final custom
PCB uses a CH32V203G8R6.

The repository contains drivers for a parallel LCD, three-button keypad,
DS1307 RTC, SPI microSD card, PWM output, and serial monitoring. It now also
contains a CH32V203 port of the fixed-point Helix MP3 decoder. The MP3 layer is
designed to read a supported file through FatFs, decode into PCM, and drive
the PCB `AUX` audio path with an 8-bit PWM carrier on PA8/TIM1_CH1. Hardware
revision R1 replaces the former TLV9061 network with an LM358 stage powered
from `+5V_IN`. `AUX` passes through 4.7 kOhm/10 nF and 10 kOhm/10 nF passive
filter sections, a 1 uF coupling capacitor, and a 1 kOhm input resistor into
LM358 channel A. Its 10 kOhm feedback resistor gives an inverting signal gain
of -10 around a 2.5 V bias made by two 10 kOhm resistors. The output reaches
`AUX_OUT` through 10 uF. The schematic provides 100 nF supply decoupling but
shows channel B pins 5, 6, and 7 unconnected. `AUX_OUT` remains a line-level
output; the schematic does not add a low-impedance speaker power stage.

The complete application uses a foreground cooperative scheduler instead of
FreeRTOS. The unused vendored kernel has been removed from the Git-ready
project. The complete cooperative
Astro/MP3 image fits G8R6 memory and is the default build. Separate MP3
hardware-test and SD/LCD diagnostic profiles remain selectable. The temporary
2 kHz WAV/PWM test and combined ENC/WAV service were removed when the firmware
was restored to its exact pre-WAV source state.

## Adopted Reference Project

The project's end-user behavior is adopted from the source-of-truth project at
`C:\OfficeWorks\esp_bell_idf`, an ESP32/ESP-IDF bell and announcement
controller with the same
front-panel hardware concept (LCD, 3 keys, DS1307 RTC, SD card, relay/light
outputs, local MP3 playback). HAPPY BELL is adopting `esp_bell_idf`'s
**non-radio** functionality onto the CH32V203 bare-metal target:

- SD-card-driven scheduled bell / music / announcement playback
  (`playlist.txt`).
- Holiday skip logic (`holiday.txt`).
- Tamil Panchangam astronomy/panchang calculation and voice-clip playback.
- Festival and special-day matching (`festival.txt`, `special.txt`).
- Front-panel setup menu driven by the 3-key keypad and 16x2 LCD
  (`menu.txt`).
- Relay (amplifier) and light on/off scheduling.
- Persistent per-feature settings (`settings.txt`) and an EEPROM-style
  bit/byte-addressable configuration store.
- Optional simple-XOR-encrypted (`.enc`) audio playback alongside plain
  `.mp3`.

**Explicitly out of scope for this port:** Bluetooth A2DP sink mode, WiFi,
HTTP/RTSP audio streaming (`webs.txt`, the `l` playlist token), and NTP time
sync. These are `esp_bell_idf`-only features tied to the ESP32 radio stack and
are not implemented here.

**Port status: reference-parity audit completed (2026-08-26); V8 plain-MP3 playback is
distortion-free at normal speed through the physical speaker; V9 `BK26` `.enc`
discovery, decryption, decoding, and physical playback are confirmed working;
the current image applies maximum-safe digital peak normalization to both
plain and encrypted playback and removes the RAW PA8 and DMA sine diagnostics.
The +30 dB-envelope hardware retest is clearer but remains noticeably quieter
than ESP-BELL; the remaining mismatch is downstream of the already-full-scale
digital path and requires analog/channel measurement.** All profiles compile
through their source/object stages with the installed WCH toolchain. The
diagnostic, MP3-test, and complete profiles link for the G8R6. Earlier
peripheral tests passed before the FreeRTOS application was
merged; those results are preserved in
`document/PROGRESS.md`.

## Source Layout

| File | Role |
|---|---|
| `Firmware/User/happybell_build.h` | selects the standalone SD/LCD diagnostic, MP3 hardware test, or default full profile |
| `Firmware/User/main.c` | profile-aware boot, cooperative full-application flow, standalone text diagnostic, or local-audio test |
| `Firmware/User/app_runtime.c/.h` | full-profile 1 ms TIM3 clock and cooperative delay/service dispatch |
| `Firmware/User/bell.c` | settings/holiday/playlist parsing, sequence playback, panchang announcement, light service, clock display |
| `Firmware/User/menu.c` | front-panel setup screens |
| `Firmware/User/mydef.h` | EEPROM map, audio folder IDs, shared prototypes |
| `Firmware/User/eeprom.c/.h` | flash-emulated config store (8x256B fast pages, round-robin, checksummed; sim-tested on host) |
| `Firmware/User/storage.c/.h` | FatFs mount, file/folder/line helpers, and bounded extension-based file discovery |
| `Firmware/User/sd_text_viewer.c/.h` | blocking LCD text-file pager used by the standalone `TEST.TXT` diagnostic |
| `Firmware/User/player.c/.h` | on-demand cooperative audio wrapper and playback stop/key/deadline service |
| `Firmware/User/rtc_util.c/.h` | weekday/leap/12h/week-of-year/date-string helpers |
| `Firmware/User/relay_light.c/.h` | RLY_IN (PA3) / LGT_IN (PB0) GPIO |
| `Firmware/User/lcd.c`, `keypad.c`, `rtc.c`, `sd*.c` | existing tested drivers (LCD gained cursor/blink/number/region/CGRAM helpers; full-profile keypad waits keep cooperative services running) |
| `Firmware/Astro/tamil_panchangam.c/.h` | astronomy engine, copied from `esp_bell_idf` with only the special/festival file readers adapted to FatFs |
| `Firmware/LibHelixMP3/ch32_mp3_player.c` | Helix wrapper: validated `.mp3`/`.enc` stream input, bounded all-file peak normalization, exact-rate DMA PWM output, metadata, and stop request |
| `Firmware/Core`, `Peripheral`, `Debug` | WCH G8R6 device headers, peripheral drivers, core support, and UART diagnostics used by the managed build |
| `Firmware/Startup/startup_ch32v20x_D6.S`, `Firmware/Ld/Link.ld` | G8R6 startup/vector code and 64 KB Flash/20 KB RAM layout |
| `Firmware/.project`, `.cproject`, `.template`, `CH32V203G8R.wvproj`, `CH32V203G8R.launch` | MounRiver Studio target, compiler, linker, flash, and debug configuration |
| `tools/build_g8r6.ps1` | reproducible WCH GCC 8 build; rejects output whose HEX hash differs from the working reference |
| `tools/encode_mp3.ps1` | creates a firmware-compatible `BK26` XOR `.enc` file from a supported MP3 |
| `tools/helix_host_decode.c`, `tools/panchang_compact_compare.py` | host regression tools for decoder and Panchang equivalence |
| `Firmware/release/CH32V203G8R6_full.hex` | verified working reference image; checksum is stored beside it |
| `hardware/HappyBell-CH32V203G8R6_R1.pdf` | authoritative three-page R1 schematic with the LM358 audio stage |

The old RTC/keypad/LCD test application, legacy 100 kHz `pwm.c` test, FreeRTOS
archive, non-G8R6 startup variants, C8T project files, cached bytecode, and old
build/optimization output trees are absent from the Git-ready project. Their
historical results remain in `document/PROGRESS.md`.

## Reproducible Build and Runtime Data

Open `Firmware` as a MounRiver Studio project for the normal managed build, or
run `pwsh -File tools/build_g8r6.ps1` with PowerShell 7 or later. The script
uses the WCH GCC 8 toolchain installed with MounRiver Studio, writes only to the
ignored `Firmware/obj` directory, and verifies the final HEX SHA-256 against
the working reference.

No audio or control asset is compiled into the firmware. At runtime the SD card
must supply the deployed `settings.txt`, `playlist.txt`, `holiday.txt`,
`menu.txt`, `festival.txt`, `special.txt`, and referenced `.mp3`/`.enc` files.
Those deployment-specific files were not present in this repository and were
therefore neither deleted nor added during cleanup.

## Hardware Description

### Development / Testing Hardware

- CH32V203C8T6 development kit used for initial peripheral development.
- Oscilloscope used for PWM measurement.
- Serial monitoring was tested; the PC terminal application is **TBD**.

### Final / Custom PCB

- MCU: CH32V203G8R6.
- MCU clock configuration: 96 MHz from the internal 8 MHz HSI through the
  x12 PLL. The reviewed G8R6 schematic shows no external HSE crystal, so the
  previous HSE/PLL selection could leave the PCB at the 8 MHz reset clock.
- LCD: parallel 4-bit interface; exact model **TBD**.
- Inputs: MENU, UP, and DOWN switches.
- RTC: DS1307 through I2C1.
- Storage: microSD card through SPI1 and FatFs.
- Audio: PA8/TIM1_CH1 PWM on `AUX`. R1 hardware uses a 4.7 kOhm/10 nF section,
  then a 10 kOhm/10 nF section, 1 uF AC coupling, and 1 kOhm into LM358 pin 2.
  Pin 3 is biased at nominally 2.5 V by 10 kOhm to `+5V_IN` and 10 kOhm to
  ground. A 10 kOhm pin-1-to-pin-2 feedback resistor sets signal gain -10;
  pin 1 is AC-coupled to `AUX_OUT` by 10 uF. LM358 pin 8 uses `+5V_IN`, pin 4
  is ground, and a 100 nF capacitor decouples the supply. Pins 5/6/7 of the
  unused channel are unconnected in the drawing and should be terminated to a
  defined stable configuration in the next hardware revision.
- Other schematic outputs: relay (`RLY_IN`) and light-control (`LGT_IN`)
  signals. GPIO and scheduling/application logic are implemented; active-level
  polarity and complete hardware behavior remain pending.
- Persistent configuration storage: CH32V203 internal flash emulation using
  eight rotating 256-byte fast pages. Host simulation passed; on-target and
  power-loss testing remain pending.
- Power shown in the schematic: `+5V_IN` and AMS1117-3.3-derived `+3V3`.
- Programming/debug interfaces shown in the schematic: SWDIO, SWCLK, NRST,
  BOOT0, RX, and TX.

## Pin Configuration

Package-pad numbers below are taken from the CH32V203G8R6 schematic. Signals
marked **TBD** are present in hardware but are not confirmed as active in the
applicable build profile.

| Pad | MCU Pin | Peripheral / Function | Signal | Usage / Notes |
|---:|---|---|---|---|
| 1 | PA14 | SWCLK | SWCLK | Programming/debug clock |
| 2 | PB6 | I2C1_SCL | SCL | DS1307 clock |
| 3 | PB7 | I2C1_SDA | SDA | DS1307 data |
| 4 | BOOT0 | Boot configuration | BOOT0 | 10 kOhm pull-down shown |
| 5 | PB8 | GPIO output | LCD_D7 | LCD data bit 7 |
| 8 | NRST | Reset | NRST | Reset/debug connection |
| 9 | PA0 | GPIO input | SWT1 / MENU | Active LOW; internal pull-up |
| 10 | PA1 | GPIO input | SWT2 / UP | Active LOW; internal pull-up |
| 11 | PA2 | GPIO input | SWT3 / DOWN | Active LOW; internal pull-up |
| 12 | PA3 | GPIO output | RLY_IN | Relay-control input; firmware behavior **TBD** |
| 13 | PA6 | SPI1_MISO | SD_DO | SD data to MCU |
| 14 | PB0 | GPIO output | LGT_IN | Light-control input; firmware behavior **TBD** |
| 15 | PA4 | GPIO output | SD_CS | SD chip select, active LOW |
| 16 | PA5 | SPI1_SCK | SD_CLK | SD clock |
| 17 | PA7 | SPI1_MOSI | SD_DI | SD data from MCU |
| 18 | PB10 | GPIO output | LCD_D4 | LCD data bit 4 |
| 19 | PB11 | GPIO output | LCD_D5 | LCD data bit 5 |
| 20 | PB12 | GPIO output | LCD_D6 | LCD data bit 6 |
| 21 | PB13 | GPIO output | LCD_RS | LCD register select |
| 22 | PB14 | GPIO output | LCD_EN | LCD enable |
| 24 | PA8 | TIM1_CH1 | AUX | MP3-test/full profiles: audio PWM; SD/LCD-only profile: not initialized |
| 25 | PA9 | USART1_TX | TX | Serial connector; runtime settings **TBD** |
| 26 | PA10 | USART1_RX | RX | Serial connector; runtime settings **TBD** |
| 28 | PA13 | SWDIO | SWDIO | Programming/debug data |

## Full-Profile Cooperative and Shared Audio DMA Architecture

- **Scheduler**: TIM3 supplies a 1 ms monotonic clock. The foreground loop
  invokes audio only when requested, checks the light service once per second,
  and runs the bell/menu service at the original 20 ms cadence. The bell's
  schedule evaluation remains at 600 ms.
- **Playback yielding**: Helix decode remains blocking as it was in the former
  high-priority audio task. Each existing 1 ms DMA-slot wait services the
  independent light schedule and the configured key/deadline polling path, so
  no new blocking interval is introduced in the places where the RTOS formerly
  allowed lower-priority work to run.
- **RTOS removal**: task control blocks/stacks, the 6 KB heap, scheduler lists,
  queue/semaphore state, tick/context-switch handlers, malloc/stack hooks, and
  the RTC mutex are absent. The single foreground owner serializes RTC/FatFs
  access. The former `Firmware/FreeRTOS` tree and `FreeRTOSConfig.h` were
  removed during Git-ready cleanup and are not build dependencies.
- **Debug UART**: the full profile retains `USART_Printf_Init(115200)` and all
  application/audio logging. Reconfirm the baud/terminal setup on hardware.
- **Audio DMA path**: see `Firmware/LibHelixMP3/ch32_mp3_player.c`. TIM2's
  sample-rate update event now drives a DMA1 Channel2 request that writes
  each prepared PCM sample directly into `TIM1->CH1CVR`, instead of a
  per-sample duty-update ISR. Volume scaling and the PWM-duty bias run
  once per 576-sample decoded granule (same formula as before, just moved out
  of the real-time path) instead of once per sample. MPEG-1 uses a Helix
  granule-output hook: row 0 is handed to DMA while row 1 is decoded, and the
  next frame begins as soon as row 0 is free. This removes the previous forced
  full-frame silence gap without another RAM buffer. A small circular
  "silence" buffer keeps the PWM carrier alive with no audible glitch when
  no decoded data is ready, preserving the existing underflow-counting
  semantics. WCH's CH32FV2x/V3x reference-manual DMA table confirms TIM2_UP
  on DMA1 Channel2. The channel is shared with SPI1_RX and TIM1_CH1; the
  current SD driver is polled and does not claim the DMA request.
  Rates that divide 96 MHz exactly, including the validated 32 kHz ENC assets,
  use a fixed TIM2 period with no TIM2 ISR. For the 44.1 kHz family only, a
  short phase-accumulator ISR alternates adjacent TIM2 periods to produce the
  exact long-term decoded sample rate; DMA still owns every duty write.
- **Relay/light GPIO**: the retained full profile uses
  `Firmware/User/relay_light.c/.h` to initialize `RLY_IN` (PA3) and `LGT_IN`
  (PB0), control the relay around playback, and evaluate the light schedule in
  a low-priority task. The current owner-authorized MP3 test also initializes
  these GPIOs and asserts PA3 only during file playback.
  External relay/amplifier behavior remains to be observed.

## Project Flow

### Default G8R6 ENC/MP3 Test (V10 link PASS; volume retest pending)

1. Boot and initialize the system clock, short-delay wrapper, LCD, keypad, and
   relay GPIO. Show the unique `ENC+MP3 V10 / AUTO GAIN VOL255` marker and halt
   with `CLOCK ERROR / NOT 96MHZ` if the runtime clock check fails. The
   focused image does not initialize or print through UART.
2. Run the retained signed fixed-point arithmetic self-test and halt on
   `ARITH FAIL`. RAW PA8 and DMA sine-wave startup tests no longer exist.
3. Show `MOUNTING SD`, mount through SPI1/FatFs, and halt with `SD CARD ERROR`
   if mounting fails.
4. Scan folder `001` through FatFs and select the alphabetically first `.enc`
   short filename case-insensitively. Show a specific missing, directory-I/O,
   or path-length error instead of attempting playback with an invalid path.
5. Validate/decrypt the selected file, show `PLAYING:` plus its path, decode it
   through Helix, and stream PCM through PA8/TIM1/TIM2/DMA. Disable PA3 after
   playback.
6. Show status, peak, completed DMA slots, underflows, decoded sample rate,
   16-bit/channel/bitrate information, compressed frame bytes, and samples per
   frame.
7. Show `UP:ENC DOWN:MP3`; both selections use caller volume 255 and the same
   bounded peak-normalized, finally saturated PWM path. UP replays the selected
   `.enc`; DOWN plays root `mp3_song.mp3`.

### Default Full Application

1. Boot: 96 MHz clock, debug UART (115200), LCD and reference setup-cursor
   glyph, keypad, DS1307, relay/light GPIO. Halt with a visible LCD error if
   the DS1307 or SD card is missing.
2. Mount the SD card via SPI1/FatFs (stays mounted; all files read directly
   from SD, no SPIFFS-equivalent staging step).
3. Load the flash-emulated config store and `settings.txt` (welcome text,
   latitude, longitude, timezone, calendar mode, panchang playback order).
4. Start the TIM3 millisecond clock and enter the cooperative foreground loop:
   bell/menu every 20 ms, independent light/RTC check every second, and audio
   decode only after a playback request.
5. Bell idle loop:
   - MENU key: setup chain (bell/starting-music selection, night-play
     window, bell start/end hours, date/time set, light on/off times, amp
     toggle, per-entry playlist enable via `menu.txt`).
   - UP key: assert the amplifier relay, retain the reference settling delays,
     and play announcement track `00001`.
   - DOWN key: toggle the amplifier relay when enabled.
   - Wait cooperatively between 20 ms idle iterations. Every ~600 ms: read RTC;
     at second 0-2 inside the allowed play window,
     check `holiday.txt` then evaluate `playlist.txt` and play any matching
     scheduled sequence; otherwise scroll the welcome text and show
     time/date.
6. A matched sequence computes the day's Panchang once, then plays the local
   tokens (`f g m h w d M W D s b F S p`), honoring group rotation,
   per-token enable bits, the duration
   cutoff, and power-loss resume via the config store.

### Intended Full-Profile MP3 Playback Path

1. Mount the SD card through FatFs and open the requested MP3 file.
2. Fill a bounded input buffer and locate MP3 frame synchronization.
3. Reject non-MPEG-1, non-mono, non-32-kHz, oversized, or unsupported-bitrate
   input.
4. Decode the deployed mono MPEG-1 Layer III profile at 32 kHz and indexed
   64/128 kbps.
5. For each 1,152-sample frame, queue its first 576-sample granule while Helix
   decodes the second. Begin the next frame when row 0 becomes free, then wait
   for row 1 only immediately before overwriting it. This removes the old
   mandatory per-frame gap without increasing static RAM.
6. Generate an 8-bit PWM carrier on PA8/TIM1_CH1.
7. Use TIM2 update events to trigger DMA1 Channel2 PWM-duty transfers at the
   MP3 sample rate.
8. Feed the schematic's `AUX` analog stage and `AUX_OUT` connector.
9. Stop timers, close the file, and release the decoder instance when playback
   finishes or an error is reported.

## Logic / Working Principle

- Keypad inputs are active LOW and use internal pull-ups.
- The LCD reports MP3/SD/file status in the default profile and adds RTC values
  and application status in the full profile.
- The RTC driver communicates with the DS1307 at address 0x68 over I2C1.
- The SD layer uses SPI1 and exposes storage to FatFs.
- The MP3 wrapper keeps application code separate from Helix internals.
- False sync patterns inside compressed payload or trailing metadata are
  skipped; one unsupported-looking candidate no longer aborts a valid file.
- A false header that requests more than a full input buffer is also skipped
  for the validated 288-byte-frame test asset.
- MP3 decoding runs in the foreground in both the MP3-test and full profiles.
  DMA transfers prepared samples into
  the TIM1 duty register; its completion interrupt performs bounded handoff.
- The ESP reference has no programmable software-amplifier gain: it converts
  full-scale signed PCM directly to the full 8-bit DAC range. CH32 callers
  likewise select the maximum volume value 255. Before PWM conversion, both
  plain and encrypted files pass through one bounded Q8 peak normalizer. It
  starts each file at 32x for short/quiet announcements, peak-scans every block
  before DMA, reduces loud blocks immediately toward a 32,512 target, and
  recovers smoothly by at most 1x per block. A 254/256 minimum gain
  keeps full-scale content off the PWM endpoints; final duty is always 1..254.
  The earlier unbounded fixed 4x trial remains removed.
- Audio initialization resets TIM1/TIM2/DMA and initializes every advanced
  TIM1 output-compare field before enabling PA8; this prevents uninitialized
  complementary/idle fields from suppressing the main PWM output.
- The managed project contains no FreeRTOS sources; linked images contain only
  the startup file's normal weak `SysTick_Handler`/`SW_Handler`, with no task,
  queue, semaphore, heap, or context-switch symbol.
- Read-only FatFs uses `FF_FS_TINY=1`, so `FIL` objects share the volume's
  existing 512-byte sector window instead of placing another 512-byte cache in
  the blocking MP3 function's stack frame.
- A small `.noinit` fault record survives `NVIC_SystemReset()`. The MP3 wrapper
  updates its current file/read/sync/decode/audio/DMA stage, allowing the next
  boot to display where a HardFault occurred.
- `CH32_MP3_GetUnderflowCount()` exposes missed buffer handoffs for hardware
  validation.
- PA8 is owned by the MP3 audio path in the default MP3-test and full profiles;
  the SD/LCD-only profile leaves it uninitialized. The legacy fixed 100 kHz
  test has been removed from the build.

## Current Status

**The default complete cooperative G8R6 application has been restored to the
exact source and binary state used before the 2 kHz WAV/PWM test. A clean
rebuild uses 61,352 bytes of Flash and 19,812 bytes of RAM, leaving 2,136 bytes
in the configured 63,488-byte application region and 668 bytes of SRAM. Its
HEX is byte-for-byte identical to the preserved pre-WAV image. Integrated
hardware regression and stack high-water validation remain open.**

The 2026-08-26 audit mapped the complete reference boot flow, idle/menu flow,
RTC scheduling, light service, settings and menu formats, holiday and playlist
matching, sequence-token handling, Panchang calculation, persistent settings,
local MP3/ENC playback, and error/relay sequencing. The CH32 full-profile
source preserves that behavior, with direct FatFs reads, internal-flash
settings, WCH peripheral drivers, and PA8 PWM/DMA replacing ESP-specific
storage and audio hardware. Its light service reads the DS1307 independently
once per second; single-foreground ownership makes an RTC mutex unnecessary. Menu labels retain
their source-file case; unsupported sequence tokens are ignored safely; and no
radio-only source branch or build dependency remains.

The complete cooperative application is now the default build. The restored
pre-WAV production link uses **61,352/63,488 bytes of Flash** and
**19,812/20,480 bytes of RAM**, leaving 2,136 bytes of application Flash and
668 bytes of RAM. Relative to the measured
117,704-byte Flash/28,432-byte RAM complete baseline, the current link saves
56,352 bytes of Flash and 8,620 bytes of RAM. FreeRTOS, general formatted I/O,
double-precision helpers, and libm are absent.

The corrected compact Panchang engine covers 2000-2050 and now compares against
the actual ESP C implementation's VSOP/ELP output. Across 2,662 weekly sunrise
samples, tithi/nakshatra mismatches were 1/0 and yoga mismatches were 4;
maximum solar/lunar longitude differences were 0.012/0.019 degrees.
Sunrise/sunset differed by at most 0.024 minute and moonrise/moonset by
0.085/0.089 minute over 120 location/date cases. MP3 remains deliberately
restricted to MPEG-1 Layer III mono, 32 kHz, indexed 64 or 128 kbps.

Earlier custom-PCB tests reported `PASS` for LCD, keypad, DS1307
read/write/persistence, SD/FatFs/`TEST.TXT` paging and errors, and the removed
legacy PWM test. V8 plain MP3 is confirmed at normal speed without audible
distortion, and V9 encrypted playback is confirmed through the physical
speaker. Maximum-volume plain/ENC quality, production counters/underflows, full-flow
hardware integration, relay/light polarity, and long-duration testing remain
pending. Detailed history is maintained only in `PROGRESS.md`.

The latest target report confirms that the 32x quiet-source envelope improves
audibility but does not match ESP-BELL loudness. At a full-scale decoded peak,
ESP emits DAC codes 0..255 while CH32 deliberately emits PWM duties 1..254, a
peak-to-peak difference of only 0.07 dB. That cannot explain a noticeable
speaker-level difference. The remaining comparison must measure ESP GPIO25/26,
CH32 PA8, LM358 pin 1, `AUX_OUT`, and the actual power-amplifier input with
the same file and load.
