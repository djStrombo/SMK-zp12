# SMK-zp12

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)
[![Release](https://img.shields.io/badge/release-v2.4.4--smk--zp12-orange.svg)](https://github.com/djStrombo/SMK-zp12/releases)
[![Web Flasher](https://img.shields.io/badge/Web%20Flasher-Live%20Online-brightgreen.svg)](https://djstrombo.github.io/SMK-zp12/)
[![Hardware](https://img.shields.io/badge/hardware-M--VAVE%20SMK--37%20(Pro%20%26%20Elite)-cyan.svg)](https://github.com/djStrombo/SMK-zp12)

SMK-zp12 is an 8-track groovebox, sampler, and synthesizer firmware ported to the M-VAVE SMK-37 (Pro and Elite).

Web Flasher, Mapper, and Sound Studio: https://djstrombo.github.io/SMK-zp12/

Based on ZP-12 for the M-VAVE FM-1 created by Sven Trogus (zvenson).

---

## Overview

SMK-zp12 brings the full 8-track groovebox and sampling capabilities of ZP-12 to the M-VAVE SMK-37 Pro and Elite hardware:

- 37 velocity-sensitive keys with scale quantizer and arpeggiator.
- 16 RGB velocity pads for drum triggers, sample slicing, and step sequencing.
- 8 continuous rotary encoders mapped to sound parameters, tempo, pad select, and master volume.
- 4 physical faders for live mixer levels (Banks 1-4 and 5-8), master volume, filter cutoff, and resonance.
- 18 dedicated hardware buttons mapped to ZP-12 navigation, transport, recording, and edit pages.
- Dual-mode architecture: Synthesizer / Groovebox mode and MIDI Controller mode (toggle via PATCH + PARA hold).
- Live On-The-Fly SysEx remapper for buttons, knobs, faders, and MIDI CC numbers without reflashing.
- Web suite featuring Web MIDI flasher, live RAM remapper, official sample chopper/editor, backup tool, and interactive cheat sheet.

---

## Web Suite

Visit https://djstrombo.github.io/SMK-zp12/ in Google Chrome, Edge, or Brave:

1. View 01 - Flasher & Hardware Remapper:
   - Auto-detects SMK-37 Pro and Elite hardware models.
   - Flashes SMK-zp12 firmware or restores factory V016 in one click.
   - Configures button assignments and MIDI controller CCs in real time via USB SysEx.

2. View 02 - Sample Chopper & Sound Editor:
   - Dedicated sample management, waveform slicing, and sound editing.

3. View 03 - Backup Tool:
   - Backup and restore loops, sounds, songs, and DX7 patches via USB MIDI.

4. View 04 - Cheat Sheet:
   - Complete reference guide for shortcuts, button combinations, and recording workflow.

---

## License

GPL-3.0 License. See LICENSE for details.
Original ZP-12 codebase copyright Sven Trogus (zvenson).
SMK-37 port by djStrombo.
