/* SPDX-License-Identifier: GPL-3.0-only */
/* zp12's USB link for the web tools (backup now, the sample editor next): a few raw flash commands, the
 * logic in the browser. A message is F0 pack7(7D 'Z' 'P' cmd addr:3 len:2 data:len sum) F7, packed as the
 * updater's (ota.c: ota_pack7), sum = ~(the bytes from cmd on); the reply has the same header and cmd, then
 * rc, addr, len, data, sum.
 *   1 HELLO        -> data: protocol, "zp12 X.Y", 0, the kit's waves
 *   2 READ a n     -> n <= 512 bytes of flash from 0x93000 up
 *   3 ERASE a      -> the 4 KiB sector at a          } only where zp12 keeps things: its store and samples,
 *   4 WRITE a data -> programmed, read back, compared } and sloopDX's DX7 banks + MY KIT (0xA0000..0xC3FFF)
 *   5 HOLD         -> the sequencer stops; no autosave while the link is busy (10 s from the last message)
 *   6 REBOOT       -> after the reply (a restored store is read at the start)
 *   7 RELOAD       -> the samples' directory read again (sp_samples.c); data: how many there are
 *   8 ASSIGN a     -> pad a & 31 plays wave (a >> 8) & 63, heard at once (the web editor's "on a pad");
 *                     bit 20 set: its CHOP group becomes (a >> 16) & 15 (5; 4 sent bit 18 + group 1-3 in 16-17)
 *   9 PADS         -> data: the wave of each of the 32 pads (0xFF none), then the kit's names, each ended by 0 */
#define ZL_MAX 512u
#define ZL_PROTO 5u                              /* 2: RELOAD, ASSIGN (the sample editor); 3: PADS; 4: ASSIGN's CHOP; 5: 8 groups */
static uint8_t zl_dec[16 + ZL_MAX], zl_wire[32 + (16 + ZL_MAX) * 8u / 7u];
static uint8_t zl_back[ZL_MAX];
static uint32_t zl_last;                         /* fm1_ms of the last message (0: never) */

static int zl_busy(void) { return zl_last && (uint32_t)(fm1_ms - zl_last) < 10000u; }

static int zl_writable(uint32_t a, uint32_t n)
{
#if BOARD_SMK37
    return FL_STORE_OK(a, n);
#else
    return FL_IN(a, n, 0xA0000u, 0xDC000u) || FL_IN(a, n, 0xE5000u, 0xE9000u) || FL_IN(a, n, 0xEA000u, 0xFC000u);
#endif
}

static void zl_reply(uint32_t cmd, uint32_t rc, uint32_t a, const uint8_t *d, uint32_t n)
{
    uint8_t *m = zl_dec;
    uint32_t i, s = 0, w;
    m[0] = 0x7D; m[1] = 'Z'; m[2] = 'P'; m[3] = (uint8_t)cmd; m[4] = (uint8_t)rc;
    m[5] = (uint8_t)a; m[6] = (uint8_t)(a >> 8); m[7] = (uint8_t)(a >> 16); m[8] = (uint8_t)n; m[9] = (uint8_t)(n >> 8);
    for (i = 0; i < n; i++) m[10 + i] = d[i];
    for (i = 3; i < 10u + n; i++) s += m[i];
    m[10 + n] = (uint8_t)~s;
    zl_wire[0] = 0xF0;
    w = 1u + ota_pack7(m, 11u + n, zl_wire + 1);
    zl_wire[w++] = 0xF7;
    ota_wire_send(zl_wire, w);
}

