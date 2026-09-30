# Requirements

## Project Overview

HAPPY BELL is firmware for a CH32V203-based controller. Peripheral development
began on a CH32V203C8T6 development kit and is being transferred to a custom
CH32V203G8R6 PCB. The repository contains LCD, keypad, RTC, SD/FatFs, PWM, and
debug-support modules plus a fixed-point Helix MP3 port.

The implemented MP3 wrapper is intended to stream a supported MP3 file from the
microSD card, decode it without dynamic allocation, and output audio through an
8-bit PWM signal on the PCB `AUX` net. The complete cooperative application is
the production G8R6 profile, is the source default, and fits the configured
Flash and RAM regions. Earlier
bounded hardware tests produced clean plain MP3 output and confirmed `.enc`
playback. The current image adds bounded high-makeup gain for low-level plain
and encrypted files; its final loudness and quality remain under validation. Separate MP3 hardware-test
and SD/LCD diagnostic profiles remain selectable.

The project's full end-user functionality is adopted from
`C:\OfficeWorks\esp_bell_idf`, the source-of-truth ESP32/ESP-IDF
bell/announcement controller with the same front-panel concept, excluding its
Bluetooth and
WiFi features. `esp_bell_idf`'s non-radio source has been fully reviewed:
`main/main.cpp`, `main/bell.cpp`, `main/menu.cpp`, `main/mydef.h`,
`main/config.hpp`, and the drivers under `main/driver/`: `astro`,
`eeprom`, `keys`, `lcd`, `mp3_player` (local-MP3 path only),
`rtc`, and `storage`. `main/driver/wifi_player` was reviewed only to identify
what to exclude and is out of scope. The sections below define the adopted
scope in CH32V203/WCH SDK and cooperative-runtime terms.

### Full-Port Scope Summary

| Feature | `esp_bell_idf` source | Adopt? |
|---|---|---|
| Settings load (welcome/lat/lon/tz/calendar/panchang order) | `bell.cpp: loadSettings()` | Yes |
| Timezone string parsing (`H:MM`, `H.MM`, `H,MM`, decimal) | `bell.cpp: parse_timezone()` | Yes |
| Holiday skip | `bell.cpp: checkLeave()` | Yes |
| Scheduled playlist engine (types `1`/`2`/`3`, local-file tokens only) | `bell.cpp: checkSong/playTime/playWeek/playSequence` | Yes |
| Panchang astronomy calculation | `driver/astro/tamil_panchangam.c` | Yes |
| Festival / special-day matching | `festival.txt`/`special.txt` parsing (astro module) | Yes |
| Front-panel setup menu | `menu.cpp` | Yes |
| EEPROM-style bit/byte config store | `driver/eeprom` | Yes (internal-flash emulation implemented) |
| Relay (amp) control | `main.cpp: setRelay()` | Yes |
| Light on/off scheduling | `bell.cpp: lightTask()` | Yes |
| `.enc` XOR-decrypted local audio | `driver/mp3_player: decryptBuffer()` | Yes |
| Local MP3 decode/playback | `driver/mp3_player` (non-BT path) | Yes (already ported via Helix/`ch32_mp3_player`) |
| `l` web-stream playlist token, `webs.txt` | `bell.cpp`, `driver/wifi_player` | **No** |
| WiFi connect/NTP sync | `driver/wifi_player`, `main.cpp` WiFi calls | **No** |
| Bluetooth A2DP sink | `driver/mp3_player` BT path, `main.cpp` BT callbacks | **No** |
| ESP32-S3 alternate key/LCD pinout | `config.hpp` `CONFIG_IDF_TARGET_ESP32S3` guards | **No** (single fixed G8R6 pinout) |

### Required Platform Adaptations

| Reference implementation | CH32V203G8R6 adaptation | Reason |
|---|---|---|
| ESP-IDF `app_main()` plus an application loop | WCH `main()` runs timed bell/light/audio cooperative services | CH32 startup/runtime model and G8R6 footprint |
| ESP GPIO/I2C/SPI drivers and `config.hpp` pins | Project-local WCH drivers using the schematic pin table | Different MCU and PCB |
| ESP I2S DAC output | PA8/TIM1_CH1 PWM, TIM2 sample clock, DMA1 Channel2, R1 LM358 path | Schematic has PWM `AUX`, not the ESP audio peripheral |
| SPIFFS-staged control files copied from SD | Direct FatFs reads from the mounted SD card | No internal SPIFFS-equivalent filesystem is implemented |
| NVS EEPROM emulation | Checksummed rotating CH32 internal-flash pages | Different persistent-storage API |
| ESP high-resolution timer | TIM3-derived 1 ms monotonic counter | Different platform timer API |
| Shared DS1307 object guarded by a mutex | Project DS1307 API owned by one cooperative foreground | No concurrent high-level I2C caller remains |
| ESP reset-reason/brownout delay and unused GPIO LED helper | Not copied | No equivalent application requirement or MCU-controlled LED pin is confirmed on the G8R6 schematic |
| Verbose Panchang UART dump | Not copied | Debug-only output is not part of bell control flow and materially increases an already oversized image |

The `settings.txt`, `menu.txt`, `holiday.txt`, `playlist.txt`, `festival.txt`,
and `special.txt` formats remain compatible for the adopted local scope. The
first six `settings.txt` lines are consumed. Additional reference-only
connection credentials, if present, are ignored and have no CH32 storage,
state, initialization, callback, task, or build dependency.

## Scope

### Included

- CH32V203 firmware, WCH device support, and cooperative application runtime.
- CH32V203G8R6 custom-PCB target. C8T development history is retained only in
  `PROGRESS.md`; its project/launch files are not part of the release build.
- Parallel LCD interface.
- MENU, UP, and DOWN keypad inputs.
- DS1307 RTC through I2C1.
- SPI1 microSD card driver and FatFs integration.
- Blocking `TEST.TXT` LCD diagnostic with paging and visible errors.
- Historical legacy 100 kHz PWM testing on PA8/TIM1_CH1; removed from the
  current build after its reported passing test.
- Helix fixed-point Layer III decoder port.
- FatFs-to-Helix streaming wrapper.
- TIM1 PWM and TIM2-triggered DMA audio output.
- Serial debug support and peripheral integration testing.
- Three source profiles in `Firmware/User/happybell_build.h`: SD/LCD-only
  diagnostic, default bare-metal MP3 hardware test, and retained full
  cooperative Astro application.

### Included (adopted from `esp_bell_idf`, non-radio scope)

- Settings load from `settings.txt` (welcome text, latitude, longitude,
  timezone string, calendar mode, panchang playback order). SSID/password
  lines are not applicable (no WiFi) and are ignored if present.
- Holiday skip logic from `holiday.txt`.
- Scheduled playlist engine from `playlist.txt`: date-exact (`1`), date-range
  + weekday-mask (`2`), and weekday-mask-only (`3`) schedule types, and every
  sequence token except `l` (web stream): `fNNNN`, `gNNN`, `m h w d M W D`,
  `s`, `b`, `F`, `S`, `p`.
- Tamil Panchangam astronomy/calendar calculation (sunrise/sunset,
  moonrise/moonset, tithi, nakshatra, yoga, karana, rashi, rahu kalam,
  gulikai, yamagandam, durmuhurtam, abhijit, nalla neram, samvatsaram,
  ayanam, rithu, regional/lunar month-day).
- Festival matching from `festival.txt` (6-field and legacy 5-field forms)
  and special-day matching from `special.txt`.
- Front-panel setup menu: bell/starting-music track selection, night-play
  toggle, bell start/end hour window, date/time set, light on/off time set,
  amp on/off toggle, and per-entry playlist enable/disable via `menu.txt`.
- EEPROM-style bit- and byte-addressable persistent configuration store using
  rotating CH32V203 internal-flash fast pages.
- Relay (amplifier) control tied to playback state and the AMP-ON menu
  toggle.
- Light output on/off scheduling against RTC time, evaluated periodically.
- Local audio playback of both plain `.mp3` and XOR-"encrypted" `.enc`
  files (single repeating 4-byte key, adopted verbatim as an obfuscation
  feature, not real encryption).
- Group-rotation (`g` token) and per-token EEPROM enable-bit gating,
  including resume-after-power-loss behavior for a trigger window.

### Not Included or Not Yet Defined

- Bluetooth A2DP sink mode: **out of scope** (explicit user requirement).
- WiFi, HTTP/RTSP streaming, `webs.txt`, the `l` playlist token, and NTP time
  sync: **out of scope** (explicit user requirement).
- Final product decisions beyond the adopted menu/scheduler behavior: **TBD**.
- Exact LCD model: **TBD**.
- Final production MP3/`.enc` asset names, folder layout, and encoding
  settings: **TBD**.
- Legacy 100 kHz PWM is not a final feature; PA8 is assigned to MP3 audio in
  the MP3-test and full profiles and is not initialized by the SD/LCD-only
  diagnostic.
- Persistent storage uses CH32V203 internal flash emulation; on-target
  endurance and power-loss behavior remain to be verified.
- Whether a DS1307 carrier module with an onboard I2C EEPROM (e.g. AT24C32,
  a common combination) is present on the custom PCB: **TBD**, needs
  schematic/hardware confirmation.
