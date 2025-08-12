#include "SD_card.h"
#include <string.h>
#include <stdio.h>

static int ends_with_case(const char *s, const char *ext) {
    size_t ls = strlen(s), le = strlen(ext);
    if (ls < le) return 0;
    for (size_t i = 0; i < le; i++) {
        char a = s[ls - le + i], b = ext[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return 0;
    }
    return 1;
}

static void replace_ext(char *path, const char *newext) {
    char *dot = NULL;
    for (char *p = path; *p; ++p) if (*p == '.') dot = p;
    if (dot) strcpy(dot, newext);
}

static void trim_crlf(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1]=='\r' || s[n-1]=='\n')) s[--n] = 0;
}

//tsq
static FRESULT parse_tsq_header(const char *tsq_path, SongMeta *info) {
    FIL f; FRESULT fr = f_open(&f, tsq_path, FA_READ | FA_OPEN_EXISTING);
    if (fr != FR_OK) return fr;

    memset(info, 0, sizeof(*info));
    strcpy(info->difficulty, "N/A");
    char line[160];

    for (;;) {
        TCHAR *r = f_gets((TCHAR*)line, sizeof(line), &f);
        if (!r) break;
        trim_crlf(line);
        if (strcmp(line, "---") == 0) break;

        if      (strncmp(line, "Name:", 5) == 0)      { strncpy(info->name,      line+5,  sizeof(info->name)-1);      while(info->name[0]==' ') memmove(info->name, info->name+1, strlen(info->name)); }
        else if (strncmp(line, "Artist:",7) == 0)      { strncpy(info->artist,    line+7,  sizeof(info->artist)-1);    while(info->artist[0]==' ') memmove(info->artist, info->artist+1, strlen(info->artist)); }
        else if (strncmp(line, "BPM:",  4) == 0)       { info->bpm = atoi(line+4); }
        else if (strncmp(line, "Difficulty:",11) == 0) { strncpy(info->difficulty,line+11, sizeof(info->difficulty)-1); while(info->difficulty[0]==' ') memmove(info->difficulty, info->difficulty+1, strlen(info->difficulty)); }
        else if (strncmp(line, "Offset:",7) == 0)      { info->offset_ms = atoi(line+7); }
    }
    f_close(&f);
    strncpy(info->tsq_path, tsq_path, sizeof(info->tsq_path)-1);
    return FR_OK;
}

//wav
static FRESULT parse_wav_info(const char *wav_path, SongMeta *m) {
    strncpy(m->wav_path, wav_path, sizeof(m->wav_path) - 1);
    m->wav_path[sizeof(m->wav_path) - 1] = 0;

    FIL f; FRESULT fr = f_open(&f, wav_path, FA_READ | FA_OPEN_EXISTING);
    if (fr != FR_OK) return fr;

    BYTE hdr[12]; UINT br=0;
    fr = f_read(&f, hdr, 12, &br);
    if (fr != FR_OK || br != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr+8, "WAVE", 4)) {
        f_close(&f); return FR_INT_ERR;
    }

    uint32_t data_size = 0;
    for (;;) {
        BYTE chdr[8];
        br=0; fr = f_read(&f, chdr, 8, &br);
        if (fr != FR_OK) { f_close(&f); return fr; }
        if (br != 8) break;

        uint32_t sz = chdr[4] | (chdr[5]<<8) | (chdr[6]<<16) | (chdr[7]<<24);

        if (!memcmp(chdr, "fmt ", 4)) {
            BYTE fmt[32]; UINT toread = (sz > sizeof(fmt)) ? sizeof(fmt) : sz;
            fr = f_read(&f, fmt, toread, &br);
            if (fr != FR_OK || br < 16) { f_close(&f); return FR_INT_ERR; }
            uint16_t audioFmt = fmt[0] | (fmt[1]<<8);
            (void)audioFmt; // PCM=1，其它暂不强制
            m->wav_channels = fmt[2] | (fmt[3]<<8);
            m->wav_sr       = fmt[4] | (fmt[5]<<8) | (fmt[6]<<16) | (fmt[7]<<24);
            m->wav_bits     = fmt[14] | (fmt[15]<<8);
            if (sz > toread) f_lseek(&f, f_tell(&f) + (sz - toread));
        } else if (!memcmp(chdr, "data", 4)) {
            data_size = sz;
            f_lseek(&f, f_tell(&f) + sz);
        } else {
            f_lseek(&f, f_tell(&f) + sz);
        }
        if (sz & 1) f_lseek(&f, f_tell(&f) + 1);
    }
    f_close(&f);

    if (m->wav_sr && m->wav_channels && m->wav_bits && data_size) {
        uint32_t bps = m->wav_sr * m->wav_channels * (m->wav_bits/8);
        m->wav_duration_ms = bps ? (uint32_t)((1000ULL * data_size) / bps) : 0;
        strncpy(m->wav_path, wav_path, sizeof(m->wav_path)-1);
        return FR_OK;
    }
    return FR_INT_ERR;
}