#if BOARD_SMK37
static int smk_sysex_frame(const uint8_t *body, uint32_t n)
{
    if (n < 5u || body[0] != 0x7D || body[1] != 0x53 || body[2] != 0x4D || body[3] != 0x4B)
        return 0;

    uint8_t cmd = body[4];
    if (cmd == 0x01) {
        /* Query Mapping (Get Mapping & Extended MIDI Config from RAM) */
        uint8_t resp[104];
        uint32_t pos = 0;
        resp[pos++] = 0xF0;
        resp[pos++] = 0x7D;
        resp[pos++] = 0x53; /* 'S' */
        resp[pos++] = 0x4D; /* 'M' */
        resp[pos++] = 0x4B; /* 'K' */
        resp[pos++] = 0x11; /* Query reply */
        for (uint32_t i = 0; i < 18u; i++) {
            uint8_t val = smk_btn_map[i];
            resp[pos++] = (val == ACT_NONE) ? 0x7F : (val & 0x7F);
        }
        for (uint32_t i = 0; i < 8u; i++) {
            uint8_t val = smk_enc_map[i];
            resp[pos++] = (val == ENC_ACT_NONE) ? 0x7F : (val & 0x7F);
        }
        for (uint32_t i = 0; i < 4u; i++) {
            uint8_t val = smk_fad_map[i];
            resp[pos++] = val & 0x7F;
        }
        /* Extended MIDI Controller Configuration (channels & CCs) */
        resp[pos++] = smk_midi_ch & 0x0Fu;
        resp[pos++] = smk_midi_knob_ch & 0x0Fu;
        resp[pos++] = smk_midi_fad_ch & 0x0Fu;
        resp[pos++] = smk_midi_pad_ch & 0x0Fu;
        for (uint32_t i = 0; i < 16u; i++) resp[pos++] = smk_midi_knob_cc[i] & 0x7Fu;
        for (uint32_t i = 0; i < 8u; i++)  resp[pos++] = smk_midi_fad_cc[i] & 0x7Fu;
        for (uint32_t i = 0; i < 32u; i++) resp[pos++] = smk_midi_pad_cc[i] & 0x7Fu;
        resp[pos++] = 0xF7;
        ota_wire_send(resp, pos);
        return 1;
    } else if (cmd == 0x02) {
        /* Set Mapping & Extended MIDI Config (RAM) */
        if (n >= 35u) {
            for (uint32_t i = 0; i < 18u; i++) {
                uint8_t val = body[5u + i];
                smk_btn_map[i] = (val == 0x7F) ? ACT_NONE : val;
            }
            for (uint32_t i = 0; i < 8u; i++) {
                uint8_t val = body[23u + i];
                smk_enc_map[i] = (val == 0x7F) ? ENC_ACT_NONE : val;
            }
            for (uint32_t i = 0; i < 4u; i++) {
                smk_fad_map[i] = body[31u + i];
            }
            if (n >= 95u) {
                smk_midi_ch      = body[35u] & 0x0Fu;
                smk_midi_knob_ch = body[36u] & 0x0Fu;
                smk_midi_fad_ch  = body[37u] & 0x0Fu;
                smk_midi_pad_ch  = body[38u] & 0x0Fu;
                for (uint32_t i = 0; i < 16u; i++) smk_midi_knob_cc[i] = body[39u + i] & 0x7Fu;
                for (uint32_t i = 0; i < 8u; i++)  smk_midi_fad_cc[i]  = body[55u + i] & 0x7Fu;
                for (uint32_t i = 0; i < 32u; i++) smk_midi_pad_cc[i]  = body[63u + i] & 0x7Fu;
            }
            ui_say("MAPPING UPDATED");
            static const uint8_t ack[8] = {0xF0, 0x7D, 0x53, 0x4D, 0x4B, 0x12, 0x00, 0xF7};
            ota_wire_send(ack, sizeof ack);
            return 1;
        }
    } else if (cmd == 0x03) {
        /* Set / Toggle MIDI Controller Mode (0 = Synth, 1 = MIDI Ctrl, 2 = Toggle) */
        if (n >= 6u) {
            uint8_t m = body[5u];
            if (m == 2u) smk_midi_mode ^= 1u;
            else smk_midi_mode = m ? 1u : 0u;
            ui_say(smk_midi_mode ? "MIDI MODE: ON" : "STANDALONE ON");
            ui.force = 1;
            static const uint8_t ack[8] = {0xF0, 0x7D, 0x53, 0x4D, 0x4B, 0x13, 0x00, 0xF7};
            ota_wire_send(ack, sizeof ack);
            return 1;
        }
    }
    return 0;
}
#endif

