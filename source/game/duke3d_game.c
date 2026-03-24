/*
 * Duke Nukem 3D - Nintendo DS Port
 * duke3d_game.c - Game logic implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "duke3d.h"

/* ---- Game Globals ---- */
player_t    player[MAXPLAYERS];
int32_t     myconnectindex = 0;
int32_t     numplayers = 1;

int32_t     current_episode = 0;
int32_t     current_level = 0;
int32_t     game_running = 1;

char        boardfilename[256] = "";
char        levelname[256] = "";

/* ---- Movement Constants ---- */
#define PLAYER_MOVESPEED    2048
#define PLAYER_TURNSPEED    32
#define PLAYER_JUMPVEL      (-6 << 8)
#define PLAYER_GRAVITY      (1 << 5)
#define PLAYER_EYEHEIGHT    (32 << 8)

/* Level filenames (E1L1 format) */
static const char *levelFiles[MAXVOLUMES][MAXLEVELS] = {
    { /* Episode 1 */
        "e1l1.map", "e1l2.map", "e1l3.map", "e1l4.map", "e1l5.map",
        "e1l6.map", "e1l7.map", "e1l8.map", NULL, NULL, NULL
    },
    { /* Episode 2 */
        "e2l1.map", "e2l2.map", "e2l3.map", "e2l4.map", "e2l5.map",
        "e2l6.map", "e2l7.map", "e2l8.map", NULL, NULL, NULL
    },
    { /* Episode 3 */
        "e3l1.map", "e3l2.map", "e3l3.map", "e3l4.map", "e3l5.map",
        "e3l6.map", "e3l7.map", "e3l8.map", NULL, NULL, NULL
    },
    { /* Episode 4 */
        "e4l1.map", "e4l2.map", "e4l3.map", "e4l4.map", "e4l5.map",
        "e4l6.map", "e4l7.map", "e4l8.map", NULL, NULL, NULL
    },
};

/* ---- Init / Uninit ---- */
int32_t game_init(void)
{
    int32_t i;

    /* Clear player data */
    memset(player, 0, sizeof(player));

    /* Initialize player defaults */
    player[0].health = 100;
    player[0].armor = 0;
    player[0].curr_weapon = 0;
    player[0].on_ground = 1;
    player[0].horiz = 100;

    /* Give pistol by default */
    player[0].gotweapon[0] = 1; /* mighty foot */
    player[0].gotweapon[1] = 1; /* pistol */
    player[0].ammo[1] = 48;     /* pistol ammo */

    for (i = 0; i < 4; i++)
        player[0].keys[i] = 0;

    return 0;
}

void game_uninit(void)
{
    /* Nothing to clean up for now */
}

/* ---- Level Loading ---- */
void game_loadLevel(int32_t episode, int32_t level)
{
    int32_t dposx, dposy, dposz;
    int16_t dang, dcursectnum;

    if (episode < 0 || episode >= MAXVOLUMES) episode = 0;
    if (level < 0 || level >= MAXLEVELS) level = 0;
    if (levelFiles[episode][level] == NULL) {
        nds_consolePrint("Level not found!\n");
        return;
    }

    current_episode = episode;
    current_level = level;
    snprintf(boardfilename, sizeof(boardfilename), "%s", levelFiles[episode][level]);

    nds_consolePrint("Loading %s...\n", boardfilename);

    if (engine_loadboard(boardfilename, &dposx, &dposy, &dposz, &dang, &dcursectnum) != 0) {
        nds_consolePrint("Failed to load map!\n");
        return;
    }

    /* Set player position from map start */
    player[0].posx = dposx;
    player[0].posy = dposy;
    player[0].posz = dposz;
    player[0].ang = dang;
    player[0].cursectnum = dcursectnum;

    /* Set global camera */
    posx = dposx;
    posy = dposy;
    posz = dposz;
    ang = dang;
    cursectnum = dcursectnum;

    nds_consolePrint("Level loaded OK!\n");
}

