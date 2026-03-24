/*
 * Duke Nukem 3D - Nintendo DS Port
 * nds_platform.c - NDS hardware abstraction implementation
 */

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "nds_platform.h"

/* ---- Globals ---- */
uint16_t *nds_framebuffer = NULL;

static volatile uint32_t nds_tickCounter = 0;
static PrintConsole bottomConsole;

/* ---- Timer IRQ Handler ---- */
static void nds_timerIRQ(void)
{
    nds_tickCounter++;
}

/* ---- System ---- */
int32_t nds_init(void)
{
    /* Power on everything */
    powerOn(POWER_ALL);

    /* Init video */
    nds_initVideo();

    /* Init console on bottom screen for debug */
    nds_initConsole();

    /* Init timer */
    nds_initTimer();

    /* Init filesystem */
    if (!nds_initFilesystem()) {
        nds_consolePrint("Warning: FAT init failed\n");
    }

    nds_consolePrint("Duke Nukem 3D NDS\n");
    nds_consolePrint("Initializing...\n");

    return 1;
}

void nds_shutdown(void)
{
    nds_shutdownAudio();
}

/* ---- Video ---- */
void nds_initVideo(void)
{
    /* Main engine: framebuffer mode on top screen */
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);

    nds_framebuffer = VRAM_A;

    /* Clear the framebuffer */
    nds_clearScreen(0);
}

void nds_flipScreen(void)
{
    swiWaitForVBlank();
}

void nds_clearScreen(uint16_t color)
{
    if (nds_framebuffer) {
        int i;
        for (i = 0; i < NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT; i++) {
            nds_framebuffer[i] = color;
        }
    }
}

void nds_drawPixel(int32_t x, int32_t y, uint16_t color)
{
    if (x >= 0 && x < NDS_SCREEN_WIDTH && y >= 0 && y < NDS_SCREEN_HEIGHT) {
        nds_framebuffer[y * NDS_SCREEN_WIDTH + x] = color;
    }
}

void nds_drawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color)
{
    /* Bresenham's line algorithm */
    int32_t dx = abs(x2 - x1);
    int32_t dy = abs(y2 - y1);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    while (1) {
        nds_drawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void nds_drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
    nds_drawLine(x, y, x + w - 1, y, color);
    nds_drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    nds_drawLine(x, y, x, y + h - 1, color);
    nds_drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void nds_fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
    int32_t ix, iy;
    for (iy = y; iy < y + h; iy++) {
        for (ix = x; ix < x + w; ix++) {
            nds_drawPixel(ix, iy, color);
        }
    }
}

/* ---- Input ---- */
static uint32_t nds_currentKeys = 0;
static touchPosition nds_touch;
static BOOL nds_touching = FALSE;

void nds_pollInput(void)
{
    scanKeys();
    nds_currentKeys = keysHeld();

    if (nds_currentKeys & KEY_TOUCH) {
        touchRead(&nds_touch);
        nds_touching = TRUE;
    } else {
        nds_touching = FALSE;
    }
}

uint32_t nds_getKeys(void)
{
    return nds_currentKeys;
}

int16_t nds_getTouchX(void)
{
    return nds_touch.px;
}

int16_t nds_getTouchY(void)
{
    return nds_touch.py;
}

BOOL nds_isTouching(void)
{
    return nds_touching;
}

/* ---- Timer ---- */
void nds_initTimer(void)
{
    /* Use timer 0 for millisecond counting
       Bus clock = 33.514 MHz, divider 1024 = ~32.728 kHz
       We use an overflow interrupt approach */
    TIMER0_DATA = TIMER_FREQ_1024(1000);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024 | TIMER_IRQ_REQ;

    irqSet(IRQ_TIMER0, nds_timerIRQ);
    irqEnable(IRQ_TIMER0);
}

uint32_t nds_getTicks(void)
{
    return nds_tickCounter;
}

void nds_delay(uint32_t ms)
{
    uint32_t target = nds_tickCounter + ms;
    while (nds_tickCounter < target) {
        swiWaitForVBlank();
    }
}

/* ---- Audio Stubs ---- */
void nds_initAudio(void)
{
    /* TODO: implement using maxmod or custom mixing */
}

void nds_shutdownAudio(void)
{
}

void nds_playSound(int32_t soundnum)
{
    (void)soundnum;
}

void nds_playMusic(const char *filename)
{
    (void)filename;
}

void nds_stopMusic(void)
{
}

/* ---- File I/O ---- */
int32_t nds_initFilesystem(void)
{
    /* FAT filesystem not available in base devkitARM image.
       Files can be loaded via NitroFS or DLDI-patched later. */
    return 0;
}

int32_t nds_fileExists(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

/* ---- Console ---- */
void nds_initConsole(void)
{
    /* Sub engine for console on bottom screen */
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(&bottomConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&bottomConsole);
}

void nds_consolePrint(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    viprintf(fmt, args);
    va_end(args);
}

void nds_consoleClear(void)
{
    consoleClear();
}
