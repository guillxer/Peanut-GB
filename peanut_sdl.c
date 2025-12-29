

#define SCREEN_SCALE 1
#define EMU_WIDTH 160
#define EMU_HEIGHT 144
#define DEVICE_WIDTH 128 * SCREEN_SCALE
#define DEVICE_HEIGHT 128 * SCREEN_SCALE
#define LOG_BUFFER_SIZE 256

#if defined(MICROPY_ENABLE_DYNRUNTIME)
#define RP2 1
#else
#define RP2 0
#endif

#if RP2
#include "py/dynruntime.h"
#else
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "SDL.h"
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
void InitRenderer() {
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	}
	// Create a window
	window = SDL_CreateWindow("SDL Draw Line", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEVICE_WIDTH, DEVICE_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
	}
	// Create a renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
	}
}
void ClearScreen(int value) {
	SDL_PumpEvents();
	// clear rt
	SDL_SetRenderDrawColor(renderer, value, value, value, 255);
	// Clear the screen
	SDL_RenderClear(renderer);
}
void UpdateFrame() {
	SDL_RenderPresent(renderer);
}
#endif

#if RP2
#if 0
void* memcpy(void* dst, const void* src, size_t len) {
	size_t i;
	unsigned char* d = (unsigned char*)dst;
	const unsigned char* s = (unsigned char*)src;

	for (i = 0; i < len; ++i) {
		d[i] = s[i];
	}
	return dst;
}
#endif
#if 1
void* memset(void* dst, int val, size_t len) {
	size_t i;
	unsigned char* d = (unsigned char*)dst;

	for (i = 0; i < len; ++i) {
		d[i] = val;
	}
	return dst;
}
void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if (d < s) {
        // Copy forwards (from start to end)
        while (n--) {
            *d++ = *s++;
        }
    } else {
        // Copy backwards (from end to start)
        char *lasts = (char *)s + (n - 1);
        char *lastd = (char *)d + (n - 1);
        while (n--) {
            *lastd-- = *lasts--;
        }
    }
    return dest;
}
#endif
#endif


void PeanutInitImpl(
	int* errorNum,
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned int* saveFileSize);
void PeanutRunImpl(
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned char* cartRamData,
	unsigned char controllerInput);
void ResampleBufferImpl(unsigned short* colorBuffer);

#if RP2
static mp_obj_t InitPeanut(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) 
{
	int argindex = 0;
	
	int romAddress = mp_obj_get_int(args[argindex++]);
	unsigned char* cartRomData = (unsigned char*)(void*)romAddress;

#if 0
	mp_buffer_info_t cartRomDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRomDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRomData = (unsigned char*)cartRomDataBuffer.buf;
#endif

	mp_buffer_info_t saveFileSizeBuffer;
	mp_get_buffer_raise(args[argindex++], &saveFileSizeBuffer, MP_BUFFER_RW);
	unsigned int* saveFileSize = (unsigned int*)saveFileSizeBuffer.buf;

	char* msg = NULL;
	int errorNum = 0;

	PeanutInitImpl(
		&errorNum,
		msg,
		LOG_BUFFER_SIZE,
		cartRomData,
		saveFileSize);
	return mp_const_none;
}

static mp_obj_t PeanutRun(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	int romAddress = mp_obj_get_int(args[argindex++]);
	unsigned char* cartRomData = (unsigned char*)(void*)romAddress;
	
#if 0
	mp_buffer_info_t cartRomDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRomDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRomData = (unsigned char*)cartRomDataBuffer.buf;
#endif
	
	mp_buffer_info_t controllerBuffer;
	mp_get_buffer_raise(args[argindex++], &controllerBuffer, MP_BUFFER_RW);
	unsigned char* controllerData = (unsigned char*)controllerBuffer.buf;

	mp_buffer_info_t cartRamDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRamDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRamData = (unsigned char*)cartRamDataBuffer.buf;

	char* msg = NULL;
	
	PeanutRunImpl(
		msg,
		LOG_BUFFER_SIZE,
		cartRomData,
		cartRamData,
		controllerData[0]);
	return mp_const_none;
}

