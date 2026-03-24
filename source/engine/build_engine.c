/*
 * Duke Nukem 3D - Nintendo DS Port
 * build_engine.c - Build Engine core implementation
 *
 * This is a simplified software renderer for the Build engine
 * adapted for NDS hardware constraints.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build_engine.h"
#include "nds_platform.h"

/* ---- Global Data ---- */
sectortype  sector[MAXSECTORS];
walltype    wall[MAXWALLS];
spritetype  sprite[MAXSPRITES];

int16_t     numsectors = 0;
int16_t     numwalls = 0;
int32_t     numsprites = 0;

int32_t     totalclock = 0;
int32_t     randomseed = 17;

uint8_t    *tileData[MAXTILES];
int16_t     tilesizx[MAXTILES];
int16_t     tilesizy[MAXTILES];

int32_t     posx = 0, posy = 0, posz = 0;
int16_t     ang = 0, horiz = 100, cursectnum = 0;

int32_t     sintable[2048];

/* ---- Rendering internals ---- */
static uint8_t palette[768];               /* 256 * RGB (6-bit each) */
static uint16_t nds_palette[256];          /* converted to NDS RGB15 */
static uint8_t palookup[32][256];          /* shade tables */

/* Column rendering buffers */
static int16_t umost[NDS_SCREEN_WIDTH];    /* upper-most Y for each column */
static int16_t dmost[NDS_SCREEN_WIDTH];    /* lower-most Y for each column */

/* Sector rendering order */
#define MAXSECTORSTORENDER 128
static int16_t sectorRenderList[MAXSECTORSTORENDER];
static int32_t numSectorsToRender;

/* ---- Trig Table Init ---- */
/* Integer sine approximation: no libm needed */
static int32_t isin(int32_t x)
{
    /* x in range 0..2047 (Build angle units, 2048 = 360 deg)
       Returns sine * 16384 (14-bit fixed point)
       Uses a parabolic approximation */
    int32_t quarter = 512; /* 2048/4 */
    int32_t half = 1024;
    int32_t val;

    x = x & 2047;

    /* Map to 0..511 range with sign */
    if (x < quarter) {
        /* 0..90 deg: rising */
        val = x;
    } else if (x < half) {
        /* 90..180 deg: falling */
        val = half - x;
    } else if (x < half + quarter) {
        /* 180..270 deg: falling negative */
        val = -(x - half);
    } else {
        /* 270..360 deg: rising to zero */
        val = -(2048 - x);
    }

    /* Parabolic approx: sin(x) ~= (4/pi)*x - (4/pi^2)*x^2 for 0..pi/2
       Scaled: val is 0..512 representing 0..90 degrees
       We want output in -16384..16384 */
    {
        /* Normalize val to -512..512 */
        /* Result = val * (2*512 - abs(val)) * 16384 / (512*512/2) */
        int32_t absval = val < 0 ? -val : val;
        int64_t result = (int64_t)val * (int64_t)(1024 - absval);
        /* Scale: at val=512, result = 512*512 = 262144, we want 16384 */
        /* 16384 / 262144 = 1/16 */
        result = (result + 8) >> 4;
        /* Clamp */
        if (result > 16384) result = 16384;
        if (result < -16384) result = -16384;
        return (int32_t)result;
    }
}

void engine_inittables(void)
{
    int32_t i;
    for (i = 0; i < 2048; i++) {
        sintable[i] = isin(i);
    }
}


/* ---- Random Number Generator ---- */
int32_t engine_krand(void)
{
    randomseed = ((randomseed * 1664525) + 1013904223);
    return (randomseed >> 16) & 0x7FFF;
}