/* ---- Input Processing ---- */
void game_processInput(void)
{
    uint32_t keys;
    player_t *p = &player[myconnectindex];

    nds_pollInput();
    keys = nds_getKeys();

    /* Reset input */
    p->input_fvel = 0;
    p->input_svel = 0;
    p->input_angvel = 0;
    p->input_bits = 0;

    /* D-Pad: movement */
    if (keys & KEY_UP) {
        p->input_fvel = PLAYER_MOVESPEED;
    }
    if (keys & KEY_DOWN) {
        p->input_fvel = -PLAYER_MOVESPEED;
    }

    /* L/R shoulder: strafe */
    if (keys & KEY_L) {
        p->input_svel = -PLAYER_MOVESPEED;
    }
    if (keys & KEY_R) {
        p->input_svel = PLAYER_MOVESPEED;
    }

    /* Left/Right: turning */
    if (keys & KEY_LEFT) {
        p->input_angvel = -PLAYER_TURNSPEED;
    }
    if (keys & KEY_RIGHT) {
        p->input_angvel = PLAYER_TURNSPEED;
    }

    /* A button: fire/action */
    if (keys & KEY_A) {
        p->input_bits |= 1;
    }

    /* B button: jump */
    if (keys & KEY_B) {
        p->input_bits |= 2;
    }

    /* X button: open/use */
    if (keys & KEY_X) {
        p->input_bits |= 4;
    }

    /* Y button: weapon switch */
    if (keys & KEY_Y) {
        p->input_bits |= 8;
    }

    /* Touch screen: look around */
    if (nds_isTouching()) {
        int16_t tx = nds_getTouchX();
        int16_t ty = nds_getTouchY();

        /* Horizontal touch = turn */
        if (tx < NDS_SCREEN_WIDTH / 3) {
            p->input_angvel = -PLAYER_TURNSPEED * 2;
        } else if (tx > (NDS_SCREEN_WIDTH * 2) / 3) {
            p->input_angvel = PLAYER_TURNSPEED * 2;
        }

        /* Vertical touch = look up/down */
        if (ty < NDS_SCREEN_HEIGHT / 3) {
            p->horizvel = 4;
        } else if (ty > (NDS_SCREEN_HEIGHT * 2) / 3) {
            p->horizvel = -4;
        }
    }

    /* START: pause / menu */
    if (keys & KEY_START) {
        p->input_bits |= 16;
    }

    /* SELECT: map */
    if (keys & KEY_SELECT) {
        p->input_bits |= 32;
    }
}

/* ---- Player Movement ---- */
void game_movePlayer(void)
{
    player_t *p = &player[myconnectindex];
    int32_t cosang, sinang;

    /* Apply turning */
    p->ang = (p->ang + p->input_angvel) & 2047;

    /* Apply look up/down */
    p->horiz += p->horizvel;
    if (p->horiz < 25) p->horiz = 25;
    if (p->horiz > 275) p->horiz = 275;
    p->horizvel = 0;

    /* Compute movement vectors */
    cosang = engine_cos(p->ang);
    sinang = engine_sin(p->ang);

    /* Forward/backward movement */
    if (p->input_fvel != 0) {
        p->posx += (int32_t)(((int64_t)p->input_fvel * cosang) >> 14);
        p->posy += (int32_t)(((int64_t)p->input_fvel * sinang) >> 14);
    }

    /* Strafing */
    if (p->input_svel != 0) {
        p->posx += (int32_t)(((int64_t)p->input_svel * sinang) >> 14);
        p->posy -= (int32_t)(((int64_t)p->input_svel * cosang) >> 14);
    }

    /* Update sector */
    engine_updatesector(p->posx, p->posy, &p->cursectnum);

    /* Apply gravity / floor alignment */
    if (p->cursectnum >= 0) {
        int32_t floorz = sector[p->cursectnum].floorz;
        int32_t ceilz = sector[p->cursectnum].ceilingz;
        int32_t eyepos = floorz - PLAYER_EYEHEIGHT;

        if (p->posz > eyepos) {
            p->posz = eyepos;
            p->on_ground = 1;
        }
        if (p->posz < ceilz + (4 << 8)) {
            p->posz = ceilz + (4 << 8);
        }
    }

    /* Jump */
    if ((p->input_bits & 2) && p->on_ground) {
        p->jumping_flag = 1;
        p->on_ground = 0;
        p->velx = 0; /* reusing as vertical velocity */
        p->vely = PLAYER_JUMPVEL;
    }

    if (p->jumping_flag) {
        p->posz += p->vely;
        p->vely += PLAYER_GRAVITY;

        if (p->cursectnum >= 0) {
            int32_t floorz = sector[p->cursectnum].floorz;
            if (p->posz >= floorz - PLAYER_EYEHEIGHT) {
                p->posz = floorz - PLAYER_EYEHEIGHT;
                p->jumping_flag = 0;
                p->on_ground = 1;
            }
        }
    }

    /* Sync global camera */
    posx = p->posx;
    posy = p->posy;
    posz = p->posz;
    ang = p->ang;
    horiz = p->horiz;
    cursectnum = p->cursectnum;
}

