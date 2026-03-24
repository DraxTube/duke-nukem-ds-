/*
 * Duke Nukem 3D - Nintendo DS Port
 * main.c - Entry point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nds.h>
#include "duke3d.h"

/* ---- Title Screen ---- */
static void drawTitleScreen(void)
{
    int32_t x, y;
    uint16_t bgColor;

    /* Draw a simple Duke3D-style title screen */
    for (y = 0; y < NDS_SCREEN_HEIGHT; y++) {
        for (x = 0; x < NDS_SCREEN_WIDTH; x++) {
            /* Dark gradient background */
            int32_t shade = y / 3;
            bgColor = nds_rgb15(shade / 4, 0, shade / 2);
            nds_framebuffer[y * NDS_SCREEN_WIDTH + x] = bgColor;
        }
    }

    /* Draw "DUKE3D" text using pixel art */
    uint16_t textColor = nds_rgb15(255, 200, 0);

    /* "D" */
    nds_fillRect(40, 60, 4, 24, textColor);
    nds_fillRect(44, 60, 8, 4, textColor);
    nds_fillRect(52, 64, 4, 16, textColor);
    nds_fillRect(44, 80, 8, 4, textColor);

    /* "U" */
    nds_fillRect(64, 60, 4, 24, textColor);
    nds_fillRect(76, 60, 4, 24, textColor);
    nds_fillRect(68, 80, 8, 4, textColor);

    /* "K" */
    nds_fillRect(88, 60, 4, 24, textColor);
    nds_fillRect(92, 70, 4, 4, textColor);
    nds_fillRect(96, 66, 4, 4, textColor);
    nds_fillRect(100, 60, 4, 6, textColor);
    nds_fillRect(96, 74, 4, 4, textColor);
    nds_fillRect(100, 78, 4, 6, textColor);

    /* "E" */
    nds_fillRect(112, 60, 4, 24, textColor);
    nds_fillRect(116, 60, 8, 4, textColor);
    nds_fillRect(116, 70, 6, 4, textColor);
    nds_fillRect(116, 80, 8, 4, textColor);

    /* "3" */
    nds_fillRect(136, 60, 12, 4, textColor);
    nds_fillRect(144, 64, 4, 8, textColor);
    nds_fillRect(136, 70, 12, 4, textColor);
    nds_fillRect(144, 74, 4, 8, textColor);
    nds_fillRect(136, 80, 12, 4, textColor);

    /* "D" */
    nds_fillRect(160, 60, 4, 24, textColor);
    nds_fillRect(164, 60, 8, 4, textColor);
    nds_fillRect(172, 64, 4, 16, textColor);
    nds_fillRect(164, 80, 8, 4, textColor);

    /* Subtitle line */
    uint16_t subColor = nds_rgb15(180, 180, 180);
    nds_fillRect(60, 100, 136, 2, subColor);

    /* "NDS PORT" small indicator */
    nds_fillRect(90, 110, 76, 2, nds_rgb15(0, 200, 0));

    /* "PRESS START" blinking */
    if ((totalclock / 30) & 1) {
        nds_fillRect(80, 150, 96, 3, nds_rgb15(255, 255, 255));
    }

    nds_flipScreen();
}