- Complete final-PCB electrical and regulatory acceptance scope: **TBD**.
- Stereo MP3 playback on CH32V203G8R6.
- High-latitude day-length edge cases in the Panchang engine: best-effort
  only, matching the `esp_bell_idf` reference.

## Hardware Requirements

| Item | Requirement / Information | Current Status |
|---|---|---|
| Development MCU | CH32V203C8T6 development kit | Used for initial testing |
| Final MCU | CH32V203G8R6 | Target custom PCB |
| System clock | 96 MHz from internal 8 MHz HSI through x12 PLL; boot requires `SystemCoreClock == 96000000` | G8R6 schematic shows no external HSE crystal; V8 playback is normal-speed and distortion-free; physical carrier measurement remains pending |
| LCD | Parallel 4-bit interface; model TBD | Reported working on G8R6 |
| Keypad | MENU, UP, DOWN; active LOW | Pre-integration hardware test PASS; new menu flow needs revalidation |
| RTC | DS1307, I2C address 0x68 | Detection/read/write/persistence reported PASS before full-app merge |
| Storage | microSD over SPI1 | Initialization and FatFs file reading reported PASS before full-app merge |
| File system | FAT-compatible volume through FatFs | `TEST.TXT` content/paging/errors reported PASS; scheduler assets pending |
| Audio | PA8 PWM through the R1 passive RC network and 5 V LM358 inverting stage to AC-coupled `AUX_OUT` | Firmware path is unchanged; revised analog response, clipping margin, and load drive require measurement |
| Relay output | `RLY_IN` to transistor/relay output stage | Current MP3 test enables it only for file playback; exact electrical polarity/output still requires measurement |
| Light output | `LGT_IN` to transistor/light output stage | GPIO and scheduled task implemented; active-level polarity unconfirmed |
| Persistent config store | Bit/byte-addressable settings (adopted `mydef.h` layout) | Internal-flash implementation host-simulation PASS; target test pending |
| Power | `+5V_IN`; AMS1117-3.3 produces `+3V3` | Schematic-confirmed; measurements TBD |
| Debug/programming | SWDIO, SWCLK, NRST, BOOT0 connectors | Hardware shown in schematic |
| Serial | PA9 TX and PA10 RX connector nets | USART1 TX configured at 115200; post-integration hardware check pending |

Development-kit and final-PCB compatibility must be verified per peripheral.
Matching MCU family names alone are not sufficient proof of package or pin
compatibility.

## Pin Requirements

| Pad | MCU Pin | Peripheral | Signal | Direction | Description | Active Level | Notes |
|---:|---|---|---|---|---|---|---|
| 1 | PA14 | Debug | SWCLK | Input | Programming/debug clock | N/A | Schematic net |
| 2 | PB6 | I2C1 | SCL | Open-drain output | DS1307 clock | N/A | Pull-up required |
| 3 | PB7 | I2C1 | SDA | Bidirectional open-drain | DS1307 data | N/A | Pull-up required |
| 4 | BOOT0 | Boot | BOOT0 | Input | Boot-mode selection | LOW normally | 10 kOhm pull-down shown |
| 5 | PB8 | GPIO | LCD_D7 | Output | LCD data bit 7 | TBD | 4-bit LCD interface |
| 8 | NRST | Reset | NRST | Input | MCU reset | LOW | Schematic reset network |
| 9 | PA0 | GPIO | SWT1 / MENU | Input | Keypad MENU | LOW | Internal pull-up |
| 10 | PA1 | GPIO | SWT2 / UP | Input | Keypad UP | LOW | Internal pull-up |
| 11 | PA2 | GPIO | SWT3 / DOWN | Input | Keypad DOWN | LOW | Internal pull-up |
| 12 | PA3 | GPIO | RLY_IN | Output | Relay-driver transistor control | HIGH during current tone/playback tests | Drives Q1 and pulls the external RLY_OUT path low; external amplifier/relay response must be verified |
| 13 | PA6 | SPI1 | SD_DO / MISO | Input | SD data to MCU | N/A | SPI Mode 0 |
| 14 | PB0 | GPIO | LGT_IN | Output | Light driver control | TBD | Firmware behavior TBD |
| 15 | PA4 | GPIO | SD_CS | Output | SD chip select | LOW | Explicit GPIO control |
| 16 | PA5 | SPI1 | SD_CLK / SCK | Output | SD serial clock | N/A | Slow during initialization |
| 17 | PA7 | SPI1 | SD_DI / MOSI | Output | SD data from MCU | N/A | SPI Mode 0 |
| 18 | PB10 | GPIO | LCD_D4 | Output | LCD data bit 4 | TBD | Verified mapping |
| 19 | PB11 | GPIO | LCD_D5 | Output | LCD data bit 5 | TBD | Verified mapping |
| 20 | PB12 | GPIO | LCD_D6 | Output | LCD data bit 6 | TBD | Verified mapping |
| 21 | PB13 | GPIO | LCD_RS | Output | LCD register select | TBD | Verified mapping |
| 22 | PB14 | GPIO | LCD_EN | Output | LCD enable | TBD | Verified mapping |
| 24 | PA8 | TIM1_CH1 | AUX | Output | Full-profile MP3 audio PWM | PWM | Not initialized by diagnostic; legacy module removed |
| 25 | PA9 | USART1_TX | TX | Output | Serial transmit | HIGH idle | Baud selected by caller; current value TBD |
| 26 | PA10 | USART1_RX | RX | Input | Serial receive net | HIGH idle | Debug helper currently configures TX only |
| 28 | PA13 | Debug | SWDIO | Bidirectional | Programming/debug data | N/A | Schematic net |

Power/ground pads and MCU pins without a confirmed project signal are omitted.

## Peripheral Requirements

### UART / Serial Monitoring

- Debug helper supports USART1 TX on PA9 and configures 8 data bits, no parity,
  one stop bit, and no hardware flow control.
- The active application calls `USART_Printf_Init(115200)`; terminal and
  post-integration hardware verification remain pending.
- PA10/RX is present on the schematic, but the current debug helper configures
  transmit-only mode.
- Serial monitoring was reported as working; the terminal program and exact
  test setup are **TBD**.

### SPI1 / SD Card

- SCK: PA5.
- MISO/SD_DO: PA6.
- MOSI/SD_DI: PA7.
- CS: PA4, active LOW.
- SPI mode: Mode 0.
- Initialization uses a slow SPI prescaler before switching speed.
- Project drivers provide initialization, block read, and block write.
- FatFs `diskio.c` adapts the SD block functions to the file system.
- `FF_FS_TINY` is `1`. Each `FIL` uses the mounted volume's shared 512-byte
  sector window rather than a private 512-byte cache, reducing the blocking
  MP3 player's stack frame. File-system access must remain serialized.
- Pre-integration hardware result: SD initialization, block-backed FatFs file
  reading, and `TEST.TXT` display were user-reported PASS. The latest cooperative
  application still requires boot and file-workflow revalidation.
- MP3 playback depends on successful SD initialization and FatFs file access.

#### LCD Text-File Pager

- `Firmware/User/sd_text_viewer.c/.h` opens a caller-selected root text file
  through the `storage.c`-owned FatFs mount.
- In `HAPPYBELL_PROFILE_DIAGNOSTIC`, open `TEST.TXT` automatically after SD
  mounting; MENU exits and can then reopen the viewer from the prompt.
- `HAPPYBELL_PROFILE_MP3_TEST` is a focused audio-test image: it
  scans and auto-plays the first `001/*.enc`; UP replays ENC and DOWN plays root
  `mp3_song.mp3`. The text pager is available only in the standalone diagnostic,
  not in the focused V10 key loop or reference-style full profile.
- The LCD shows two 16-character rows. UP advances, DOWN returns to the
  previous page, and MENU exits.
- CR/LF formatting, tab-to-space conversion, printable-ASCII replacement,
  empty/missing/open/read/mount messages, paging, and exit behavior are
  implemented.
- Content display, paging, and viewer errors were user-reported PASS on the
  pre-integration firmware. The standalone diagnostic entry path requires
  hardware revalidation after integration.

### GPIO / Keypad

- PA0 = MENU, PA1 = UP, PA2 = DOWN.
- Inputs use internal pull-ups and are active LOW.
- Existing code uses edge-style key events; full-profile `KEY_WaitKey()` keeps
  cooperative light/audio services running during its 100 ms polling delay.
- MENU/UP/DOWN hardware operation was user-reported PASS before the full
  application merge. The new idle/menu sequences require revalidation.

### RTC

- Device: DS1307.
- I2C address: 0x68.
- I2C1 pins: PB6 SCL and PB7 SDA.
- Required behavior: initialize, detect, read, and write time/date.
- Time must continue across MCU resets and power cycles when the intended RTC
  backup supply is fitted and working.
- Detection, read/write, and backup persistence were user-reported PASS before
  the full application merge. The latest boot and scheduling flow require
  hardware revalidation.

### LCD

- Parallel 4-bit interface.
- PB10 = D4, PB11 = D5, PB12 = D6, PB8 = D7.
- PB13 = RS and PB14 = EN.
- G8R6 LCD operation was reported as working.
- Exact controller/model, geometry, and electrical levels are **TBD**.

### Legacy PWM Test (removed)