static mp_obj_t ResampleBuffer(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	mp_buffer_info_t colorBuffer;
	mp_get_buffer_raise(args[argindex++], &colorBuffer, MP_BUFFER_RW);
	unsigned short* color = (unsigned short*)colorBuffer.buf;

	ResampleBufferImpl(color);
	return mp_const_none;
}

mp_obj_t mpy_init(mp_obj_fun_bc_t* self, size_t n_args, size_t n_kw, mp_obj_t* args) {
	MP_DYNRUNTIME_INIT_ENTRY
	mp_store_global(
		MP_QSTR_InitPeanut,
		MP_DYNRUNTIME_MAKE_FUNCTION(InitPeanut));
	mp_store_global(
		MP_QSTR_PeanutRun,
		MP_DYNRUNTIME_MAKE_FUNCTION(PeanutRun));
	mp_store_global(
		MP_QSTR_ResampleBuffer,
		MP_DYNRUNTIME_MAKE_FUNCTION(ResampleBuffer));
	MP_DYNRUNTIME_INIT_EXIT
}
#endif

#include "peanut_gb.h"

char* messageLogPtr;

struct gb_s gb;
struct priv_t priv =
{
	.rom = NULL,
	.cart_ram = NULL
};
enum gb_init_error_e gb_ret;


unsigned char gb_rom_read(
	struct gb_s* gb,
	const uint_fast32_t addr)
{
	const struct priv_t* const p = gb->direct.priv;
	return p->rom[addr];
}
uint8_t gb_cart_ram_read(
	struct gb_s* gb,
	const uint_fast32_t addr)
{
	const struct priv_t* const p = gb->direct.priv;
	return p->cart_ram[addr];
}
void gb_cart_ram_write(
	struct gb_s* gb,
	const uint_fast32_t addr,
	const uint8_t val)
{
	const struct priv_t* const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}
uint8_t gb_bootrom_read(
	struct gb_s* gb,
	const uint_fast16_t addr)
{
	const struct priv_t* const p = gb->direct.priv;
	return p->bootrom[addr];
}
void LogMessage(char* message)
{
	return;
	#if 0
	if (strlen(message) + 1 > LOG_BUFFER_SIZE)
	{
		strcpy_s(messageLogPtr, LOG_BUFFER_SIZE, message);
		messageLogPtr[LOG_BUFFER_SIZE-1] = '\0';
	}
	else
	{
		strcpy_s(messageLogPtr, strlen(message) + 1, message);
	}
	#endif
}
void gb_error(
	struct gb_s* gb,
	const enum gb_error_e gb_err,
	const uint16_t addr)
{
	// TODO move error messages here
}