/* main loop: a frame for us is taken, any other is left to ota_service */
static void zl_service(void)
{
    const uint8_t *p;
    uint32_t n, d, cmd, a, len, i, s = 0, rc = 0;
    uint8_t h[3];
    if (!ota_frame_get(&p, &n))
        return;
#if BOARD_SMK37
    {
        uint8_t dec[128];
        uint32_t dec_len = ota_unpack7(p, n, dec, sizeof dec);
        if (dec_len >= 5u && dec[0] == 0x7D && dec[1] == 'S' && dec[2] == 'M' && dec[3] == 'K') {
            if (smk_sysex_frame(dec, dec_len)) {
                ota_frame_done();
                return;
            }
        }
    }
#endif
    if (n < 4u || ota_unpack7(p, 4, h, 3) < 3u || h[0] != 0x7D || h[1] != 'Z' || h[2] != 'P')
        return;
    d = ota_unpack7(p, n, zl_dec, sizeof zl_dec);
    ota_frame_done();
    if (d < 10u || d > sizeof zl_dec)
        return;
    for (i = 3; i + 1u < d; i++) s += zl_dec[i];
    cmd = zl_dec[3];
    a = zl_dec[4] | (uint32_t)zl_dec[5] << 8 | (uint32_t)zl_dec[6] << 16;
    len = zl_dec[7] | (uint32_t)zl_dec[8] << 8;
    if ((uint8_t)~s != zl_dec[d - 1u] || len > ZL_MAX || (cmd == 4u && len + 10u != d))
        return;                                  /* damaged: no reply, the host asks again */
    zl_last = fm1_ms ? fm1_ms : 1u;
    if (!flash_ok && cmd >= 2u && cmd <= 4u) {
        zl_reply(cmd, 9, a, 0, 0);
        return;
    }
    switch (cmd) {
    case 1: {
        static const uint8_t HI[] = {ZL_PROTO, 'z', 'p', '1', '2', ' '};
        uint8_t b[16];
        for (i = 0; i < sizeof HI; i++) b[i] = HI[i];
        for (n = 0; ZP12_VERSION[n] && i + 2u < sizeof b; n++) b[i++] = (uint8_t)ZP12_VERSION[n];
        b[i++] = 0;
        b[i++] = KIT_NWAVE;                      /* (the own samples are waves KIT_NWAVE + slot) */
        zl_reply(1, 0, 0, b, i);
        return;
    }
    case 2:
        if (a < 0x93000u || !FL_IN(a, len, 0x93000u, 0x100000u) || st_read(a, zl_back, len))
            rc = 1, len = 0;
        zl_reply(2, rc, a, zl_back, len);
        return;
    case 3: {
        uint32_t took;
        rc = (a & 0xFFFu) || !zl_writable(a, 0x1000u) ? 2u : fl_erase4k_quiet(a, &took) ? 3u : 0u;
        break;
    }
    case 4:
        if (!zl_writable(a, len))
            rc = 2;
        else if (fl_write(a, zl_dec + 9, len) || st_read(a, zl_back, len))
            rc = 3;
        else
            for (i = 0; i < len && !rc; i++)
                rc = zl_back[i] != zl_dec[9 + i] ? 4u : 0u;
        break;
    case 5:
        sq_post(RQ_STOP, 0, 0);
        ui_say("USB BACKUP");
        break;
    case 6:
        zl_reply(6, 0, 0, 0, 0);
        for (i = fm1_ms; (uint32_t)(fm1_ms - i) < 80u;) {
            fm1_wdt_feed();                      /* (the reply on its way) */
            usb_retry(fm1_ms);
        }
        fm1_reboot();
        return;
    case 7: {
        uint8_t b[1] = {0};
        zu_load();
        for (i = 0; i < SP_NUSER; i++) b[0] += sp_wave[KIT_NWAVE + i].n != 0u;
        ui_say(b[0] ? "SAMPLES LOADED" : "NO SAMPLES");
        zl_reply(7, 0, a, b, 1);
        return;
    }
    case 8:
        if (((a >> 8) & 63u) >= KIT_NWAVE + SP_NUSER || !sp_wave[(a >> 8) & 63u].n)
            rc = 2;
        else {
            wave_set(a & 31u, (a >> 8) & 63u);
            uint32_t g = a & (1u << 20) ? (a >> 16) & 15u : a & (1u << 18) ? (a >> 16) & 3u : 99u;   /* (bit 18: 2.4's editor) */
            if (g <= 8u)
                sp_sound[a & 31u].flags = (uint8_t)((sp_sound[a & 31u].flags & ~60u) | g << 2);
        }
        break;
    case 9: {
        uint32_t k;
        for (i = 0; i < SP_NSOUND; i++) zl_back[i] = sp_sound[i].wave;
        for (k = 0; k < KIT_NWAVE && i + 10u < ZL_MAX; k++) {
            for (n = 0; KIT_WAVE[k].name[n]; n++) zl_back[i++] = (uint8_t)KIT_WAVE[k].name[n];
            zl_back[i++] = 0;
        }
        zl_reply(9, 0, a, zl_back, i);
        return;
    }
    default:
        rc = 7;
        break;
    }
    zl_reply(cmd, rc, a, 0, 0);
}