/* ---- Demo Mode: procedural level ---- */
static void generateDemoLevel(void)
{
    /*
     * Since we can't include copyrighted Duke3D map files,
     * generate a small demo level to show the engine works.
     */
    int32_t i;

    numsectors = 0;
    numwalls = 0;
    numsprites = 0;

    /* ---- Room 1: Main room (square) ---- */
    {
        int16_t firstwall = numwalls;
        int32_t size = 8192;

        /* Wall 0: south */
        wall[numwalls].x = -size; wall[numwalls].y = -size;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 0;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 1: east */
        wall[numwalls].x = size; wall[numwalls].y = -size;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 4;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 2: north - portal to room 2 */
        wall[numwalls].x = size; wall[numwalls].y = size;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = 8;  /* connects to room 2, wall 8 */
        wall[numwalls].nextsector = 1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 8;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 3: west */
        wall[numwalls].x = -size; wall[numwalls].y = size;
        wall[numwalls].point2 = firstwall;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 12;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Sector 0 */
        sector[numsectors].wallptr = firstwall;
        sector[numsectors].wallnum = 4;
        sector[numsectors].ceilingz = -(32 << 8);
        sector[numsectors].floorz = (32 << 8);
        sector[numsectors].ceilingpicnum = 0;
        sector[numsectors].floorpicnum = 0;
        sector[numsectors].ceilingshade = 0;
        sector[numsectors].floorshade = 0;
        sector[numsectors].visibility = 0;
        numsectors++;
    }

    /* ---- Room 2: Corridor (connected to room 1 north wall) ---- */
    {
        int16_t firstwall = numwalls;
        int32_t size = 8192;
        int32_t depth = 16384;

        /* Wall 4: west */
        wall[numwalls].x = -size; wall[numwalls].y = size;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 6;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 5: north */
        wall[numwalls].x = -size; wall[numwalls].y = size + depth;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 2;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 6: east-north */
        wall[numwalls].x = size; wall[numwalls].y = size + depth;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 10;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 7: east-south */
        wall[numwalls].x = size; wall[numwalls].y = size;
        wall[numwalls].point2 = numwalls + 1;
        wall[numwalls].nextwall = -1;
        wall[numwalls].nextsector = -1;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 14;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Wall 8: south (portal back to room 1) */
        wall[numwalls].x = size; wall[numwalls].y = size;
        wall[numwalls].point2 = firstwall;
        wall[numwalls].nextwall = 2; /* connects back to room 1, wall 2 */
        wall[numwalls].nextsector = 0;
        wall[numwalls].picnum = 0;
        wall[numwalls].shade = 8;
        wall[numwalls].xrepeat = 8; wall[numwalls].yrepeat = 8;
        numwalls++;

        /* Sector 1 */
        sector[numsectors].wallptr = firstwall;
        sector[numsectors].wallnum = 5;
        sector[numsectors].ceilingz = -(48 << 8);  /* higher ceiling */
        sector[numsectors].floorz = (32 << 8);
        sector[numsectors].ceilingpicnum = 0;
        sector[numsectors].floorpicnum = 0;
        sector[numsectors].ceilingshade = 4;
        sector[numsectors].floorshade = 2;
        sector[numsectors].visibility = 0;
        numsectors++;
    }

    /* Set player start position */
    player[0].posx = 0;
    player[0].posy = 0;
    player[0].posz = 0;
    player[0].ang = 512; /* facing north */
    player[0].cursectnum = 0;

    posx = player[0].posx;
    posy = player[0].posy;
    posz = player[0].posz;
    ang = player[0].ang;
    cursectnum = player[0].cursectnum;

    nds_consolePrint("Demo level generated\n");
    nds_consolePrint("%d sectors, %d walls\n", numsectors, numwalls);
}

/* ---- Main Entry Point ---- */
int main(void)
{
    int32_t titleTimer = 0;
    BOOL inTitle = TRUE;

    /* Initialize NDS hardware */
    if (!nds_init()) {
        return 1;
    }

    nds_consolePrint("Duke Nukem 3D NDS v0.1\n");
    nds_consolePrint("----------------------\n\n");

    /* Initialize Build engine */
    if (engine_init() != 0) {
        nds_consolePrint("Engine init failed!\n");
        while (1) swiWaitForVBlank();
    }
    nds_consolePrint("Engine initialized OK\n");

    /* Initialize game */
    game_init();

    /* Try to load game data files */
    {
        int32_t hasData = 0;

        /* Try loading palette */
        if (nds_fileExists("palette.dat")) {
            nds_consolePrint("Loading palette...\n");
            engine_loadpalette("palette.dat");
            hasData = 1;
        }

        /* Try loading art */
        if (nds_fileExists("tiles000.art")) {
            nds_consolePrint("Loading tiles...\n");
            engine_loadpics("tiles000.art");
            hasData = 1;
        }

        /* Try loading first level */
        if (nds_fileExists("e1l1.map")) {
            nds_consolePrint("Found game data!\n");
            game_loadLevel(0, 0);
        } else {
            nds_consolePrint("No game data found.\n");
            nds_consolePrint("Generating demo level...\n");
            generateDemoLevel();
        }
    }

    nds_consolePrint("\nPress START to play!\n");
    nds_consolePrint("D-Pad: Move/Turn\n");
    nds_consolePrint("L/R: Strafe\n");
    nds_consolePrint("Touch: Look\n");

    /* ---- Title Screen Loop ---- */
    while (inTitle) {
        drawTitleScreen();
        titleTimer++;

        nds_pollInput();
        uint32_t keys = nds_getKeys();

        if (keys & KEY_START) {
            inTitle = FALSE;
        }

        /* Auto-start after a while */
        if (titleTimer > 300) {
            inTitle = FALSE;
        }
    }

    nds_consoleClear();
    nds_consolePrint("Game running!\n");
    nds_consolePrint("FPS: --\n");

    /* ---- Main Game Loop ---- */
    uint32_t lastTick = nds_getTicks();
    uint32_t frameCount = 0;

    while (game_running) {
        game_loop();
        frameCount++;

        /* FPS counter on console every second */
        uint32_t now = nds_getTicks();
        if (now - lastTick >= 1000) {
            nds_consoleClear();
            nds_consolePrint("FPS: %d\n", (int)frameCount);
            nds_consolePrint("Pos: %d,%d\n",
                (int)(posx >> 4), (int)(posy >> 4));
            nds_consolePrint("Ang: %d Sec: %d\n",
                (int)ang, (int)cursectnum);
            frameCount = 0;
            lastTick = now;
        }

        /* Check for exit (START+SELECT) */
        {
            uint32_t keys = nds_getKeys();
            if ((keys & KEY_START) && (keys & KEY_SELECT)) {
                game_running = 0;
            }
        }
    }

    /* Cleanup */
    game_uninit();
    engine_uninit();
    nds_shutdown();

    return 0;
}
