/*
 * Duke Nukem 3D - Nintendo DS Port
 * duke3d.h - Game-specific definitions
 */

#ifndef DUKE3D_H
#define DUKE3D_H

#include "types.h"
#include "build_engine.h"
#include "nds_platform.h"

/* ---- Game Constants ---- */
#define GAMEVERSION     19
#define BYTEVERSION     27

#define RECSYNCBUFSIZ   2048
#define MOVEFIFOSIZ     2048

#define MAXGAMEVARS     512
#define MAXVOLUMES      4
#define MAXLEVELS       11

#define MAXQUOTES       16384

/* Weapon constants */
#define MAX_WEAPONS     12

/* Game flags */
#define GAMEFLAG_DUKE3D  0x00000001

/* ---- Player Structure ---- */
typedef struct {
    /* Position */
    int32_t     posx, posy, posz;
    int16_t     ang, horiz;
    int16_t     cursectnum;

    /* Movement */
    int32_t     velx, vely;
    int16_t     angvel;
    int16_t     horizvel;

    /* Status */
    int16_t     health;
    int16_t     armor;
    int16_t     curr_weapon;
    int16_t     ammo[MAX_WEAPONS];
    int16_t     gotweapon[MAX_WEAPONS];

    /* Inventory */
    int16_t     keys[4]; /* blue, red, yellow, green */

    /* State flags */
    int16_t     jumping_flag;
    int16_t     on_ground;
    int16_t     dead_flag;
    int16_t     kickback_pic;

    /* View bobbing */
    int32_t     bobposx, bobposy;
    int32_t     pyoff;

    /* Misc */
    int16_t     look_ang;
    int16_t     rotscrnang;
    int32_t     newowner;

    /* Input */
    uint32_t    input_bits;
    int16_t     input_fvel;
    int16_t     input_svel;
    int16_t     input_angvel;

} player_t;

/* ---- Game Globals ---- */
extern player_t    player[MAXPLAYERS];
extern int32_t     myconnectindex;
extern int32_t     numplayers;

extern int32_t     current_episode;
extern int32_t     current_level;
extern int32_t     game_running;

extern char        boardfilename[256];
extern char        levelname[256];

/* ---- Game Functions ---- */
int32_t  game_init(void);
void     game_uninit(void);
void     game_loop(void);
void     game_processInput(void);
void     game_movePlayer(void);
void     game_drawHUD(void);
void     game_loadLevel(int32_t episode, int32_t level);

/* ---- Timer ---- */
uint32_t game_getTicks(void);

#endif /* DUKE3D_H */