/* ---- Engine Init/Uninit ---- */
int32_t engine_init(void)
{
    int32_t i;

    /* Clear all structures */
    memset(sector, 0, sizeof(sector));
    memset(wall, 0, sizeof(wall));
    memset(sprite, 0, sizeof(sprite));
    memset(tileData, 0, sizeof(tileData));
    memset(tilesizx, 0, sizeof(tilesizx));
    memset(tilesizy, 0, sizeof(tilesizy));

    /* Init trig tables */
    engine_inittables();

    /* Init column clipping */
    for (i = 0; i < NDS_SCREEN_WIDTH; i++) {
        umost[i] = 0;
        dmost[i] = NDS_SCREEN_HEIGHT - 1;
    }

    /* Generate default palette (grayscale) until real palette is loaded */
    for (i = 0; i < 256; i++) {
        palette[i * 3 + 0] = (uint8_t)(i >> 2);
        palette[i * 3 + 1] = (uint8_t)(i >> 2);
        palette[i * 3 + 2] = (uint8_t)(i >> 2);
        nds_palette[i] = nds_paletteConvert(
            palette[i * 3 + 0],
            palette[i * 3 + 1],
            palette[i * 3 + 2]
        );
    }

    /* Generate shade tables */
    for (i = 0; i < 32; i++) {
        int32_t j;
        for (j = 0; j < 256; j++) {
            int32_t shade = j - (i * 4);
            if (shade < 0) shade = 0;
            if (shade > 255) shade = 255;
            palookup[i][j] = (uint8_t)shade;
        }
    }

    return 0;
}

void engine_uninit(void)
{
    int32_t i;

    /* Free tile data */
    for (i = 0; i < MAXTILES; i++) {
        if (tileData[i]) {
            free(tileData[i]);
            tileData[i] = NULL;
        }
    }
}

/* ---- Board Loading ---- */
int32_t engine_loadboard(const char *filename, int32_t *daposx, int32_t *daposy,
                          int32_t *daposz, int16_t *daang, int16_t *dacursectnum)
{
    FILE *f;
    int32_t mapversion;
    int32_t i;

    f = fopen(filename, "rb");
    if (!f) {
        nds_consolePrint("Cannot open: %s\n", filename);
        return -1;
    }

    /* Read map version */
    fread(&mapversion, 4, 1, f);

    /* Read player start position */
    fread(daposx, 4, 1, f);
    fread(daposy, 4, 1, f);
    fread(daposz, 4, 1, f);
    fread(daang, 2, 1, f);
    fread(dacursectnum, 2, 1, f);

    /* Read sectors */
    fread(&numsectors, 2, 1, f);
    if (numsectors > MAXSECTORS) {
        nds_consolePrint("Too many sectors: %d\n", numsectors);
        fclose(f);
        return -1;
    }
    fread(sector, sizeof(sectortype), numsectors, f);

    /* Read walls */
    fread(&numwalls, 2, 1, f);
    if (numwalls > MAXWALLS) {
        nds_consolePrint("Too many walls: %d\n", numwalls);
        fclose(f);
        return -1;
    }
    fread(wall, sizeof(walltype), numwalls, f);

    /* Read sprites */
    {
        int16_t ns;
        fread(&ns, 2, 1, f);
        numsprites = ns;
    }
    if (numsprites > MAXSPRITES) {
        nds_consolePrint("Too many sprites: %d\n", (int)numsprites);
        fclose(f);
        return -1;
    }
    fread(sprite, sizeof(spritetype), numsprites, f);

    fclose(f);

    nds_consolePrint("Map loaded: %d sec, %d walls\n", numsectors, numwalls);

    return 0;
}

