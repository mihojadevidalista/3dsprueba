#include <3ds.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	gfxInitDefault();

	// Consola de texto en la pantalla superior
	consoleInit(GFX_TOP, NULL);

	printf("\x1b[12;18HHola");
	printf("\x1b[29;1HPresiona START para salir");

	// Bucle principal
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // Vuelve al Homebrew Launcher

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