/* ---- HUD Drawing ---- */
void game_drawHUD(void)
{
    player_t *p = &player[myconnectindex];

    /* Simple HUD bar at bottom of screen */
    uint16_t hudColor = nds_rgb15(32, 32, 48);
    nds_fillRect(0, NDS_SCREEN_HEIGHT - 16, NDS_SCREEN_WIDTH, 16, hudColor);

    /* Health indicator (red bar) */
    {
        int32_t hw = (p->health * 60) / 100;
        if (hw < 0) hw = 0;
        if (hw > 60) hw = 60;

        uint16_t healthColor = nds_rgb15(200, 32, 32);
        nds_fillRect(4, NDS_SCREEN_HEIGHT - 12, hw, 8, healthColor);

        /* Border */
        uint16_t borderColor = nds_rgb15(255, 255, 255);
        nds_drawRect(3, NDS_SCREEN_HEIGHT - 13, 62, 10, borderColor);
    }

    /* Ammo indicator (yellow bar) */
    {
        int32_t ammo = p->ammo[p->curr_weapon];
        int32_t aw = (ammo > 200) ? 60 : (ammo * 60) / 200;
        if (aw < 0) aw = 0;

        uint16_t ammoColor = nds_rgb15(200, 200, 32);
        nds_fillRect(NDS_SCREEN_WIDTH - 64, NDS_SCREEN_HEIGHT - 12, aw, 8, ammoColor);

        uint16_t borderColor = nds_rgb15(255, 255, 255);
        nds_drawRect(NDS_SCREEN_WIDTH - 65, NDS_SCREEN_HEIGHT - 13, 62, 10, borderColor);
    }

    /* Key indicators */
    {
        int32_t kx = 80;
        if (p->keys[0]) { /* blue */
            nds_fillRect(kx, NDS_SCREEN_HEIGHT - 12, 8, 8, nds_rgb15(32, 32, 255));
        }
        kx += 12;
        if (p->keys[1]) { /* red */
            nds_fillRect(kx, NDS_SCREEN_HEIGHT - 12, 8, 8, nds_rgb15(255, 32, 32));
        }
        kx += 12;
        if (p->keys[2]) { /* yellow */
            nds_fillRect(kx, NDS_SCREEN_HEIGHT - 12, 8, 8, nds_rgb15(255, 255, 32));
        }
    }
}

/* ---- Main Game Loop (one frame) ---- */
void game_loop(void)
{
    game_processInput();
    game_movePlayer();

    /* Render the 3D view */
    engine_drawrooms(posx, posy, posz, ang, horiz, cursectnum);
    engine_drawmasks();

    /* Draw HUD overlay */
    game_drawHUD();

    /* Flip */
    engine_nextpage();
}

/* ---- Timer ---- */
uint32_t game_getTicks(void)
{
    return nds_getTicks();
}