- Removed on owner instruction (2026-08-24): PA8/TIM1_CH1 now belongs
  exclusively to MP3 audio. `pwm.c/.h` and their parked copy are absent from
  the Git-ready repository and build.
- Standalone approximately 100 kHz operation and a later integrated PWM test
  were user-reported PASS before removal; the exact later measurement was not
  supplied. The earlier approximately 9 kHz result remains historical.
- These results do not validate the current nominal 375 kHz MP3/DMA carrier.

### MP3 Decode and Audio PWM

- Public wrapper: `CH32_MP3_PlayFile(path, volume)`.
- Input source: FatFs file on the SPI microSD card.
- Supported format on CH32V203G8R6:
  - MPEG-1 Layer III, native mono.
  - 32 kHz sample rate.
  - Indexed 64 or 128 kbps CBR/VBR frames used by the deployed corpus.
- Explicitly unsupported:
  - Stereo, joint-stereo, and dual-channel streams.
  - MPEG-2, MPEG-2.5, free-bitrate, and bitrate indices other than 5 or 9.
  - Sample rates other than 32 kHz.
  - Encoded frames larger than the 768-byte input buffer. The wrapper returns
    `CH32_MP3_FRAME_TOO_LARGE` instead of looping on an unfillable frame.
- Decoder allocation: one static Helix decoder instance; no decoder heap use.
- A file that reaches the end without one successfully decoded frame returns
  an error; an encrypted file with a valid preamble but no decodable frame is
  reported as corrupt rather than successful playback.
- The wrapper exposes the verified stream parameters: sample rate, bitrate,
  decoded bits per sample, output channel count, MPEG version/layer, compressed
  frame bytes, samples per frame, and encrypted/plain source type.
- Sync-word candidates with joint/dual mode bits are skipped because compressed
  payload and trailing metadata can contain false sync patterns. Stereo is
  reported as unsupported only when no valid frame was decoded.
- For the validated `mp3_song.mp3`, full-buffer decoder underflow is treated as
  a false oversized-header candidate and scanning advances by one byte; its
  actual 288-byte frames cannot exceed the 768-byte input buffer.
- Input buffer: 768 bytes, reduced to make room for two encoded side-info
  channels while retaining one-channel PCM output and the reserved stack.
- Plain and encrypted MP3 files with an ID3v2 header are seeked to the declared
  tag end before frame scanning. Synchsafe size bytes and the declared tag end
  are checked before the first MPEG header is validated.
- Helix validates mono input before decoding into its one-channel PCM state.
- The parsed side-info `part23Length` must be copied into the decoder state
  consumed by Huffman processing; this local Helix correction is required.
- Every mono synthesis-history reference must use `VBUF_BLOCK_STRIDE` and
  `VBUF_HALF_LENGTH`. The CH32 single-channel layout uses 32 words per block,
  while the upstream/two-channel layout uses 64. A remaining hard-coded
  `vbuf + 64` in `PolyphaseMono()` shifted its main convolution by one compact
  block and produced corrupted PCM without stopping DMA; it is now corrected.
- Fixed-point `CLZ()` must shift an unsigned working value. Shifting a positive
  signed value into the sign bit is undefined C behavior. The RISC-V `MADD64`
  product assembly must likewise avoid a signed left shift of a negative high
  word.
- Stereo-only dequantization and subband paths must be excluded at compile time
  when `MAX_NCHAN=1`; unreachable channel-1 indexing is still outside the
  compact arrays and must not remain in the mono translation units.
- PCM buffering: two contiguous slots of 576 signed 16-bit samples (reused in
  place as the DMA source once volume-scaled; no separate DMA buffer). A mono
  MPEG-1 frame produces 1,152
  samples across both slots. `MP3DecodeWithGranuleHook()` hands each 576-sample
  row to the wrapper immediately after synthesis. The next frame begins when
  row 0 is free; the hook waits for row 1 only immediately before Helix writes
  it. This removes the previous mandatory full-frame decode gap without adding
  static RAM. A real underflow remains possible if SD plus granule decoding
  takes longer than the preceding 18 ms row at 32 kHz and must be measured.
- Audio carrier: TIM1_CH1 on PA8, ARR 255, nominally 375 kHz when the timer
  clock is 96 MHz. This PWM output feeds the board's analog low-pass filter
  stage between `AUX` and `AUX_OUT`, which recovers the analog audio signal.
- Clock source: the G8R6 schematic has no external HSE crystal or oscillator
  connection. Firmware must use `SYSCLK_FREQ_96MHz_HSI`, which enables WCH's
  internal-HSI x12 PLL path. Selecting HSE can time out at startup and leave
  the MCU near 8 MHz; with ARR 255 that produces only about 31.25 kHz PWM and
  leaves insufficient CPU throughput for continuous Helix decoding.
- The MP3-test must halt on `CLOCK ERROR / NOT 96MHZ` unless
  `SystemCoreClockUpdate()` reports exactly 96 MHz before any decode.
- `Audio_Init()` additionally requires APB1 `/2`; PCLK1 is 48 MHz and WCH's
  automatic timer x2 path gives TIM2 a 96 MHz counter clock. The validated
  32 kHz ENC files use exactly 3,000 timer ticks per sample. No I2S peripheral
  is used, so bit clock (BCLK) and master clock (MCLK) are not applicable.
- Sample rates that divide 96 MHz use one fixed TIM2 period. For 44.1, 22.05,
  and 11.025 kHz, a bounded phase accumulator selects adjacent integer periods.
  At 44.1 kHz, each 147-sample cycle contains 19 periods of 2,176 ticks and
  128 periods of 2,177 ticks, totaling 320,000 ticks and exactly 44,100 samples
  per second on average. The ISR changes only TIM2 ARR; DMA1 Channel2 still
  writes every PWM duty sample.
- The R1 schematic is authoritative. From PA8 `AUX` toward the amplifier it
  shows 4.7 kOhm series/10 nF shunt, then 10 kOhm series/10 nF shunt, then a
  1 uF coupling capacitor and 1 kOhm into LM358 channel-A inverting input pin
  2. The separate passive sections have isolated RC corner estimates of about
  3.39 kHz and 1.59 kHz; their loaded composite response must be measured.
- LM358 pin 3 is biased at nominally 2.5 V using 10 kOhm to `+5V_IN` and 10
  kOhm to ground. Pin 8 is `+5V_IN`, pin 4 is ground, and 100 nF is shown from
  the 5 V supply to ground. A 10 kOhm pin-1-to-pin-2 feedback resistor with the
  1 kOhm input resistor gives inverting signal gain `-Rf/Rin = -10` and noise
  gain 11. Pin 1 is coupled to `AUX_OUT` through 10 uF.
- LM358 channel B pins 5, 6, and 7 are unconnected in R1. This does not require
  firmware changes, but floating inputs can increase noise or oscillation risk.
  A future schematic revision should bias pin 5 at the 2.5 V reference and tie
  pin 7 to pin 6 for a stable follower, subject to board-layout review.
- LM358 is not rail-to-rail and is not a speaker power amplifier. Validate pin
  1 bias, peak swing, load current, distortion, stability, and temperature on
  the populated board. The 10 uF output capacitor is suitable for a high-
  impedance line input; its high-pass corner is load-dependent and direct
  4/8-ohm speaker drive is not an accepted configuration.
- Output scaling: caller volume is 0-255 and remains 255. The ESP reference
  exposes no programmable amplifier gain and maps signed-16 PCM directly to
  the full 8-bit DAC range. On CH32, both plain and encrypted files use one
  block-peak-controlled Q8 gain: 254/256x through 32x, target peak 32,512
  (99.2% of signed-16 full scale), +1x maximum rise per 576-sample block,
  with each rise interpolated across the block; immediate reduction before a
  louder block; and no gain tracking below a decoded peak of 512. Playback
  starts at 32x so short announcements do not end during a slow gain ramp; the
  complete first block is peak-scanned and its gain reduced before DMA if
  necessary. The result is volume-scaled and finally saturated before
  adding the 128 PWM midpoint; at volume 255 the modeled duty range is 1..254.
  This is the maximum digitally safe range, not proof that the unknown
  downstream amplifier/speaker or gain-10 LM358 stage cannot clip. Scope,
  listening, current, and temperature checks remain required.
- Before SD playback, the focused image must verify one signed
  `MULSHIFT32()` result and one signed `MADD64()` result against fixed 64-bit
  constants. Display `ARITH FAIL` and stop if either RISC-V primitive is wrong.
- RAW PA8 and DMA sine-wave diagnostic generators are removed from production
  source and startup. Reintroducing a test generator requires a separate,
  explicitly authorized diagnostic change and must not enter the normal
  playback path.
- Before each audio start, reset TIM1, TIM2, and DMA1 Channel2 and initialize
  the entire `TIM_OCInitTypeDef` with `TIM_OCStructInit()`. TIM1's WCH advanced
  timer setup consumes complementary-output and idle-state fields even though
  only PA8/TIM1_CH1 is used.
- The focused MP3 profile enables PA3/RLY_IN only around file playback so an
  externally connected amplifier/relay can be energized.
  The schematic shows Q1 as a low-side driver and does not show the external
  relay, amplifier, or speaker itself.