/* ---- ART/Tile Loading ---- */
int32_t engine_loadpics(const char *filename)
{
    FILE *f;
    int32_t artversion, numtiles_total;
    int32_t localtilestart, localtileend;
    int32_t i;

    f = fopen(filename, "rb");
    if (!f) {
        nds_consolePrint("Cannot open: %s\n", filename);
        return -1;
    }

    fread(&artversion, 4, 1, f);
    fread(&numtiles_total, 4, 1, f);
    fread(&localtilestart, 4, 1, f);
    fread(&localtileend, 4, 1, f);

    if (localtileend >= MAXTILES) localtileend = MAXTILES - 1;

    /* Read tile sizes */
    for (i = localtilestart; i <= localtileend; i++) {
        fread(&tilesizx[i], 2, 1, f);
    }
    for (i = localtilestart; i <= localtileend; i++) {
        fread(&tilesizy[i], 2, 1, f);
    }

    /* Skip picanm data (4 bytes per tile) */
    fseek(f, (localtileend - localtilestart + 1) * 4, SEEK_CUR);

    /* Read tile pixel data */
    for (i = localtilestart; i <= localtileend; i++) {
        int32_t tilesize = tilesizx[i] * tilesizy[i];
        if (tilesize > 0 && tilesize < 65536) {
            tileData[i] = (uint8_t *)malloc(tilesize);
            if (tileData[i]) {
                fread(tileData[i], 1, tilesize, f);
            } else {
                fseek(f, tilesize, SEEK_CUR);
            }
        } else {
            tileData[i] = NULL;
        }
    }

    fclose(f);

    nds_consolePrint("Art loaded: tiles %d-%d\n", localtilestart, localtileend);

    return 0;
}

/* ---- Palette Loading ---- */
int32_t engine_loadpalette(const char *filename)
{
    FILE *f;
    int32_t i;

    f = fopen(filename, "rb");
    if (!f) {
        nds_consolePrint("Cannot open: %s\n", filename);
        return -1;
    }

    fread(palette, 1, 768, f);

    /* Read shade tables if available */
    {
        int16_t numshades;
        fread(&numshades, 2, 1, f);
        if (numshades > 32) numshades = 32;
        fread(palookup, 256, numshades, f);
    }

    fclose(f);

    /* Convert to NDS palette */
    for (i = 0; i < 256; i++) {
        nds_palette[i] = nds_paletteConvert(
            palette[i * 3 + 0],
            palette[i * 3 + 1],
            palette[i * 3 + 2]
        );
    }

    nds_consolePrint("Palette loaded\n");
    return 0;
}

/* ---- Sector Updater ---- */
void engine_updatesector(int32_t x, int32_t y, int16_t *sectnum)
{
    int16_t i, j;
    walltype *wal;

    /* First check current sector */
    if (*sectnum >= 0 && *sectnum < numsectors) {
        /* Point-in-sector test */
        int32_t inside = 1;
        wal = &wall[sector[*sectnum].wallptr];
        j = sector[*sectnum].wallnum;

        /* Simple inside test using ray casting */
        int32_t cnt = 0;
        int16_t startwall = sector[*sectnum].wallptr;
        int16_t endwall = startwall + j - 1;

        for (i = startwall; i <= endwall; i++) {
            int32_t x1 = wall[i].x - x;
            int32_t y1 = wall[i].y - y;
            int32_t x2 = wall[wall[i].point2].x - x;
            int32_t y2 = wall[wall[i].point2].y - y;

            if ((y1 ^ y2) < 0) {
                int32_t t = (x1 ^ x2);
                if (t >= 0) cnt ^= t;
                else {
                    int64_t cross = (int64_t)x1 * y2 - (int64_t)x2 * y1;
                    cnt ^= (cross ^ y2);
                }
            }
        }

        if (cnt < 0) return; /* still in same sector */
    }

    /* Search all sectors */
    for (i = 0; i < numsectors; i++) {
        int32_t cnt = 0;
        int16_t startwall = sector[i].wallptr;
        int16_t endwall = startwall + sector[i].wallnum - 1;

        for (j = startwall; j <= endwall; j++) {
            int32_t x1 = wall[j].x - x;
            int32_t y1 = wall[j].y - y;
            int32_t x2 = wall[wall[j].point2].x - x;
            int32_t y2 = wall[wall[j].point2].y - y;

            if ((y1 ^ y2) < 0) {
                int32_t t = (x1 ^ x2);
                if (t >= 0) cnt ^= t;
                else {
                    int64_t cross = (int64_t)x1 * y2 - (int64_t)x2 * y1;
                    cnt ^= (cross ^ y2);
                }
            }
        }

        if (cnt < 0) {
            *sectnum = i;
            return;
        }
    }

    *sectnum = -1;
}

