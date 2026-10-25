// Muñequito Runner - homebrew para Nintendo 3DS
// Controles: Cruceta o Stick = mover | A = saltar | B (mantener) = correr | START = salir

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SW 400          // ancho de la pantalla superior
#define SH 240          // alto de la pantalla superior
#define T 16            // tamaño de cada bloque (pixeles)
#define COLS 150        // ancho del nivel en bloques
#define ROWS 15         // alto del nivel en bloques
#define GOAL_COL 140    // columna de la meta

#define PW 10           // ancho del muñeco
#define PH 24           // alto del muñeco

#define WALK_SPEED 2.0f
#define RUN_SPEED  3.5f
#define GRAVITY    0.4f
#define JUMP_SPEED 7.5f
#define MAX_FALL   8.0f

typedef struct {
	float x, y, vx, vy;
	int onGround;
	int facing;      // 1 = derecha, -1 = izquierda
	float phase;     // fase de la animacion de las piernas
} Player;

unsigned char g_map[ROWS][COLS];
Player g_player;
int g_wins = 0;
int g_deaths = 0;

static u8* fb;

// ---------------------------------------------------------------
// Dibujo (directo al framebuffer de la pantalla superior)
// ---------------------------------------------------------------

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
	putRect(sx, sy, T, T, 200, 76, 12);               // ladrillo
	putRect(sx, sy, T, 1, 240, 160, 90);              // borde claro arriba
	putRect(sx, sy + T - 1, T, 1, 90, 30, 0);         // borde oscuro abajo
	putRect(sx + T - 1, sy, 1, T, 90, 30, 0);         // borde oscuro derecha
	putRect(sx + 7, sy + 4, 1, 8, 140, 50, 5);        // detalle
}

static void drawCloud(int x, int y)
{
	putRect(x + 8, y, 24, 8, 255, 255, 255);
	putRect(x, y + 8, 48, 10, 255, 255, 255);
	putRect(x + 6, y + 18, 36, 4, 235, 245, 255);
}

static void drawPlayer(int sx, int sy, const Player* p)
{
	const u8 cr = 20, cg = 20, cb = 20;   // color del "palito"
	int cx = sx + 5;                      // centro del cuerpo
	float swing = 0.0f;
	if (p->onGround) swing = sinf(p->phase) * 5.0f;

	// piernas
	if (p->onGround) {
		putLine(cx, sy + 16, cx + (int)swing, sy + 23, 2, cr, cg, cb);
		putLine(cx, sy + 16, cx - (int)swing, sy + 23, 2, cr, cg, cb);
	} else {
		putLine(cx, sy + 16, cx - 4, sy + 22, 2, cr, cg, cb);
		putLine(cx, sy + 16, cx + 4, sy + 22, 2, cr, cg, cb);
	}

	// cuerpo
	putRect(cx - 1, sy + 8, 2, 9, cr, cg, cb);

	// brazos
	if (p->onGround) {
		putLine(cx, sy + 10, cx - (int)(swing * 0.8f), sy + 16, 2, cr, cg, cb);
		putLine(cx, sy + 10, cx + (int)(swing * 0.8f), sy + 16, 2, cr, cg, cb);
	} else {
		putLine(cx, sy + 10, cx - 5, sy + 5, 2, cr, cg, cb);
		putLine(cx, sy + 10, cx + 5, sy + 5, 2, cr, cg, cb);
	}

	// cabeza con gorra roja (mira hacia donde camina)
	putRect(sx + 1, sy, 8, 8, 255, 205, 150);
	putRect(sx + 1, sy, 8, 3, 220, 30, 30);
	if (p->facing > 0) {
		putRect(sx + 7, sy + 4, 1, 2, 0, 0, 0);       // ojo derecha
		putRect(sx + 8, sy + 2, 3, 2, 220, 30, 30);   // visera
	} else {
		putRect(sx + 2, sy + 4, 1, 2, 0, 0, 0);       // ojo izquierda
		putRect(sx - 1, sy + 2, 3, 2, 220, 30, 30);   // visera
	}
}

// ---------------------------------------------------------------
// Nivel
// ---------------------------------------------------------------