void auto_assign_palette(struct priv_t* priv, uint8_t game_checksum)
{
	//size_t palette_bytes = 3 * 4 * sizeof(uint16_t);
	switch (game_checksum)
	{
		/* Balloon Kid and Tetris Blast */
		case 0x71:
		case 0xFF:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x7E60, 0x7C00, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x7E60, 0x7C00, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x7E60, 0x7C00, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Pokemon Yellow and Tetris */
		case 0x15:
		case 0xDB:
		case 0x95: /* Not officially */
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x7FE0, 0x7C00, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x7FE0, 0x7C00, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x7FE0, 0x7C00, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Donkey Kong */
		case 0x19:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x7E60, 0x7C00, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Pokemon Blue */
		case 0x61:
		case 0x45:

			/* Pokemon Blue Star */
		case 0xD8:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x329F, 0x001F, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x329F, 0x001F, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Pokemon Red */
		case 0x14:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x3FE6, 0x0200, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Pokemon Red Star */
		case 0x8B:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x329F, 0x001F, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x3FE6, 0x0200, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Kirby */
		case 0x27:
		case 0x49:
		case 0x5C:
		case 0xB3:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7D8A, 0x6800, 0x3000, 0x0000 }, /* OBJ0 */
				{ 0x001F, 0x7FFF, 0x7FEF, 0x021F }, /* OBJ1 */
				{ 0x527F, 0x7FE0, 0x0180, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Donkey Kong Land [1/2/III] */
		case 0x18:
		case 0x6A:
		case 0x4B:
		case 0x6B:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7F08, 0x7F40, 0x48E0, 0x2400 }, /* OBJ0 */
				{ 0x7FFF, 0x2EFF, 0x7C00, 0x001F }, /* OBJ1 */
				{ 0x7FFF, 0x463B, 0x2951, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Link's Awakening */
		case 0x70:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x03E0, 0x1A00, 0x0120 }, /* OBJ0 */
				{ 0x7FFF, 0x329F, 0x001F, 0x001F }, /* OBJ1 */
				{ 0x7FFF, 0x7E10, 0x48E7, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		/* Mega Man [1/2/3] & others I don't care about. */
		case 0x01:
		case 0x10:
		case 0x29:
		case 0x52:
		case 0x5D:
		case 0x68:
		case 0x6D:
		case 0xF6:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x329F, 0x001F, 0x0000 }, /* OBJ0 */
				{ 0x7FFF, 0x3FE6, 0x0200, 0x0000 }, /* OBJ1 */
				{ 0x7FFF, 0x7EAC, 0x40C0, 0x0000 }  /* BG */
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//memcpy(priv->selected_palette, palette, palette_bytes);
			break;
		}

		default:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7FFF, 0x5294, 0x294A, 0x0000 },
				{ 0x7FFF, 0x5294, 0x294A, 0x0000 },
				{ 0x7FFF, 0x5294, 0x294A, 0x0000 }
			};
			for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			//LogMessage("No palette found.");
			//memcpy(priv->selected_palette, palette, palette_bytes);
		}
	}
}

void PeanutInitImpl(
	int* errorNum,
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned int* saveFileSize)
{
	messageLogPtr = logBuffer;

	priv.rom = cartRomData;
	priv.cart_ram = NULL;

	gb_ret = gb_init(
		&gb,
		&gb_rom_read,
		&gb_cart_ram_read,
		&gb_cart_ram_write,
		&gb_error,
		&priv);

	gb_ret = *errorNum;
	
	#if 0
	switch (gb_ret)
	{
		case GB_INIT_NO_ERROR:
			LogMessage("No Error.");
			break;
		case GB_INIT_CARTRIDGE_UNSUPPORTED:
			LogMessage("Unsupported cartridge.");
			return;
		case GB_INIT_INVALID_CHECKSUM:
			LogMessage("Invalid ROM: Checksum failure.");
			return;
		default:
			LogMessage("Unknown error.");
			return;
	}
	#endif

	if (gb_get_save_size_s(&gb, &priv.save_size) < 0)
	{
		gb_ret = *errorNum;
		//LogMessage("Unable to get save size.");
		//return;
	}
	
	saveFileSize[0] = priv.save_size;

	gb_init_lcd(&gb, &lcd_draw_line);
	
	auto_assign_palette(&priv, gb_colour_hash(&gb));
}

void PeanutRunImpl(
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned char* cartRamData,
	unsigned char controllerInput)
{
	priv.rom = cartRomData;
	priv.cart_ram = cartRamData;
	gb.direct.joypad = controllerInput;
	gb_run_frame(&gb);
}

void ResampleBufferImpl(unsigned short* colorBuffer)
{
	for (int x = 0; x < DEVICE_WIDTH; ++x) {
		for (int y = 0; y < DEVICE_HEIGHT; ++y) {
			unsigned short pixelData = priv.fb[y][x];
			colorBuffer[y * DEVICE_HEIGHT + x] = pixelData;
		}
	}
}

#if !RP2
void Present(unsigned short* colorBuffer)
{
	for (int x = 0; x < DEVICE_WIDTH; ++x) {
		for (int y = 0; y < DEVICE_HEIGHT; ++y) {
			unsigned int color565 = colorBuffer[x * DEVICE_HEIGHT + y];
			float red = (float)((color565 >> 10) & 0x1F) / 31.0f;
			float green = (float)((color565 >> 5) & 0x1F) / 31.0f;
			float blue = (float)((color565) & 0x1F) / 31.0f;
			SDL_SetRenderDrawColor(renderer, 255.0f * red, 255.0f * green, 255.0f * blue, 255);
			SDL_RenderDrawPoint(renderer, x, y);
		}
	}
}