/* ---- Software Renderer ---- */

/* Transform a point from world space to view space */
static inline void engine_transformPoint(int32_t wx, int32_t wy,
                                         int32_t viewx, int32_t viewy, int16_t viewang,
                                         int32_t *sx, int32_t *sy)
{
    int32_t dx = wx - viewx;
    int32_t dy = wy - viewy;
    int32_t cosang = engine_cos(viewang);
    int32_t sinang = engine_sin(viewang);

    /* Rotate to view space */
    *sx = ((int64_t)dx * cosang + (int64_t)dy * sinang) >> 14;
    *sy = ((int64_t)dy * cosang - (int64_t)dx * sinang) >> 14;
}

/* Project a view-space point to screen coordinates */
static inline int32_t engine_projectX(int32_t vx, int32_t vy)
{
    if (vy <= 0) return -1;
    return (NDS_SCREEN_WIDTH / 2) + (((int64_t)vx * (NDS_SCREEN_WIDTH / 2)) / vy);
}

static inline int32_t engine_projectY(int32_t vz, int32_t vy, int32_t dahoriz)
{
    if (vy <= 0) return -1;
    return dahoriz + (((int64_t)vz * (NDS_SCREEN_WIDTH / 2)) / vy);
}

/* Draw a vertical column with a solid color */
static void engine_drawColumn(int32_t x, int32_t y1, int32_t y2, uint16_t color)
{
    int32_t y;
    if (x < 0 || x >= NDS_SCREEN_WIDTH) return;
    if (y1 < 0) y1 = 0;
    if (y2 >= NDS_SCREEN_HEIGHT) y2 = NDS_SCREEN_HEIGHT - 1;

    for (y = y1; y <= y2; y++) {
        nds_framebuffer[y * NDS_SCREEN_WIDTH + x] = color;
    }
}

/* Draw a textured vertical column */
static void engine_drawTexColumn(int32_t x, int32_t y1, int32_t y2,
                                  uint8_t *tex, int32_t texw, int32_t texh,
                                  int32_t texu, int32_t texv_start, int32_t texv_step,
                                  int32_t shade)
{
    int32_t y;
    int32_t texv;

    if (x < 0 || x >= NDS_SCREEN_WIDTH) return;
    if (!tex || texw <= 0 || texh <= 0) return;
    if (y1 < 0) {
        texv_start += texv_step * (-y1);
        y1 = 0;
    }
    if (y2 >= NDS_SCREEN_HEIGHT) y2 = NDS_SCREEN_HEIGHT - 1;
    if (shade < 0) shade = 0;
    if (shade > 31) shade = 31;

    texu = texu & (texw - 1);
    texv = texv_start;

    for (y = y1; y <= y2; y++) {
        int32_t tv = (texv >> 16) & (texh - 1);
        uint8_t pidx = tex[texu * texh + tv];
        pidx = palookup[shade][pidx];
        nds_framebuffer[y * NDS_SCREEN_WIDTH + x] = nds_palette[pidx];
        texv += texv_step;
    }
}