- The R1 schematic contains no speaker power-amplifier stage after
  `AUX_OUT`. Audible testing therefore requires a powered speaker or suitable
  external amplifier; a passive speaker connected directly to `AUX_OUT` is
  not an acceptance setup. The owner confirmed that the passive speaker is
  presently connected directly. Use only a separately qualified downstream
  power stage or powered speaker, and retain `AUX_OUT` as its line-level input.
- The manufactured R1 PCB already contains the LM358 and passive network shown
  above. No firmware gain or GPIO change is implied by this analog revision.
- Sample timing: TIM2 free-runs at the decoded sample rate and its update
  event drives PWM duty via **DMA**, not a per-sample duty-write interrupt.
  DMA1 Channel2 is configured memory-to-peripheral, half-word, from a
  decoded PCM slot (or a small circular silence buffer when nothing is
  ready) directly into `TIM1->CH1CVR`. Only the DMA Transfer-Complete
  interrupt (`DMA1_Channel2_IRQHandler`) runs on the CPU, once per
  576-sample slot (or once per silence-buffer wrap), to hand off to the
  next ready slot.
  - The TIM2 update ISR remains disabled for exact-divider rates such as the
    validated 32 kHz ENC set. It is enabled only for fractional-divider rates
    and performs one bounded ARR phase-accumulator update; it never touches
    PCM, FatFs, Helix, or TIM1 duty data.
  - WCH's CH32FV2x/V3x reference-manual DMA request table confirms TIM2_UP on
    DMA1 Channel2. That channel also carries SPI1_RX and TIM1_CH1 requests.
    The current SD driver is polled; do not add SPI RX DMA on the same channel
    without redesigning ownership.
- Volume and bounded all-file gain are applied in bulk to each decoded
  576-sample granule in `Audio_PrepareSlot()`, immediately after synthesis and
  before the row is marked ready for DMA. Both plain and encrypted paths use
  the Q8 envelope above, add +128 after final saturation, and remain in the
  modeled 1..254 PWM-duty range at volume 255. DMA
  streams the prepared row with no CPU involvement per sample.
- The DMA completion interrupt must remain short (slot handoff only) and
  must not perform file I/O or MP3 decoding, same rule as the previous ISR.
- `CH32_MP3_GetUnderflowCount()` must be checked during performance testing.
  The DMA implementation preserves the same underflow semantics as before:
  counted only when real playback runs dry into the silence buffer, not on
  every normal slot-to-slot handoff.
- Playback is blocking at the application API level. The MP3-test profile uses
  the bare-metal millisecond delay while waiting for DMA slots; the full
  profile uses the cooperative audio wait hook to service light and configured
  key/deadline polling at the same 1 ms yield points.
- Before each file/read/sync/decode/audio-init/PCM/DMA phase, the wrapper
  records a one-byte fault stage. `MP3Decode()` further records header,
  side-info, main-data, scale-factor, Huffman, dequantization, IMDCT, and
  subband checkpoints. A HardFault copies the active stage to an 8-byte
  `.noinit` record before `NVIC_SystemReset()`; the next boot displays
  `HARDFAULT` plus the retained stage for five seconds.

### Local Encrypted Audio (`.enc`)

- Exact format, confirmed from `esp_bell_idf` `decryptBuffer()` and the mounted
  files: the complete original MP3 byte stream, starting at file byte zero, is
  XORed with the repeating four-byte ASCII key `BK26`. There is no IV, salt,
  nonce, padding, authentication tag, encryption header, or wrapper metadata.
  Any ID3 metadata remains inside the XOR stream. This is reversible
  obfuscation, not cryptographic encryption.
- Decryption uses `(absolute_file_offset + byte_index) & 3` for key selection.
  It does not depend on refill history, so FatFs short reads, retained bytes,
  ID3 seeking, and buffer compaction cannot shift the XOR phase.
- Before decoding, the wrapper decrypts and validates the preamble, checks an
  ID3v2 synchsafe size against file length, seeks to the audio offset with the
  correct key phase, and requires a valid MPEG Layer III header.
- Full-profile file selection prefers `<name>.mp3`; if absent, it tries
  `<name>.enc`. V9's standalone test scans folder `001` once, selects the
  alphabetically first `.enc` FAT short filename, and plays it automatically.
  UP replays that encrypted path; DOWN plays root `mp3_song.mp3` for A/B.
- `.enc` extension detection is ASCII case-insensitive so FAT short names
  returned as `.ENC` are decrypted with the same repeating key.
- Must not change the supported-format restrictions in "MP3 Decode and Audio
  PWM" above (mono MPEG-1/2 Layer III, 16-48 kHz) — decryption happens
  before decode and is independent of them.

- V9 error results distinguish open failure, SD read failure, invalid ENC
  preamble/key, unsupported decrypted format, corrupt stream/format change,
  oversized frame, unsupported rate/stereo, audio-clock configuration, and DMA
  timeout. The LCD maps each to a bounded visible message.
- Mounted folder `001` verification (2026-08-25): 25 `.enc` files, all with a
  45-byte ID3v2.4 tag containing `TSSE=Lavf58.26.100`, followed by contiguous
  mono MPEG-1 Layer III at 32 kHz/128 kbps. Decoded PCM is signed 16-bit mono;
  each compressed frame is 576 bytes and produces 1,152 PCM samples. All
  21,843 frames across the 25 files end exactly at their respective EOF.
- `001/0001.enc`: 428,013 bytes; encrypted SHA-256
  `C261CFF4085067513016C3FF74E49773B22CCD17FE11BE51FC20FD52D13FB516`;
  decrypted MP3 SHA-256
  `A03F88121ACC627FDBC2219BC838D59C7F05390F0AAD3336DCE9770F3868BC66`;
  743 frames; XOR-twice byte-for-byte round-trip PASS.

- `tools/encode_mp3.ps1` creates the required XOR file without overwriting an
  existing output unless `-Force` is supplied:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\encode_mp3.ps1 `
  -InputPath "C:\path\to\00001.mp3" `
  -OutputPath "C:\path\to\00001.enc"