static void buildLevel(void)
{
	memset(g_map, 0, sizeof(g_map));

	// suelo: dos filas de bloques a lo largo de todo el camino
	for (int c = 0; c < COLS; c++) {
		g_map[13][c] = 1;
		g_map[14][c] = 1;
	}

	// hoyo chico (2 bloques)
	for (int c = 40; c < 42; c++) { g_map[13][c] = 0; g_map[14][c] = 0; }

	// hoyo grande (6 bloques): hay que correr con B para pasarlo
	for (int c = 78; c < 84; c++) { g_map[13][c] = 0; g_map[14][c] = 0; }

	// bloques flotantes
	for (int c = 20; c < 24; c++) g_map[9][c] = 1;
	for (int c = 90; c < 95; c++) g_map[9][c] = 1;
	for (int c = 100; c < 104; c++) g_map[10][c] = 1;

	// bloques sueltos en el piso
	g_map[12][30] = 1;
	g_map[12][31] = 1;
	g_map[11][31] = 1;

	// escalera de 4 escalones
	for (int h = 1; h <= 4; h++)
		for (int r = 13 - h; r < 13; r++)
			g_map[r][54 + h] = 1;

	// columnas de bloques
	for (int r = 11; r < 13; r++) g_map[r][65] = 1;
	for (int r = 10; r < 13; r++) g_map[r][70] = 1;

	// pared al final del mundo
	for (int r = 0; r < ROWS; r++) g_map[r][COLS - 1] = 1;
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
	g_player.y = 13 * T - PH;
	g_player.vx = 0;
	g_player.vy = 0;
	g_player.onGround = 1;
	g_player.facing = 1;
	g_player.phase = 0;
}

// ---------------------------------------------------------------
// Fisica
// ---------------------------------------------------------------

static float approach(float v, float target, float step)
{
	if (v < target) { v += step; if (v > target) v = target; }
	else if (v > target) { v -= step; if (v < target) v = target; }
	return v;
}

static void updatePlayer(Player* p, int dir, int run, int jumpPressed, int jumpHeld)
{
	// --- movimiento horizontal ---
	float target = dir * (run ? RUN_SPEED : WALK_SPEED);
	float accel = p->onGround ? 0.30f : 0.18f;
	if (dir == 0) accel = p->onGround ? 0.40f : 0.05f;
	p->vx = approach(p->vx, target, accel);
	if (dir != 0) p->facing = dir;

	// --- salto ---
	if (jumpPressed && p->onGround) {
		p->vy = -JUMP_SPEED;
		p->onGround = 0;
	}
	if (!jumpHeld && p->vy < -3.0f) p->vy = -3.0f;   // salto corto si sueltas A

	// --- gravedad ---
	p->vy += GRAVITY;
	if (p->vy > MAX_FALL) p->vy = MAX_FALL;

	// --- mover en X y chocar ---
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

	// --- mover en Y y chocar ---
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

	// --- animacion ---
	if (p->onGround && fabsf(p->vx) > 0.1f)
		p->phase += fabsf(p->vx) * 0.18f;
	else if (p->onGround)
		p->phase = 0;
}

// ---------------------------------------------------------------
// Programa principal
// ---------------------------------------------------------------

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	printf("\x1b[1;1H=== MUNEQUITO RUNNER ===");
	printf("\x1b[4;1HCruceta / Stick : caminar");
	printf("\x1b[6;1HA               : saltar");
	printf("\x1b[8;1HB (mantener)    : correr");
	printf("\x1b[10;1HLlega a la bandera roja!");
	printf("\x1b[12;1HCuidado con los hoyos!");
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
		if ((held & KEY_DPAD_RIGHT) || cp.dx > 30) dir += 1;
		if ((held & KEY_DPAD_LEFT)  || cp.dx < -30) dir -= 1;
		int run = (held & KEY_B) ? 1 : 0;

		updatePlayer(&g_player, dir, run, (down & KEY_A) ? 1 : 0, (held & KEY_A) ? 1 : 0);

		// se cayo a un hoyo
		if (g_player.y > SH + 40) {
			g_deaths++;
			resetPlayer();
		}

		// llego a la meta
		if (g_player.x > GOAL_COL * T) {
			g_wins++;
			resetPlayer();
		}

		if (g_wins != lastWins || g_deaths != lastDeaths) {
			printf("\x1b[17;1HMetas: %d   Caidas: %d   ", g_wins, g_deaths);
			lastWins = g_wins;
			lastDeaths = g_deaths;
		}

		// camara
		float cam = g_player.x + PW / 2 - SW / 2;
		if (cam < 0) cam = 0;
		if (cam > COLS * T - SW) cam = COLS * T - SW;
		int camx = (int)cam;

		// --- dibujar ---
		fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

		putRect(0, 0, SW, SH, 92, 148, 252);                // cielo

		for (int i = 0; i < 8; i++) {                       // nubes (parallax)
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

		// bandera de la meta
		{
			int px = GOAL_COL * T - camx;
			putRect(px + 6, 4 * T, 4, 9 * T, 220, 220, 220);   // mastil
			putRect(px + 10, 4 * T + 2, 20, 12, 220, 30, 30);  // bandera
			putRect(px + 4, 4 * T - 4, 8, 4, 255, 215, 0);     // punta
		}

		drawPlayer((int)(g_player.x - camx), (int)g_player.y, &g_player);

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