/* Draw wall span between two screen X positions */
static void engine_drawWallSpan(int32_t x1, int32_t x2,
                                 int32_t cy1a, int32_t cy1b,  /* ceil Y at x1, x2 */
                                 int32_t fy1a, int32_t fy1b,  /* floor Y at x1, x2 */
                                 int16_t picnum, int8_t shade,
                                 int32_t wallLength, int32_t dist1, int32_t dist2)
{
    int32_t x;
    int32_t dx = x2 - x1;
    if (dx <= 0) return;

    uint8_t *tex = NULL;
    int32_t texw = 0, texh = 0;

    if (picnum >= 0 && picnum < MAXTILES) {
        tex = tileData[picnum];
        texw = tilesizx[picnum];
        texh = tilesizy[picnum];
    }

    int32_t shadeidx = shade;
    if (shadeidx < 0) shadeidx = 0;
    if (shadeidx > 31) shadeidx = 31;

    for (x = x1; x <= x2; x++) {
        if (x < 0 || x >= NDS_SCREEN_WIDTH) continue;

        /* Interpolate Y positions */
        int32_t t = x - x1;
        int32_t cy = cy1a + (((cy1b - cy1a) * t) / dx);
        int32_t fy = fy1a + (((fy1b - fy1a) * t) / dx);

        /* Clamp */
        if (cy < umost[x]) cy = umost[x];
        if (fy > dmost[x]) fy = dmost[x];
        if (cy > fy) continue;

        /* Draw ceiling (solid color) */
        if (cy > umost[x]) {
            engine_drawColumn(x, umost[x], cy - 1, nds_palette[0]);
        }

        /* Draw wall column */
        if (tex && texw > 0 && texh > 0) {
            /* Calculate texture U from interpolated distance */
            int32_t texu = (t * wallLength / dx) & (texw - 1);
            int32_t texv_step = (texh << 16) / max(fy - cy + 1, 1);
            engine_drawTexColumn(x, cy, fy, tex, texw, texh,
                                texu, 0, texv_step, shadeidx);
        } else {
            /* Fallback: colored wall */
            uint16_t wallColor = nds_rgb15(128, 128, 128);
            engine_drawColumn(x, cy, fy, wallColor);
        }

        /* Draw floor (solid color) */
        if (fy < dmost[x]) {
            engine_drawColumn(x, fy + 1, dmost[x], nds_palette[16]);
        }
    }
}