```

### EEPROM-Style Configuration Store

- Adopted from `esp_bell_idf`'s `EEPROM_NVS` wrapper and `mydef.h` memory
  map: bits 0-12 (0-100 range checked by `readBool`/`writeBool`) are
  individually bit-addressable feature/playlist-entry enable flags; bytes
  13-49 hold byte-addressable settings (amp state, amp on/off enable,
  night-play enable, bell start/end hour, bell/starting-music track index,
  light on/off hour/minute, and a 2-byte trigger-window resume id/position
  pair starting at offset 26/28);
  offset 50 and above hold one 2-byte "last played index" counter per
  rotating group (`g` token), addressed as `50 + group_number * 2`.
- Reference offsets 15 (automatic network time) and 19 (Bluetooth enable) are
  intentionally left unused. The CH32 source defines no radio macros, menu
  entries, state, initialization, callback, task, or dependency for them;
  later local offsets retain their reference numeric positions.
- **Implemented** (`Firmware/User/eeprom.c`): CH32V203 internal flash
  emulation using eight 256-byte fast pages at 0x0800F800-0x0800FFFF,
  written round-robin (one erase+program per commit, wear spread x8),
  each record checksummed with a monotonically-increasing sequence number;
  the newest valid record wins at boot and a corrupted newest record falls
  back to the previous one. `FLASH_ErasePage_Fast`/`FLASH_ProgramPage_Fast`
  are used because the standard `FLASH_ErasePage` on this part erases 4 KB.
  The linker FLASH region is shrunk to 62 KB to keep these pages free.
- Store payload is 240 bytes (`EEPROM_SIZE`), which limits rotating-group
  ids to 1..94 (`MEM_GROUP_AT + id*2 < 240`). The esp_bell_idf reference
  allowed larger ids with its 1024-byte NVS blob; raise `EEPROM_SIZE` only
  with a redesigned record layout (a 256-byte page holds header + 240 + 8
  spare).
- A factory-fresh store reads as all 0xFF, so every enable bit starts as 1
  (all playlist entries enabled) -- same behavior as the reference's fresh
  NVS blob.
- Logic was verified with a host-side simulation (persistence across
  simulated reboot, rotation through all 8 slots, corrupt-newest-record
  fallback). On-target verification is pending, including that a config
  write during playback does not disturb audio DMA/decode -- flash ops
  stall the CPU; measure, don't assume.
- Exact guaranteed flash endurance for this part is **TBD** from the
  datasheet (typically 10k+ cycles/page; the x8 rotation multiplies that).
- Every write in the `esp_bell_idf` reference is followed by an immediate
  commit (`nvs_commit`); the CH32 store must preserve durable-write-on-write
  semantics unless a measured, documented reason (e.g. flash wear) changes
  this.

### Scheduled Playlist Engine

- Input: `playlist.txt`, one schedule line per entry, read directly from the
  SD card (no SPIFFS-equivalent staging step on this target).
- Line types:
  - `1YYMMDDHHMMDD...`: exact date.
  - `2YYMMDDYYMMDDWWHHMMDD...`: date range + weekday bit-mask (bit 0 =
    Sunday .. bit 6 = Saturday), both fields hex-encoded as in the source.
  - `3WWHHMMDD...`: weekday bit-mask only.
- Header fields after the type byte: `HHMM` start time, `DD` duration in hex
  minutes (`00` = run once, no loop).
- Supported local sequence tokens:
  - `fNNNN`: direct 4-digit file path.
  - `gNNN`: rotating group; 3-digit group id, increments each play, wraps to
    1; last-played index persisted in the EEPROM-style store.
  - `m h w d M W D`: minute / hour / weekday / date / month / week-of-year /
    day-of-year folder-index tokens, each also an EEPROM enable-bit address.
  - `s`: starting-music track index from the config store.
  - `b`: bell sound from the config store, repeated `get12Hour()` times.
  - `F`: festival audio id from the day's calculated Panchang.
  - `S`: special-day audio id from the day's calculated Panchang.
  - `p`: generated Panchang announcement sequence, ordered by the
    `panchangOrder` string from `settings.txt` line 6 (each letter selects
    one Panchang element/time-range announcement, exactly as in
    `bell.cpp: playSequence()`).
- Matching against RTC time/date/weekday, sequence playback with duration
  cutoff, group-rotation index persistence, and resume of a trigger window
  interrupted by a reboot (matched by a date+start-time derived id) must all
  be preserved.
- `holiday.txt` (`YYMMDD` per line) skip check must run before playlist
  evaluation on the day it matches.
- `menu.txt` (`<eeprom-bit>:<label>` per line) drives the per-entry
  enable/disable setup screen.

### Panchang Astronomy Engine

- Uses an application-specific single-precision model for the practical
  2000-2050 operating range. It retains the Sun/Moon positions, tithi,
  nakshatra, yoga, primary karana, signs, sunrise/sunset, moonrise/moonset,
  calendar fields, timing windows, and festival/special-day results consumed
  by the application.
- The corrected compact model uses the golden implementation's two-pass NOAA
  solar event sequence, the leading 30 ELP longitude and 20 latitude terms
  quantized to millidegrees, its 2000-2050 Delta-T polynomial, split binary32
  phase accumulation, lightweight trigonometric kernels, and bounded event
  search/refinement. Unconsumed secondary karana,
  yoga-end, Abhijit, second durmuhurta, and adhika-masa storage/calculations
  were removed.
- Against the actual ESP C VSOP/ELP implementation, 2,662 weekly sunrise
  samples had maximum Sun/Moon longitude differences of 0.012/0.019 degrees.
  Tithi, nakshatra, and yoga classification mismatches were 1, 0, and 4,
  confined to category boundaries. Across 120 date/location cases,
  sunrise/sunset differed by at most 0.024 minute and moonrise/moonset by
  0.085/0.089 minute, with no event-presence mismatch.
- Across 612 monthly transition cases, maximum tithi/nakshatra end-time errors
  were 1.968/1.946 minutes. Five-minute spoken rounding differed in 57/41
  cases; those boundary-only differences and on-target execution time remain
  release-validation items.
- Festival matching must use the Tamil solar month/day regardless of the
  configured calendar mode, so solar festivals stay correct in lunar mode
  (matches the `esp_bell_idf` behavior).

## Software Requirements

### Architecture

- `Firmware/User/happybell_build.h` selects one of three profiles:
  `HAPPYBELL_PROFILE_DIAGNOSTIC` for SD/LCD only,
  `HAPPYBELL_PROFILE_MP3_TEST` for blocking foreground Helix/DMA/PWM testing,
  and default `HAPPYBELL_PROFILE_FULL` for the cooperative Astro/MP3
  application. The full profile fits the CH32V203G8R6.
- The managed project contains no `Firmware/FreeRTOS` tree or RTOS include
  paths. No profile may link task, queue, semaphore, heap, tick, or
  context-switch state.
- Keep MCU device support under `Firmware/Core`, `Firmware/Peripheral`, and
  `Firmware/Startup`.
- Keep board/application drivers under `Firmware/User`.
- Keep FatFs under `Firmware/FatFs`.
- Keep the third-party decoder and CH32 wrapper under `Firmware/LibHelixMP3`.
- Do not re-add the removed FreeRTOS source or `FreeRTOSConfig.h` without a new
  measured requirement.
- Application code must call the CH32 wrapper instead of Helix internals where
  practical.

### Cooperative Runtime Architecture

- TIM3 runs at 1 kHz and provides `millis()` without using SysTick. Its IRQ is
  lower priority than TIM2/DMA audio interrupts and only clears update state
  and increments one volatile counter.
- The foreground runs bell/menu at the original 20 ms cadence, schedule/display
  evaluation every 600 ms, light/RTC once per second, and audio only on demand.
- Blocking Helix work retains the former high-priority-task behavior. Its
  existing 1 ms DMA-slot waits call the cooperative hook, which services the
  light interval and the active 10 ms or 100 ms key/deadline poll.
- DMA/TIM ISRs share only the existing `volatile` playback state. No ISR may
  call FatFs, Helix, RTC, menu, or logging code.

### Default MP3-Test Initialization Sequence

1. Update the system clock value and initialize delay support.
2. Initialize the LCD and consume any retained HardFault record. Show
   `HARDFAULT` plus `MP3 FILE`, `MP3 READ`, `MP3 SYNC`, `MP3 DECODE`,
   `AUDIO INIT`, `PCM PREPARE`, or `DMA OUTPUT` for five seconds when present.
3. Initialize the keypad and relay/light GPIO; keep PA3 LOW outside playback.
4. Run the signed Helix arithmetic self-test and halt on `ARITH FAIL`.
5. Show `MOUNTING SD`, initialize SD/SPI, and mount FatFs; show a blocking LCD
   error if unavailable.
6. Scan folder `001`, select the alphabetically first case-insensitive `.enc`
   short filename, show `PLAYING:` plus that path, enable PA3, and play it.
7. Report the decoder result, decoded PCM peak,
   completed DMA-slot count, and DMA underflows. Abort a stalled slot wait
   after 500 ms with `DMA TIMEOUT`.
8. Show `UP:ENC DOWN:MP3`; UP replays ENC and DOWN plays root
   `mp3_song.mp3`; both use volume 255 and the same bounded gain path.

### Retained Full-Profile Initialization Sequence

1. Configure the priority grouping, update the clock value, and require the
   internal-HSI PLL result to report 96 MHz before audio initialization.
2. Initialize delay support and USART1 TX at 115200.
3. Initialize LCD, load the reference setup cursor into HD44780 CGRAM slot 2,
   then initialize keypad, RTC, relay, and light GPIO.
4. Detect/read the RTC; show a blocking LCD error if unavailable.
5. Initialize SD/SPI and mount FatFs; show a blocking LCD error if unavailable.
6. Initialize the flash store/settings and MP3 player wrapper.
7. Start TIM3 and enter the cooperative bell/light/audio foreground loop.
8. When playback begins, configure TIM1 PWM, TIM2 sample events, and DMA only
   after decoded PCM is ready; stop and close resources after playback/error.

The full-profile bell service retains the reference 20 ms idle cadence, 600 ms
schedule/display interval, 256-byte playlist/sequence line capacity, and relay
settling sequence. Date/time edit screens use the same CGRAM slot-2 cursor
glyph as the reference.

### Error Handling

- Return distinct file, decoder, unsupported-channel, unsupported-sample-rate,
  and frame-too-large results.
- Never decode two-channel PCM into mono-sized arrays. The deployed-profile
  parser rejects all stereo modes before decoder state is used.
- Preserve SD/RTC timeout and diagnostic failures rather than hiding them.
- Do not mark a hardware function PASS until it has been measured or observed.

### Interrupt and Shared-State Requirements

- TIM2 produces DMA request events; it does not run a per-sample application
  ISR in the current audio design.
- DMA1 Channel2 transfer-complete work is limited to prepared-slot handoff.
- File reading and MP3 decoding remain in foreground context.
- Data shared between DMA/ISR and foreground context remains `volatile` and must be
  reviewed for race conditions whenever buffer logic changes.
- All high-level DS1307 access is serialized by the single foreground. The
  light service obtains its own fresh time once per second rather than relying
  on the bell display/scheduler snapshot.

## Libraries Used

| Library / Module | Purpose | Version / Source | Used By | Notes |
|---|---|---|---|---|
| CH32V20x device support | MCU registers and peripheral drivers | Project SDK copy; version TBD | All firmware | WCH RISC-V target |
| `debug.c/.h` | Delay and serial debug helpers | Project-local | Application/drivers | USART1 helper uses PA9 TX |
| `lcd.c/.h` | LCD driver | Project-local | Application | G8R6 mapping reported working |
| `keypad.c/.h` | Button input driver | Project-local | Application | PA0/PA1/PA2; pre-integration test PASS |
| `rtc.c/.h` | DS1307 driver | Project-local | Application | Detection/read/write/persistence pre-integration PASS |
| `sd.c/.h`, `sd_spi.c/.h` | SD SPI/block driver | Project-local | FatFs/application | Initialization/file-read pre-integration PASS |
| FatFs | FAT file-system implementation | Repository copy; exact upstream version TBD | SD files and MP3 wrapper | Configured read-only |
| `sd_text_viewer.c/.h` | Blocking 16x2 text-file paging/error diagnostic | Project-local | Standalone `TEST.TXT` diagnostic | Pre-integration `TEST.TXT` PASS; post-parity hardware revalidation pending |
| `pwm.c/.h` | Removed legacy fixed-frequency PWM | Removed from repository | Historical test only | Passing result retained; not in build |
| Helix fixed-point MP3 decoder | Layer III frame decode | Ported from `C:\OfficeWorks\esp_bell_idf\components\libhelix-mp3`; upstream version TBD | CH32 MP3 wrapper | Third-party licenses retained |
| `ch32_mp3_player.c/.h` | FatFs, validated plain/ENC input, Helix decoding, bounded all-file peak normalization, exact-rate DMA/PWM, metadata, and errors | Project-local wrapper | MP3 test/full application | Maximum-safe digital gain arithmetic PASS; prior encrypted hardware playback PASS; maximum-volume listening and full-profile hardware regression pending |
| `tools/encode_mp3.ps1` | Create `BK26` XOR `.enc` assets on Windows | Project-local PowerShell utility | Host-side asset preparation | Pure PowerShell/.NET; round-trip SHA-256 test PASS |
| `tools/helix_host_decode.c` | Decode an MP3 to raw signed-16 PCM for reference comparison | Project-local host test | Helix port validation | Unhooked and granule-hook modes validated against the untouched reference |
| Tamil Panchangam astronomy engine | Compact 2000-2050 Sun/moon position, calendar categories, and rise/set model | Application-specific rewrite validated against `esp_bell_idf` reference | Scheduling engine (`F`/`S`/`p` tokens) | Host category and sunrise/sunset comparison complete; moonrise/on-target timing pending |
| Scheduling / menu / EEPROM-store application layer | SD text-file parsing, front-panel setup, config store, relay/light logic | Ported from `esp_bell_idf` application sources | Application | Source implemented and linked; hardware regression pending |
| `relay_light.c/.h` | RLY_IN/LGT_IN GPIO init and set functions | Project-local | Application | GPIO and calling logic implemented; polarity test pending |

### Documented Helix Source Modifications

- `MAX_NCHAN` is configured as one through `HELIX_CH32_MAX_CHANNELS`.
- Frame parsing rejects channel counts larger than the configured decoder.
- Heap allocation was replaced by one statically allocated decoder state.
- The mono-only port uses a compact synthesis-history layout. With
  `MAX_NCHAN=1`, `FDCT32()` and `PolyphaseMono()` use a 32-integer block stride
  and a 544-integer mirrored-half offset. Do not restore the upstream
  stereo-interleaved 64/1,088 addressing without also restoring the much
  larger two-channel allocation.
- The RISC-V `MADD64` path sign-extends the signed high multiplication word.
- `CLZ()` shifts an unsigned working value, and the RISC-V `MADD64` assembly
  avoids signed left-shift overflow.
- Mono builds compile out stereo-only dequantization/subband channel-1 paths.
- Header, bitrate, sample-rate, scalefactor-band, and side-info paths are
  specialized for deployed MPEG-1 Layer III mono 32-kHz 64/128-kbps assets.
- `MP3DecodeWithGranuleHook()` optionally exposes before/after synthesis events
  while the original `MP3Decode()` remains reference compatible.
- Helix license files remain in `Firmware/LibHelixMP3`.
- Project include paths were added to both checked-in MounRiver `.wvproj`
  configurations.

## Testing Requirements

| Test | Objective / Setup | Expected Result | Actual Result | Status |
|---|---|---|---|---|
| Serial monitoring | Connect configured serial interface | Reliable debug output | Reported working; details TBD | PASS |
| LCD on G8R6 | Initialize verified pins and display text | Correct readable text | Reported working | PASS |
| Keypad | Press MENU/UP/DOWN | One correct event per press | User-reported pre-integration success | PASS |
| RTC detection | Probe DS1307 at 0x68 | Device acknowledges | User-reported pre-integration success | PASS |
| RTC read/write | Set and read known time | Correct advancing time | User-reported pre-integration success | PASS |
| RTC persistence | Power-cycle with backup supply | Time continues | User-reported pre-integration success; setup/duration TBD | PASS |
| SD initialization | Initialize card in SPI Mode 0 | Card becomes ready | User-reported pre-integration success | PASS |
| SD block read | Read known sector | Data matches source | Block path exercised by successful FatFs file read | PASS |
| FatFs file read | Open/read known file | Exact contents returned | `TEST.TXT` content displayed on LCD | PASS |
| `TEST.TXT` first page | Run the SD/LCD diagnostic profile with the file in the SD root | First formatted page appears | Viewer behavior passed before merge; diagnostic profile not retested | PENDING |
| `TEST.TXT` paging | Use file longer than two LCD rows | UP/DOWN page and MENU exits | User-reported pre-integration success | PASS |
| `TEST.TXT` errors | Exercise viewer failure cases | Correct LCD status/error messages | User-reported pre-integration success; exact cases TBD | PASS |
| Legacy PWM standalone | Measure PA8 | Stable 100 kHz | Approximately 100 kHz reported | PASS |
| Legacy PWM integrated | Initialize LCD/keypad and measure PA8 | Intended PWM output remains stable | Latest test user-reported PASS; exact measurement TBD; module later removed | PASS |
| Project configuration | Parse G8R6 `.wvproj`/`.cproject` and inspect paths | Valid JSON/XML and required sources; no obsolete target or FreeRTOS inputs | G8R6 files select NoneOS; C8T, D8/D8W, and FreeRTOS files are absent; no RTOS include path remains | PASS |
| Reference-flow audit | Map `C:\OfficeWorks\esp_bell_idf` boot, menu, scheduler, storage, RTC, Panchang, persistence, and local-audio paths | Every applicable non-radio behavior is implemented or has a documented platform adaptation | 2026-08-26 source/config audit completed; mapping and adaptations documented above | PASS |
| Radio dependency exclusion | Search CH32 application and build inputs for radio drivers, credentials, callbacks, states, tasks, and dependencies | No application/build reference remains | 2026-08-26 application/build search returned no match | PASS |
| Integrated source compile | Compile all application/library sources and required startup assembly without FreeRTOS | No compile/assemble errors | All three profiles compiled with WCH GCC; only pre-existing unused-parameter/sign-compare warnings appear under stricter `-Wall -Wextra` | PASS |
| G8R6 diagnostic link | Link SD/LCD-only profile with WCH GCC | Fits configured FLASH/RAM | Refreshed 2026-08-26: 8,616 B loadable flash and 5,292 B RAM | PASS |
| Original `mp3_song.mp3` structure | Scan mounted stereo source before conversion | Source is structurally valid and retained | Preserved as `mp3_orig.mp3`; 2,880,985 B; SHA-256 `0CA7024D1AA17D4D6AD071C3B89FF82D0047F542D07EB3579CC07A9489E16A92` | PASS |
| Current `mp3_song.mp3` structure | Decode and scan converted SD-root asset | Native mono and every frame fits input buffer | 2,021,184 B; SHA-256 `233CACC745347BF021ADAF5D397747D7D04535806A1D9D03C37C6CC6A0AE3C8B`; 7,018 contiguous mono MPEG-1 32 kHz/64 kbps frames, 288 B each; host decoded peak 32,768 | PASS |
| Helix PCM reference comparison | Decode the exact current asset with the untouched reference, compact CH32 port, and enabled no-op granule hook | Same output count and every signed-16 sample equal | All paths produced 8,084,736 samples; PCM SHA-256 `ADC72D6CD76A89317DAD3BD107764806C3F5808BF1DB715CA1D9E2167AA36F4A`; zero differing samples; correlation 1.0; hook before/after counts 14,036 each | PASS |
| Direct `mp3_song.mp3` playback | Play the plain A/B path | LCD reports playback; nonzero song `P:`/`D:`; clean audible amplified `AUX_OUT` | V8 user report: physical-speaker audio plays at normal speed with no distortion. Final song counters and underflow count were not reported. | IN PROGRESS |
| V8 physical audio quality | Listen to `mp3_song.mp3` through the device speaker | Normal speed/pitch and no audible distortion | User reported normal playback speed and no distortion | PASS |
| Helix RISC-V arithmetic self-test | Boot V6 image | LCD displays `ARITH PASS`; a mismatch displays `ARITH FAIL` and halts before audio | LCD displayed `ARITH PASS` | PASS |
| Removed tone diagnostics | Inspect source and ELF | No RAW PA8 or DMA sine functions, waveform data, APIs, screens, or calls remain | V10 source search and ELF symbol audit found no test-tone implementation | PASS |
| Maximum-safe audio gain arithmetic | Exhaustively model block peaks, gain bounds, signed-16 endpoints, volume scaling, and PWM conversion | Gain remains 254/256x-32x; no arithmetic overflow/wrap; duty avoids both PWM rails | Every peak 1..32,768 and signed endpoint modeled; duty range 1..254, zero out-of-range cases | PASS |
| Maximum-volume plain/ENC quality | Play representative plain and encrypted MP3 files | Match ESP-BELL perceived loudness without clipping, distortion, pumping, or unwanted noise | User reports improved output after the 32x envelope, but it remains noticeably below ESP-BELL; digital peaks already differ by only 0.07 dB | FAIL (LEVEL) |
| MP3-test static stack audit | Compile changed audio/decoder sources with `-fstack-usage` | Known foreground/decode frames fit the reserved stack | V10: `CH32_MP3_PlayFile` 272 B and `main` 80 B; `MP3DecodeInternal` remains 96 B and the earlier largest Helix subroutine was 272 B; reserved stack 2,304 B | PASS |
| FreeRTOS removal | Inspect full ELF symbols/map and managed inputs | No scheduler/strong SysTick/task/queue/heap state linked | No RTOS API/state symbol; only startup weak `SysTick_Handler`/`SW_Handler`; source tree removed | PASS |
| MP3 HardFault stage retention | Cause/reproduce playback fault | Next boot shows `HARDFAULT` and exact retained MP3 stage for five seconds | Relocated record successfully reported `MP3 DECODE` on hardware | PASS |
| Reference ENC asset structure | Decrypt and scan `D:\001\0001.enc` on the host | Valid supported frames with no truncation or scan gaps | 428,013 B; SHA-256 `C261CFF4085067513016C3FF74E49773B22CCD17FE11BE51FC20FD52D13FB516`; ID3v2.4 ends at offset 45; 743 contiguous mono MPEG-1 Layer III frames, 32 kHz, 128 kbps, 576 B/frame, exact EOF | PASS |
| Helix substage retention | Reproduce playback fault with the fine-grained image | LCD identifies header, side-info, main-data, scale-factor, Huffman, dequantization, IMDCT, or subband | LCD reported `DEC SUBBAND` | PASS |
| Compact mono synthesis bounds | Audit every FDCT/polyphase history-buffer address over one granule | All accesses remain inside `SubbandInfo.vbuf[1088]` | Original MAX_NCHAN=1 layout reached index 2,127; corrected compact layout reaches 0..1,071 within 0..1,087. One- and two-channel affected-source syntax checks pass. | PASS |
| Compact mono synthesis playback | Play `001/0001.ENC` with the corrected FDCT/polyphase layout | No `DEC SUBBAND` HardFault; decoded data reaches audio initialization and DMA | LCD remained on `PLAYING: / 001/0001.ENC`; no HardFault was reported, but no final status or audible output was observed | IN PROGRESS |
| Audio pipeline counters | Play `001/0001.ENC` with the diagnostic image | Nonzero PCM peak and increasing completed-DMA-slot count; stalled DMA returns within 500 ms | Firmware built; target result not yet supplied | PENDING |
| G8R6 final link | Link complete cooperative full application | Fits configured FLASH/RAM | Restored pre-WAV LTO link: 61,352 B loadable Flash and 2,136 B application-region margin; RAM is 19,812 B with 668 B free | PASS |
| Cooperative tick vector | Inspect the final LTO ELF and run the full image | `TIM3_IRQHandler` is a strong project handler and `Loading...` advances to the reference idle/date flow | Final ELF reports a strong `T` handler; on-target confirmation remains required | PASS (STATIC) |
| Full-profile SRAM budget | Inspect link map | RAM fits with safe stack margin | Includes fixed 2,048 B main/ISR stack; requirement is 19,812 B, leaving 668 B; runtime high-water validation remains required | PASS (STATIC) |
| Old mono MPEG-1 rejection | Play folder `001` `.enc` with the previous bounded build | Valid MPEG-1 is rejected before its 1,152 samples overflow the 576-sample slot | LCD showed `MPEG1 REJECTED`, underflow 0 | PASS |
| Mono MPEG-1 decoder test | Decode supported mono MPEG-1 with the current two-slot mode | Reference-correct PCM reaches PWM; no mandatory frame gap; actual underflows recorded | Host output is bit-identical to reference; V8 plain MP3 is normal-speed/distortion-free and ENC playback is user-confirmed; final underflow counts remain unreported | IN PROGRESS |
| Audio PWM carrier | Measure PA8 during playback | Nominal 375 kHz carrier at confirmed 96 MHz | Previous HSE selection could leave carrier near 31.25 kHz; V8 measurement pending | PENDING |
| Audio sample timing | Measure/inspect TIM2 timing | Matches decoded sample rate | V9 math: 32 kHz is exact at 3,000 ticks/sample; 44.1 kHz phase cycle is exact over 147 samples; physical measurement pending | IN PROGRESS |
| AUX/speaker audible output | Listen through the actual downstream amplifier/speaker | Recognizable, stable audio without audible distortion | V8 plain MP3 plays at normal speed with no audible distortion; electrical rail-clipping/filter measurements remain pending | PASS |
| Deployed-profile buffer continuity | Play representative 32-kHz mono 64/128-kbps assets | No audible gaps; underflows = 0 | Prior plain and ENC playback pass; final production underflow counts pending | IN PROGRESS |
| MPEG-1 granule handoff | Play `mp3_song.mp3` with the corrected HSI clock | Recognizable normal-speed audio; no forced once-per-frame silence; actual underflow count recorded | Host hook/PCM comparison pass; V8 is normal-speed and distortion-free; completed-song underflow result remains pending | IN PROGRESS |
| `.enc` host encoder | Apply the utility twice and compare SHA-256 with source | Second XOR pass reproduces original bytes | Round-trip hash matched | PASS |
| Standalone `TEST.TXT` diagnostic revalidation | Select the diagnostic profile and exercise the viewer | File pages on LCD; UP/DOWN page; MENU exits/reopens; no FreeRTOS API is called | Profile links; earlier hardware behavior passed; post-parity target run pending | PENDING |
| Folder `001` ENC discovery | Place supported `.enc` files in folder `001` | Alphabetically first short filename is selected case-insensitively and auto-played | V9 implementation/build checks PASS and user confirmed working ENC playback | PASS |
| `.enc` decryption/structure | Decrypt mounted folder `001` and validate each resulting stream | Exact XOR round-trip; valid supported contiguous MP3 frames to EOF | All 25 files and 21,843 frames passed; exact parameters and `0001.enc` hashes recorded above | PASS |
| `.enc` physical playback | Flash and play selected `001/*.enc` | Decrypt, decode, and play through the physical speaker correctly | User confirmed `.enc` playback works correctly; exact final counters/underflows were not supplied | PASS |
| Settings load | Parse a known `settings.txt` | Welcome text, lat/lon, timezone, calendar mode, panchang order match | Source implemented; not run | PENDING |
| Holiday skip | Match RTC date against a `holiday.txt` entry | Scheduled playback skipped that day | Source implemented; not run | PENDING |
| Playlist schedule matching | Types `1`/`2`/`3` against known RTC state | Correct sequence selected/skipped | Source implemented; not run | PENDING |
| Group rotation (`g`) | Repeated scheduled plays of a `g` token | Index increments/wraps/persists | Source implemented; not run | PENDING |
| Resume after power loss | Interrupt a sequence mid-window, reboot | Already-played groups skipped | Source implemented; not run | PENDING |
| Panchang calculation accuracy | Compare compact model with actual ESP C VSOP/ELP behavior over 2000-2050 | Practically identical consumed categories and rise/set output | 2,662 category, 120 rise/set, and 612 transition cases; tithi/nakshatra/yoga mismatches 1/0/4; max Sun/Moon error 0.012/0.019 degrees; solar/lunar events within 0.024/0.089 minute | PASS (HOST) |
| Festival / special-day matching | Known input files against calculated Panchang | Correct id; solar rule retained | Source implemented; not run | PENDING |
| Menu flows | Exercise all adopted setup screens | Correct store locations and LCD behavior | Source implemented; not run | PENDING |
| EEPROM-store persistence | Write known values, power-cycle, re-read | Values survive power loss | Host simulation PASS; target not run | PENDING |
| Relay control | Trigger amp-on and playback | `RLY_IN` follows specification | Source implemented; not run | PENDING |
| Light scheduling | Test normal and midnight-spanning windows | `LGT_IN` follows schedule | Source implemented; not run | PENDING |
| Full-port RAM budget | Inspect link map with all adopted modules linked in | Fits 20 KB with adequate stack margin | Measured 19,812 B including fixed 2,048 B stack; 668 B remains; hardware high-water test pending | PASS (STATIC) |
| Cooperative scheduling equivalence | Compare task periods/yield points with replacement services | 20/600/1000 ms and playback 1/10/100 ms behavior retained | Source mapping complete; hardware regression pending | PENDING |
| DMA1 Channel2 request mapping | Confirm TIM2_UP -> DMA1 Channel2 against the CH32V203 reference manual | Mapping confirmed correct, or corrected in code | WCH CH32FV2x/V3x manual table confirms TIM2_UP on Channel2 | PASS |
| Audio DMA carrier/timing | Measure PA8 PWM duty updates during playback | Duty changes at the decoded sample rate; matches pre-DMA (ISR) behavior | Not measured | PENDING |
| Audio DMA underflow semantics | Play a file with an intentionally slow SD/decode path | `CH32_MP3_GetUnderflowCount()` increments only on real gaps, not on every slot handoff | Not tested | PENDING |
| R1 AUX/LM358 response | Verify the populated 4.7 kOhm/10 nF and 10 kOhm/10 nF sections, gain-10 stage, and `AUX_OUT` | Stable required-band response, PWM-carrier attenuation, about 2.5 V quiescent pin-1 bias, and no rail clipping | R1 schematic reviewed; populated board not measured | PENDING |
| Relay/light GPIO polarity | Drive `RLY_IN`/`LGT_IN` HIGH and LOW, observe the transistor/relay stage | Confirms which level is "on" | Not tested | PENDING |
| Full integration | Operate required peripherals together | No interference | Not completed | PENDING |

## Dependencies

### Hardware

- CH32V203C8T6 development kit.
- CH32V203G8R6 custom PCB.
- WCH programming/debug hardware.
- LCD and three switches.
- DS1307 and its backup/pull-up arrangement.
- Compatible microSD card formatted for the configured FatFs volume.
- Oscilloscope or logic analyzer.
- Powered speaker or external audio amplifier appropriate for the two-pin
  `AUX_OUT` signal connector. The schematic does not include a speaker power
  amplifier; exact test amplifier/load is **TBD**.
- Stable `+5V_IN` supply.

### Software

- MounRiver Studio 2.5.0 managed project with WCH RISC-V Embedded GCC 8.2.0,
  as recorded by the generated makefiles and build output.
- CH32V20x device support included in the repository.
- FatFs included in the repository.
- Helix decoder source and retained licenses included in the repository.
- MP3 conversion/encoding tool for supported mono MPEG-1/2 assets: **TBD**.
- Windows PowerShell/.NET for optional `.enc` asset generation; no external
  module is required.
- PowerShell 7 or later (`pwsh`) for `tools/build_g8r6.ps1`. Windows PowerShell
  5 is rejected because its native-argument handling does not reproduce the
  reference LTO image byte-for-byte.
- Serial terminal software: **TBD**.

The current FatFs configuration has long-filename support disabled. Folder
`001` audio selection therefore displays and opens the FAT 8.3 short filename
returned by `f_readdir()`; encrypted audio files must be discoverable directly
inside that folder.

## Constraints

- CH32V203G8R6 SRAM is limited to 20 KB.
- Stereo buffers do not fit the selected low-memory design. Mono MPEG-1 uses
  both existing 576-sample slots for each 1,152-sample frame and may have a
  decode gap at frame boundaries.
- A linker map must confirm the final RAM footprint and stack margin.
- Only one decoder instance may be active because its state is statically
  allocated.
- MP3 playback is blocking and depends on SD/FatFs working reliably.
- PA8/TIM1_CH1 is assigned exclusively to MP3 audio in the MP3-test and full
  profiles and is not initialized by the SD/LCD-only diagnostic; the legacy
  PWM module is outside the build.
- TIM2 is reserved by the MP3 wrapper during playback.
- Timer calculations depend on the actual timer clock, not only the CPU-clock
  constant.
- SD initialization depends on correct power, wiring, CS timing, SPI mode, and
  card compatibility.
- I2C operation depends on correct wiring, voltage, address, and pull-ups.
- Plain MPEG-1 audio quality is proven audible at normal speed without
  distortion, and ENC playback is confirmed working. The new maximum-safe
  digital level on both paths and production underflow/counter results remain
  unverified on hardware.
- LM358 is the R1 signal-conditioning op amp, not a speaker power amplifier.
  The actual downstream power stage/load is **TBD**.
- The shown LM358 channel has nominal signal gain 10 on a 5 V supply and 2.5 V
  bias. Full-range PWM-derived audio can exceed its clean positive output-swing
  margin. Keep volume 255 only subject to scope/listening validation at pin 1
  and `AUX_OUT`; do not infer clean operation from successful playback alone.
- ESP enables both internal DAC channels; CH32 exposes one PA8 PWM channel.
  Enabling both ESP channels does not itself prove a 6 dB advantage unless the
  reference analog stage sums or differentially uses them. Record which ESP
  DAC pin(s) feed the speaker amplifier and the amplifier topology before
  attempting to compensate this difference in firmware.
- The installed filter capacitances, population of the two parallel feedback
  capacitors, and the existence/location of a mid-supply bias are release
  blockers. Do not infer them from the drawing alone.
- The old RAW PA8 and DMA sine diagnostics are historical only and were removed
  in V10. The Helix PCM remains independently proven sample-identical to the
  untouched reference.

### Full-Port Memory and Platform Constraints

- The SD/LCD-only diagnostic profile fits: its refreshed 2026-08-26 WCH GCC
  build uses 8,616 bytes of loadable flash and 5,292 bytes of RAM.
- The default MP3-test profile fits with little unallocated SRAM. The refreshed
  reproducible WCH GCC LTO link uses 39,244 of 63,488 bytes of loadable flash
  and 19,324 of 20,480 bytes of RAM. The size tool includes the 2,048-byte
  main/ISR stack and 8-byte `.noinit` fault record, leaving 1,156 bytes. Do not
  add globals or features to this test image without recovering and
  re-measuring RAM.
- The V10 GCC stack report shows 272 bytes for `CH32_MP3_PlayFile` and 80 bytes
  for `main`; the V9 ENC discovery measurement was 128 bytes.
  `MP3DecodeInternal` remains 96 bytes
  and `Audio_GranuleOutputHook` remains bounded. An earlier
  full-source audit found 272 bytes for the largest Helix subroutine.
  `FF_FS_TINY=1` remains enabled. The reserved stack is 2,048 bytes. Static
  analysis does not replace a runtime stack-guard check under DMA interrupts.

- The combined RAM footprint of the Panchang astronomy engine, the playlist scheduling engine, the
  extended menu/setup flow, the EEPROM-style config store, FatFs, and the
  Helix MP3 decoder/PCM buffers must fit within the 20 KB SRAM budget with
  an adequate worst-case stack margin. The refreshed cooperative link requires
  19,812 bytes including the fixed 2,048-byte main/ISR stack, leaving 668
  bytes. Helix static state is the dominant RAM consumer and runtime
  high-water validation remains open.
- The restored pre-WAV full-profile link requires 61,352 bytes of loadable
  Flash, leaving 2,136 bytes in the reserved 63,488-byte application region.
  It requires 19,812 bytes of RAM including the fixed stack, leaving 668
  bytes.
- `esp_bell_idf` is C++ under ESP-IDF. This repository is cooperative C using
  the WCH SDK. Adopted modules use plain C structures and project-local
  wrappers while preserving documented behavior and file formats.
- No SPIFFS-equivalent internal filesystem exists on this target; all
  `esp_bell_idf` `SPI_MOUNT_POINT` (SPIFFS-staged copy) reads become direct
  SD/FatFs reads. Repeated per-menu-item and per-scheduled-play SD file opens
  must be measured for acceptable latency under FatFs on this target.
- CH32V203 (RISC-V IMAC) has no confirmed hardware FPU; the compact Panchang
  engine's single-precision execution time must be measured on target.
- Persistent-store endurance depends on the CH32V203 fast-page guarantees.
  The eight-slot rotation spreads writes, but settings and scheduled-play
  commits still require on-target endurance and interruption testing.

## Acceptance Criteria

The project is complete only when all applicable criteria are satisfied:

1. The G8R6 PCB boots reliably with the confirmed clock configuration.
2. Serial monitoring configuration is documented and verified.
3. LCD and keypad operate reliably with the documented pins.
4. DS1307 detection, read, write, and persistence tests pass.
5. SD initialization, block access, and FatFs file reading pass.
6. The final role of PA8 is decided and documented.
7. The G8R6 firmware builds without unjustified warnings or errors.
8. The linker map confirms RAM use within 20 KB with adequate stack margin.
9. Deployed MPEG-1 Layer III mono, 32-kHz, indexed 64/128-kbps content decodes
   from SD with usable audio; granule handoff eliminates the old mandatory
   frame-boundary gap and measured production underflows meet the acceptance
   target. CRC-header skipping, padding, ID3v2, plain MP3, and encrypted input
   remain supported.
10. PA8 PWM and TIM2 sample timing match their calculated values.
11. `AUX_OUT` produces acceptable audio. Production assets report zero buffer
    underflows, or any measured residual decode-time gaps are explicitly
    resolved. The R1 LM358 stage is verified for the documented populated
    values, stable audio-band response, approximately 2.5 V pin-1 bias, and
    unclipped output swing into the intended load.
12. All required peripherals coexist without clock, timer, or pin conflicts.
13. Long-duration custom-PCB testing passes.
14. All remaining TBD items needed by the final product are resolved.
15. Settings, holiday, playlist, festival, special-day, and menu file formats
    parse identically to the `esp_bell_idf` reference behavior for the
    non-radio scope defined above.
16. The compact Panchang engine remains within the recorded practical
    longitude and sunrise/sunset error bounds across 2000-2050; independent
    moonrise/moonset validation is completed before release.
17. The EEPROM-style config store persists correctly across power loss using
    a confirmed storage mechanism, and supports resume of an interrupted
    scheduled sequence.
18. Relay and light outputs behave per the adopted scheduling/amp-control
    logic and are verified on the custom PCB.
19. `.enc` playback produces audio equivalent to its decrypted MP3 source, and
    the shared plain/ENC gain provides maximum practical listening level
    without clipping, distortion, pumping, added noise, amplifier overcurrent,
    speaker distress, or unsafe component temperature.
20. The full-port RAM budget fits within 20 KB SRAM with adequate stack
    margin, confirmed from the linker map with every adopted module linked
    in.