FRESULT SongFS_Mount(FATFS *fs) {
    return f_mount(fs, SONGFS_VOL, 1);
}

FRESULT SongFS_Unmount(void) {
    return f_mount(NULL, SONGFS_VOL, 0);
}

static void path_join(char *out, size_t outsz, const char *dir, const char *name) {
    size_t n = snprintf(out, outsz, "%s%s%s",
                        dir,
                        (dir[0] && dir[strlen(dir)-1] == '/') ? "" : "/",
                        name);
    if (n >= outsz) out[outsz-1] = 0;
}

static size_t scan_dir(const char *path, SongMeta *out, size_t cap, size_t *count) {
    DIR dir; FILINFO fno; FRESULT fr;
    fr = f_opendir(&dir, path);
    if (fr != FR_OK) return *count;

    while (1) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break;

        if (fno.fattrib & AM_DIR) {
#if SONGFS_RECURSIVE
            if (fno.fname[0] != '.') {
                char sub[128];
                path_join(sub, sizeof(sub), path, fno.fname);
                scan_dir(sub, out, cap, count);
            }
#endif
            continue;
        }

        if (!ends_with_case(fno.fname, ".tsq")) continue;
        if (*count >= cap) continue;

        SongMeta tmp; memset(&tmp, 0, sizeof(tmp));
        path_join(tmp.tsq_path, sizeof(tmp.tsq_path), path, fno.fname);

        if (parse_tsq_header(tmp.tsq_path, &tmp) != FR_OK) continue;

        strncpy(tmp.wav_path, tmp.tsq_path, sizeof(tmp.wav_path)-1);
        replace_ext(tmp.wav_path, ".wav");

        (void)parse_wav_info(tmp.wav_path, &tmp);

        out[(*count)++] = tmp;
    }
    f_closedir(&dir);
    return *count;
}


size_t SongFS_ListSongs(const char *root, SongMeta *out, size_t max_items) {
    size_t n = 0;
    scan_dir(root, out, max_items, &n);
    return n;
}

void SongFS_Print(const SongMeta *m, void (*print)(const char *fmt, ...)) {
    uint32_t mm = m->wav_duration_ms / 60000U;
    uint32_t ss = (m->wav_duration_ms % 60000U) / 1000U;
    print("\r\n[Sequence]\r\n  File: %s\r\n  Name: %s\r\n  Artist: %s\r\n  BPM: %d\r\n  Difficulty: %s\r\n  Offset: %d ms\r\n",
          m->tsq_path,
          m->name[0]?m->name:"(N/A)",
          m->artist[0]?m->artist:"(N/A)",
          m->bpm, m->difficulty, m->offset_ms);

    if (m->wav_sr && m->wav_channels && m->wav_bits) {
        print("[Audio]\r\n  File: %s\r\n  %u ch, %lu Hz, %u-bit\r\n  Length: %02lu:%02lu\r\n",
              m->wav_path,
              (unsigned)m->wav_channels, (unsigned long)m->wav_sr, (unsigned)m->wav_bits,
              (unsigned long)mm, (unsigned long)ss);
    } else {
        print("[Audio]\r\n  File: %s\r\n  (open/parse failed)\r\n", m->wav_path);
    }
}