void LoadRom(unsigned char** data, char* fileName)
{
	FILE* dataFile;
	errno_t err = fopen_s(&dataFile, fileName, "rb");
	assert(err == 0);
	if (dataFile)
	{
		// Allocate size of file
		fseek(dataFile, 0L, SEEK_END);
		int fileSize = ftell(dataFile);
		*data = malloc(fileSize);
		fseek(dataFile, 0, SEEK_SET);
		fread_s(*data, fileSize, sizeof(unsigned char), fileSize, dataFile);
		fclose(dataFile);
	}
}

void LoadSave(unsigned char** data, char* fileName, int fileSize)
{
	FILE* dataFile;
	if (fileSize <= 0)
	{
		return;
	}
	*data = malloc(fileSize);
	errno_t err = fopen_s(&dataFile, fileName, "rb");
	if (err != 0)
	{
		err = fopen_s(&dataFile, fileName, "wb");
		assert(err == 0);
		if (dataFile)
		{
			memset(*data, 0, fileSize);
			fwrite(*data, sizeof(unsigned char), fileSize, dataFile);
			fclose(dataFile);
		}
	}
	else
	{
		fread_s(*data, fileSize, sizeof(unsigned char), fileSize, dataFile);
	}
	fclose(dataFile);
}


static unsigned char* cartRomData = NULL;
static unsigned char* cartRamData = NULL;

#define main main  // Undo SDL main entry rename
int main()
{
	InitRenderer();
	char* logBuffer = malloc(LOG_BUFFER_SIZE);
	// Read cart rom and ram, ram size encoded in rom
	int errorNumber = 0;
	int saveFileSize = 0;

	LoadRom(&cartRomData, "PokemonRed.gb");

	PeanutInit(
		&errorNumber,
		logBuffer,
		LOG_BUFFER_SIZE,
		cartRomData,
		&saveFileSize);

	if (saveFileSize > 0)
	{
		LoadSave(&cartRamData, "PokemonRed.sav", saveFileSize);
	}

	bool bRunning = true;
	unsigned char controllerInput = 0xff;
	while (bRunning)
	{
		SDL_PumpEvents();
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			static int fullscreen = 0;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_RETURN:
						controllerInput &= ~JOYPAD_START;
						break;

					case SDLK_BACKSPACE:
						controllerInput &= ~JOYPAD_SELECT;
						break;

					case SDLK_z:
						controllerInput &= ~JOYPAD_A;
						break;

					case SDLK_x:
						controllerInput &= ~JOYPAD_B;
						break;

					case SDLK_UP:
						controllerInput &= ~JOYPAD_UP;
						break;

					case SDLK_RIGHT:
						controllerInput &= ~JOYPAD_RIGHT;
						break;

					case SDLK_DOWN:
						controllerInput &= ~JOYPAD_DOWN;
						break;

					case SDLK_LEFT:
						controllerInput &= ~JOYPAD_LEFT;
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_RETURN:
						controllerInput |= JOYPAD_START;
						break;

					case SDLK_BACKSPACE:
						controllerInput |= JOYPAD_SELECT;
						break;

					case SDLK_z:
						controllerInput |= JOYPAD_A;
						break;

					case SDLK_x:
						controllerInput |= JOYPAD_B;
						break;

					case SDLK_UP:
						controllerInput |= JOYPAD_UP;
						break;

					case SDLK_RIGHT:
						controllerInput |= JOYPAD_RIGHT;
						break;

					case SDLK_DOWN:
						controllerInput |= JOYPAD_DOWN;
						break;

					case SDLK_LEFT:
						controllerInput |= JOYPAD_LEFT;
						break;
				}
			}
			break;
		}

		PeanutRun(
			logBuffer,
			LOG_BUFFER_SIZE,
			cartRomData,
			cartRamData,
			controllerInput);

		ResampleBuffer(colorBuffer);
		Present(colorBuffer);
		UpdateFrame();
	}
	SDL_Quit();
	exit(0);
	return 0;
}
#endif
