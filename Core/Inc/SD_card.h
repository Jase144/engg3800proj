#pragma once
#include "ff.h"       // FatFs
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // tsq 
    char     name[65];
    char     artist[65];
    int      bpm;
    char     difficulty[12];   // N/A, EASY, MEDIUM, HARD, EXPERT, GIGA
    int      offset_ms;        // 0..1000

    char     tsq_path[128];
    char     wav_path[128];

    // wav info
    uint16_t wav_channels;
    uint32_t wav_sr;
    uint16_t wav_bits;
    uint32_t wav_duration_ms;  
} SongMeta;

// Whether to recursively scan subdirectories (change to 1 if necessary)
#ifndef SONGFS_RECURSIVE
#define SONGFS_RECURSIVE 0
#endif

#ifndef SONGFS_VOL
#define SONGFS_VOL "0:"
#endif

FRESULT SongFS_Mount(FATFS *fs);
FRESULT SongFS_Unmount(void);

size_t  SongFS_ListSongs(const char *root, SongMeta *out, size_t max_items);

void    SongFS_Print(const SongMeta *m, void (*print)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif
