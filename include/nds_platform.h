/*
 * Duke Nukem 3D - Nintendo DS Port
 * nds_platform.h - NDS hardware abstraction layer
 */

#ifndef NDS_PLATFORM_H
#define NDS_PLATFORM_H

#include <nds.h>
#include "types.h"

/* ---- NDS Screen Constants ---- */
#define NDS_SCREEN_WIDTH    256
#define NDS_SCREEN_HEIGHT   192

/* ---- Framebuffer ---- */
extern uint16_t *nds_framebuffer;

/* ---- Platform Functions ---- */

/* System */
int32_t  nds_init(void);
void     nds_shutdown(void);

/* Video */
void     nds_initVideo(void);
void     nds_flipScreen(void);
void     nds_clearScreen(uint16_t color);
void     nds_drawPixel(int32_t x, int32_t y, uint16_t color);
void     nds_drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color);
void     nds_drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
void     nds_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);

/* Color conversion */
static inline uint16_t nds_rgb15(uint8_t r, uint8_t g, uint8_t b)
{
    return RGB15(r >> 3, g >> 3, b >> 3) | BIT(15);
}

/* Palette conversion: Build engine palette (6-bit) to NDS (5-bit) */
static inline uint16_t nds_paletteConvert(uint8_t r, uint8_t g, uint8_t b)
{
    return RGB15(r >> 1, g >> 1, b >> 1) | BIT(15);
}

/* Input */
void     nds_pollInput(void);
uint32_t nds_getKeys(void);
int16_t  nds_getTouchX(void);
int16_t  nds_getTouchY(void);
BOOL     nds_isTouching(void);

/* Timer */
void     nds_initTimer(void);
uint32_t nds_getTicks(void);   /* milliseconds since init */
void     nds_delay(uint32_t ms);

/* Audio (stubs for now) */
void     nds_initAudio(void);
void     nds_shutdownAudio(void);
void     nds_playSound(int32_t soundnum);
void     nds_playMusic(const char *filename);
void     nds_stopMusic(void);

/* File I/O */
int32_t  nds_initFilesystem(void);
int32_t  nds_fileExists(const char *filename);

/* Console/Debug (bottom screen) */
void     nds_initConsole(void);
void     nds_consolePrint(const char *fmt, ...);
void     nds_consoleClear(void);

#endif /* NDS_PLATFORM_H */
