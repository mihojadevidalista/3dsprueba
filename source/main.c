// Mi Juego 3DS - homebrew para Nintendo 3DS
// Generado con Edit3DS
// Controles: Cruceta o Stick = mover | A = saltar | B (mantener) = correr | START = salir

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SW 400
#define SH 240
#define T 16
#define COLS 150
#define ROWS 15
#define GOAL_COL 140

#define PW 10
#define PH 24

#define WALK_SPEED 2.00f
#define RUN_SPEED  3.50f
#define GRAVITY    0.40f
#define JUMP_SPEED 7.50f
#define MAX_FALL   8.00f

typedef struct {
	float x, y, vx, vy;
	int onGround;
	int facing;
	float phase;
} Player;

unsigned char g_map[ROWS][COLS];
Player g_player;
int g_wins = 0;
int g_deaths = 0;

static u8* fb;

static void putRect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > SW) w = SW - x;
	if (y + h > SH) h = SH - y;
	if (w <= 0 || h <= 0) return;

	for (int i = x; i < x + w; i++) {
		u8* p = fb + (i * SH + (SH - 1 - y)) * 3;
		for (int j = 0; j < h; j++) {
			p[0] = b;
			p[1] = g;
			p[2] = r;
			p -= 3;
		}
	}
}

static void putLine(int x0, int y0, int x1, int y1, int th, u8 r, u8 g, u8 b)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	for (;;) {
		putRect(x0, y0, th, th, r, g, b);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

static void drawTile(int sx, int sy)
{
	putRect(sx, sy, T, T, 15, 145, 13);
	putRect(sx, sy, T, 1, 123, 195, 122);
	putRect(sx, sy + T - 1, T, 1, 7, 65, 6);
	putRect(sx + T - 1, sy, 1, T, 7, 65, 6);
	putRect(sx + 7, sy + 4, 1, 8, 11, 102, 9);
}

static void drawCloud(int x, int y)
{
	putRect(x + 8, y, 24, 8, 255, 255, 255);
	putRect(x, y + 8, 48, 10, 255, 255, 255);
	putRect(x + 6, y + 18, 36, 4, 235, 245, 255);
}

static void drawPlayer(int sx, int sy, const Player* p)
{
	const u8 cr = 20, cg = 20, cb = 20;
	int cx = sx + 5;
	float swing = 0.0f;
	if (p->onGround) swing = sinf(p->phase) * 5.0f;

	if (p->onGround) {
		putLine(cx, sy + 16, cx + (int)swing, sy + 23, 2, cr, cg, cb);
		putLine(cx, sy + 16, cx - (int)swing, sy + 23, 2, cr, cg, cb);
	} else {
		putLine(cx, sy + 16, cx - 4, sy + 22, 2, cr, cg, cb);
		putLine(cx, sy + 16, cx + 4, sy + 22, 2, cr, cg, cb);
	}

	putRect(cx - 1, sy + 8, 2, 9, cr, cg, cb);

	if (p->onGround) {
		putLine(cx, sy + 10, cx - (int)(swing * 0.8f), sy + 16, 2, cr, cg, cb);
		putLine(cx, sy + 10, cx + (int)(swing * 0.8f), sy + 16, 2, cr, cg, cb);
	} else {
		putLine(cx, sy + 10, cx - 5, sy + 5, 2, cr, cg, cb);
		putLine(cx, sy + 10, cx + 5, sy + 5, 2, cr, cg, cb);
	}

	putRect(sx + 1, sy, 8, 8, 255, 205, 150);
	putRect(sx + 1, sy, 8, 3, 220, 30, 30);
	if (p->facing > 0) {
		putRect(sx + 7, sy + 4, 1, 2, 0, 0, 0);
		putRect(sx + 8, sy + 2, 3, 2, 220, 30, 30);
	} else {
		putRect(sx + 2, sy + 4, 1, 2, 0, 0, 0);
		putRect(sx - 1, sy + 2, 3, 2, 220, 30, 30);
	}
}

static void buildLevel(void)
{
	memset(g_map, 0, sizeof(g_map));
	g_map[0][149] = 1;
	g_map[1][149] = 1;
	g_map[2][149] = 1;
	for (int c = 112; c <= 113; c++) g_map[3][c] = 1;
	g_map[3][149] = 1;
	g_map[4][149] = 1;
	g_map[5][7] = 1;
	for (int c = 14; c <= 15; c++) g_map[5][c] = 1;
	g_map[5][149] = 1;
	g_map[6][14] = 1;
	g_map[6][149] = 1;
	g_map[7][149] = 1;
	g_map[8][149] = 1;
	for (int c = 20; c <= 23; c++) g_map[9][c] = 1;
	g_map[9][58] = 1;
	for (int c = 90; c <= 94; c++) g_map[9][c] = 1;
	for (int c = 114; c <= 117; c++) g_map[9][c] = 1;
	g_map[9][149] = 1;
	for (int c = 57; c <= 58; c++) g_map[10][c] = 1;
	g_map[10][70] = 1;
	for (int c = 100; c <= 103; c++) g_map[10][c] = 1;
	for (int c = 108; c <= 109; c++) g_map[10][c] = 1;
	for (int c = 126; c <= 128; c++) g_map[10][c] = 1;
	g_map[10][149] = 1;
	g_map[11][6] = 1;
	g_map[11][31] = 1;
	for (int c = 56; c <= 58; c++) g_map[11][c] = 1;
	g_map[11][65] = 1;
	g_map[11][70] = 1;
	for (int c = 108; c <= 109; c++) g_map[11][c] = 1;
	g_map[11][149] = 1;
	for (int c = 5; c <= 6; c++) g_map[12][c] = 1;
	g_map[12][10] = 1;
	g_map[12][19] = 1;
	for (int c = 30; c <= 31; c++) g_map[12][c] = 1;
	for (int c = 55; c <= 58; c++) g_map[12][c] = 1;
	g_map[12][65] = 1;
	g_map[12][70] = 1;
	for (int c = 108; c <= 109; c++) g_map[12][c] = 1;
	g_map[12][149] = 1;
	for (int c = 0; c <= 6; c++) g_map[13][c] = 1;
	for (int c = 9; c <= 11; c++) g_map[13][c] = 1;
	for (int c = 14; c <= 39; c++) g_map[13][c] = 1;
	for (int c = 42; c <= 77; c++) g_map[13][c] = 1;
	for (int c = 84; c <= 119; c++) g_map[13][c] = 1;
	for (int c = 122; c <= 149; c++) g_map[13][c] = 1;
	for (int c = 0; c <= 6; c++) g_map[14][c] = 1;
	for (int c = 9; c <= 11; c++) g_map[14][c] = 1;
	for (int c = 14; c <= 39; c++) g_map[14][c] = 1;
	for (int c = 42; c <= 77; c++) g_map[14][c] = 1;
	for (int c = 84; c <= 119; c++) g_map[14][c] = 1;
	for (int c = 122; c <= 149; c++) g_map[14][c] = 1;
}

static int solidAt(int tx, int ty)
{
	if (tx < 0 || tx >= COLS) return 1;
	if (ty < 0 || ty >= ROWS) return 0;
	return g_map[ty][tx];
}

static void resetPlayer(void)
{
	g_player.x = 2 * T;
	g_player.y = (12 + 1) * T - PH;
	g_player.vx = 0;
	g_player.vy = 0;
	g_player.onGround = 1;
	g_player.facing = 1;
	g_player.phase = 0;
}

static float approach(float v, float target, float step)
{
	if (v < target) { v += step; if (v > target) v = target; }
	else if (v > target) { v -= step; if (v < target) v = target; }
	return v;
}

static void updatePlayer(Player* p, int dir, int run, int jumpPressed, int jumpHeld)
{
	float target = dir * (run ? RUN_SPEED : WALK_SPEED);
	float accel = p->onGround ? 0.30f : 0.18f;
	if (dir == 0) accel = p->onGround ? 0.40f : 0.05f;
	p->vx = approach(p->vx, target, accel);
	if (dir != 0) p->facing = dir;

	if (jumpPressed && p->onGround) {
		p->vy = -JUMP_SPEED;
		p->onGround = 0;
	}
	if (!jumpHeld && p->vy < -3.0f) p->vy = -3.0f;

	p->vy += GRAVITY;
	if (p->vy > MAX_FALL) p->vy = MAX_FALL;

	p->x += p->vx;
	{
		int top = (int)floorf(p->y / T);
		int bot = (int)floorf((p->y + PH - 1) / T);
		if (p->vx > 0) {
			int tx = (int)floorf((p->x + PW - 0.01f) / T);
			for (int ty = top; ty <= bot; ty++) {
				if (solidAt(tx, ty)) { p->x = tx * T - PW; p->vx = 0; break; }
			}
		} else if (p->vx < 0) {
			int tx = (int)floorf(p->x / T);
			for (int ty = top; ty <= bot; ty++) {
				if (solidAt(tx, ty)) { p->x = (tx + 1) * T; p->vx = 0; break; }
			}
		}
	}

	p->y += p->vy;
	p->onGround = 0;
	{
		int l = (int)floorf(p->x / T);
		int r = (int)floorf((p->x + PW - 0.01f) / T);
		if (p->vy >= 0) {
			int ty = (int)floorf((p->y + PH) / T);
			for (int tx = l; tx <= r; tx++) {
				if (solidAt(tx, ty)) {
					p->y = ty * T - PH;
					p->vy = 0;
					p->onGround = 1;
					break;
				}
			}
		} else {
			int ty = (int)floorf(p->y / T);
			for (int tx = l; tx <= r; tx++) {
				if (solidAt(tx, ty)) {
					p->y = (ty + 1) * T;
					p->vy = 0;
					break;
				}
			}
		}
	}

	if (p->onGround && fabsf(p->vx) > 0.1f)
		p->phase += fabsf(p->vx) * 0.18f;
	else if (p->onGround)
		p->phase = 0;
}

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	printf("\x1b[1;1H=== Mi Juego 3DS ===");
	printf("\x1b[4;1HCruceta / Stick : caminar");
	printf("\x1b[6;1HA               : saltar");
	printf("\x1b[8;1HB (mantener)    : correr");
	printf("\x1b[10;1HLlega a la bandera roja!");
	printf("\x1b[14;1HSTART          : salir");
	printf("\x1b[17;1HMetas: 0   Caidas: 0   ");

	buildLevel();
	resetPlayer();

	int lastWins = 0, lastDeaths = 0;

	while (aptMainLoop())
	{
		hidScanInput();
		u32 held = hidKeysHeld();
		u32 down = hidKeysDown();

		if (down & KEY_START) break;

		circlePosition cp;
		hidCircleRead(&cp);

		int dir = 0;
		if ((held & KEY_DRIGHT) || cp.dx > 30) dir += 1;
		if ((held & KEY_DLEFT)  || cp.dx < -30) dir -= 1;
		int run = (held & KEY_B) ? 1 : 0;

		updatePlayer(&g_player, dir, run, (down & KEY_A) ? 1 : 0, (held & KEY_A) ? 1 : 0);

		if (g_player.y > SH + 40) {
			g_deaths++;
			resetPlayer();
		}

		if (g_player.x > GOAL_COL * T) {
			g_wins++;
			resetPlayer();
		}

		if (g_wins != lastWins || g_deaths != lastDeaths) {
			printf("\x1b[17;1HMetas: %d   Caidas: %d   ", g_wins, g_deaths);
			lastWins = g_wins;
			lastDeaths = g_deaths;
		}

		float cam = g_player.x + PW / 2 - SW / 2;
		if (cam < 0) cam = 0;
		if (cam > COLS * T - SW) cam = COLS * T - SW;
		int camx = (int)cam;

		fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

		putRect(0, 0, SW, SH, 87, 146, 255);

		for (int i = 0; i < 8; i++) {
			int wx = 60 + i * 330;
			drawCloud(wx - camx / 2, 20 + (i % 3) * 22);
		}

		int firstCol = camx / T;
		int lastCol = (camx + SW) / T;
		if (lastCol >= COLS) lastCol = COLS - 1;
		for (int ty = 0; ty < ROWS; ty++) {
			for (int tx = firstCol; tx <= lastCol; tx++) {
				if (g_map[ty][tx]) drawTile(tx * T - camx, ty * T);
			}
		}

		{
			int px = GOAL_COL * T - camx;
			putRect(px + 6, 4 * T, 4, 9 * T, 220, 220, 220);
			putRect(px + 10, 4 * T + 2, 20, 12, 220, 30, 30);
			putRect(px + 4, 4 * T - 4, 8, 4, 255, 215, 0);
		}

		drawPlayer((int)(g_player.x - camx), (int)g_player.y, &g_player);

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
