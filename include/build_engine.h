/*
 * Duke Nukem 3D - Nintendo DS Port
 * build_engine.h - Build Engine core definitions
 */

#ifndef BUILD_ENGINE_H
#define BUILD_ENGINE_H

#include "types.h"

/* ---- Build Engine Constants ---- */
#define MAXSECTORS      512
#define MAXWALLS        4096
#define MAXSPRITES      2048
#define MAXTILES        4096
#define MAXSTATUS       1024
#define MAXPLAYERS      1
#define MAXPSKYTILES    256

/* NDS screen dimensions */
#define XDIM            256
#define YDIM            192

/* Build engine angle units: 2048 = 360 degrees */
#define ANGLESCALE      2048

/* ---- Build Engine Structures ---- */

typedef struct {
    int32_t x, y;
} vec2_t;

typedef struct {
    int32_t x, y, z;
} vec3_t;

typedef struct {
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    int16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t  ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t  floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, filler;
    int16_t lotag, hitag, extra;
} sectortype;

typedef struct {
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    int16_t cstat;
    int16_t picnum, overpicnum;
    int8_t  shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} walltype;

typedef struct {
    int32_t x, y, z;
    int16_t cstat;
    int16_t picnum;
    int8_t  shade;
    uint8_t pal, clipdist, filler;
    uint8_t xrepeat, yrepeat;
    int8_t  xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner, xvel, yvel, zvel;
    int16_t lotag, hitag, extra;
} spritetype;

/* ---- Build Engine Globals ---- */
extern sectortype  sector[MAXSECTORS];
extern walltype    wall[MAXWALLS];
extern spritetype  sprite[MAXSPRITES];

extern int16_t     numsectors, numwalls;
extern int32_t     numsprites;

extern int32_t     totalclock;
extern int32_t     randomseed;

/* Tile data */
extern uint8_t    *tileData[MAXTILES];
extern int16_t     tilesizx[MAXTILES];
extern int16_t     tilesizy[MAXTILES];

/* Player/camera position */
extern int32_t     posx, posy, posz;
extern int16_t     ang, horiz, cursectnum;

/* ---- Build Engine Functions ---- */
int32_t  engine_init(void);
void     engine_uninit(void);
int32_t  engine_loadboard(const char *filename, int32_t *daposx, int32_t *daposy,
                          int32_t *daposz, int16_t *daang, int16_t *dacursectnum);
void     engine_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,
                          int16_t daang, int32_t dahoriz, int16_t dacursectnum);
void     engine_drawmasks(void);
void     engine_nextpage(void);

int32_t  engine_krand(void);
void     engine_updatesector(int32_t x, int32_t y, int16_t *sectnum);

/* Trigonometry tables */
extern int32_t     sintable[2048];
void     engine_inittables(void);

/* ---- Tile/Art loading ---- */
int32_t  engine_loadpics(const char *filename);

#endif /* BUILD_ENGINE_H */