/* ---- Main Render Function ---- */
void engine_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz,
                       int16_t daang, int32_t dahoriz, int16_t dacursectnum)
{
    int32_t i, j;
    int32_t sx1, sy1, sx2, sy2;

    if (dacursectnum < 0 || dacursectnum >= numsectors) return;

    /* Reset column clipping */
    for (i = 0; i < NDS_SCREEN_WIDTH; i++) {
        umost[i] = 0;
        dmost[i] = NDS_SCREEN_HEIGHT - 1;
    }

    /* Clear screen */
    nds_clearScreen(0);

    /* Simple front-to-back sector rendering via BFS/portal traversal */
    static uint8_t sectorVisited[MAXSECTORS];
    memset(sectorVisited, 0, sizeof(sectorVisited));

    numSectorsToRender = 0;
    sectorRenderList[numSectorsToRender++] = dacursectnum;
    sectorVisited[dacursectnum] = 1;

    int32_t queuePos = 0;

    while (queuePos < numSectorsToRender && numSectorsToRender < MAXSECTORSTORENDER) {
        int16_t sectnum = sectorRenderList[queuePos++];
        sectortype *sec = &sector[sectnum];
        int16_t startwall = sec->wallptr;
        int16_t endwall = startwall + sec->wallnum - 1;

        for (j = startwall; j <= endwall; j++) {
            walltype *wal = &wall[j];

            /* Transform wall endpoints to view space */
            int32_t vx1, vy1, vx2, vy2;
            engine_transformPoint(wal->x, wal->y, daposx, daposy, daang, &vx1, &vy1);
            engine_transformPoint(wall[wal->point2].x, wall[wal->point2].y,
                                 daposx, daposy, daang, &vx2, &vy2);

            /* Backface culling */
            if (vy1 <= 0 && vy2 <= 0) continue;

            /* Simple near-plane clip */
            if (vy1 <= 0) {
                int32_t t = (1 - vy1);
                int32_t d = (vy2 - vy1);
                if (d != 0) {
                    vx1 = vx1 + (int32_t)(((int64_t)(vx2 - vx1) * t) / d);
                    vy1 = 1;
                }
            }
            if (vy2 <= 0) {
                int32_t t = (1 - vy2);
                int32_t d = (vy1 - vy2);
                if (d != 0) {
                    vx2 = vx2 + (int32_t)(((int64_t)(vx1 - vx2) * t) / d);
                    vy2 = 1;
                }
            }

            /* Project to screen X */
            int32_t screenX1 = engine_projectX(vx1, vy1);
            int32_t screenX2 = engine_projectX(vx2, vy2);

            if (screenX1 >= screenX2) continue;
            if (screenX2 < 0 || screenX1 >= NDS_SCREEN_WIDTH) continue;

            /* Compute screen Y for ceiling and floor at both endpoints */
            int32_t ceilZ = sec->ceilingz - daposz;
            int32_t floorZ = sec->floorz - daposz;

            int32_t cy1 = engine_projectY(-ceilZ, vy1, dahoriz);
            int32_t cy2 = engine_projectY(-ceilZ, vy2, dahoriz);
            int32_t fy1 = engine_projectY(-floorZ, vy1, dahoriz);
            int32_t fy2 = engine_projectY(-floorZ, vy2, dahoriz);

            /* Calculate wall length for texture mapping */
            int32_t wdx = wall[wal->point2].x - wal->x;
            int32_t wdy = wall[wal->point2].y - wal->y;
            int32_t wallLen = 1;
            {
                int64_t wlsq = (int64_t)wdx * wdx + (int64_t)wdy * wdy;
                /* Integer sqrt approximation */
                if (wlsq > 0) {
                    wallLen = (int32_t)(wdx > 0 ? wdx : -wdx) + (int32_t)(wdy > 0 ? wdy : -wdy);
                    wallLen = wallLen >> 4;
                    if (wallLen < 1) wallLen = 1;
                }
            }

            if (wal->nextsector >= 0 && wal->nextsector < numsectors) {
                /* Portal wall: draw upper and lower portions, then add adjacent sector */
                sectortype *nextsec = &sector[wal->nextsector];
                int32_t nextCeilZ = nextsec->ceilingz - daposz;
                int32_t nextFloorZ = nextsec->floorz - daposz;

                int32_t ncy1 = engine_projectY(-nextCeilZ, vy1, dahoriz);
                int32_t ncy2 = engine_projectY(-nextCeilZ, vy2, dahoriz);
                int32_t nfy1 = engine_projectY(-nextFloorZ, vy1, dahoriz);
                int32_t nfy2 = engine_projectY(-nextFloorZ, vy2, dahoriz);

                /* Draw upper wall (if next sector ceiling is lower) */
                if (nextCeilZ > ceilZ) {
                    engine_drawWallSpan(screenX1, screenX2,
                                        cy1, cy2, ncy1, ncy2,
                                        wal->picnum, wal->shade,
                                        wallLen, vy1, vy2);
                }

                /* Draw lower wall (if next sector floor is higher) */
                if (nextFloorZ < floorZ) {
                    engine_drawWallSpan(screenX1, screenX2,
                                        nfy1, nfy2, fy1, fy2,
                                        wal->picnum, wal->shade,
                                        wallLen, vy1, vy2);
                }

                /* Add next sector to render queue */
                if (!sectorVisited[wal->nextsector]) {
                    sectorVisited[wal->nextsector] = 1;
                    if (numSectorsToRender < MAXSECTORSTORENDER) {
                        sectorRenderList[numSectorsToRender++] = wal->nextsector;
                    }
                }
            } else {
                /* Solid wall: draw the full wall span */
                engine_drawWallSpan(screenX1, screenX2,
                                    cy1, cy2, fy1, fy2,
                                    wal->picnum, wal->shade,
                                    wallLen, vy1, vy2);
            }
        }
    }
}

void engine_drawmasks(void)
{
    /* TODO: Draw masked walls and sprites */
    /* For now, this is a stub - sprite rendering will be added later */
}

void engine_nextpage(void)
{
    nds_flipScreen();
    totalclock++;
}
