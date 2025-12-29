/**
 * MIT License
 * Copyright (c) 2018-2023 Mahyar Koshkouei
 *
 * An example of using the peanut_gb.h library. This example application uses
 * SDL2 to draw the screen and get input.
 */


#if 0
#include <math.h>

#include <stdint.h>
#include <stdlib.h>

#include "SDL.h"


#include "peanut_gb.h"

enum {
	LOG_CATERGORY_PEANUTSDL = SDL_LOG_CATEGORY_CUSTOM
};

struct priv_t
{
	/* Window context used to generate message boxes. */
	SDL_Window *win;
	/* Pointer to allocated memory holding GB file. */
	uint8_t *rom;
	/* Pointer to allocated memory holding save file. */
	uint8_t *cart_ram;
	/* Size of the cart_ram in bytes. */
	size_t save_size;
	/* Pointer to boot ROM binary if available. */
	uint8_t *bootrom;

	/* Colour palette for each BG, OBJ0, and OBJ1. */
	uint16_t selected_palette[3][4];
	uint16_t fb[LCD_HEIGHT][LCD_WIDTH];
};

/**
 * Returns a byte from the ROM file at the given address.
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
		       const uint8_t val)
{
	const struct priv_t * const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}

uint8_t gb_bootrom_read(struct gb_s *gb, const uint_fast16_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->bootrom[addr];
}

void read_cart_ram_file(const char *save_file_name, uint8_t **dest,
			const size_t len)
{
	SDL_RWops *f;

	/* If save file not required. */
	if(len == 0)
	{
		*dest = NULL;
		return;
	}

	/* Allocate enough memory to hold save file. */
	if((*dest = SDL_malloc(len)) == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"%d: %s", __LINE__, SDL_GetError());
		exit(EXIT_FAILURE);
	}

	f = SDL_RWFromFile(save_file_name, "rb");

	/* It doesn't matter if the save file doesn't exist. We initialise the
	 * save memory allocated above. The save file will be created on exit. */
	if(f == NULL)
	{
		SDL_memset(*dest, 0, len);
		return;
	}

	/* Read save file to allocated memory. */
	SDL_RWread(f, *dest, sizeof(uint8_t), len);
	SDL_RWclose(f);
}

void write_cart_ram_file(const char *save_file_name, uint8_t **dest,
			 const size_t len)
{
	SDL_RWops *f;

	if(len == 0 || *dest == NULL)
		return;

	if((f = SDL_RWFromFile(save_file_name, "wb")) == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Unable to open save file: %s",
				SDL_GetError());
		return;
	}

	/* Record save file. */
	SDL_RWwrite(f, *dest, sizeof(uint8_t), len);
	SDL_RWclose(f);

	return;
}

/**
 * Handles an error reported by the emulator. The emulator context may be used
 * to better understand why the error given in gb_err was reported.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
	const char* gb_err_str[GB_INVALID_MAX] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE",
		""
	};
	struct priv_t *priv = gb->direct.priv;
	char error_msg[256];
	char location[64] = "";
	uint8_t instr_byte;

	/* Record save file. */
	write_cart_ram_file("recovery.sav", &priv->cart_ram, priv->save_size);

	if(addr >= 0x4000 && addr < 0x8000)
	{
		uint32_t rom_addr;
		rom_addr = (uint32_t)addr * (uint32_t)gb->selected_rom_bank;
		SDL_snprintf(location, sizeof(location),
			" (bank %d mode %d, file offset %u)",
			gb->selected_rom_bank, gb->cart_mode_select, rom_addr);
	}

	instr_byte = __gb_read(gb, addr);

	SDL_snprintf(error_msg, sizeof(error_msg),
		"Error: %s at 0x%04X%s with instruction %02X.\n"
		"Cart RAM saved to recovery.sav\n"
		"Exiting.\n",
		gb_err_str[gb_err], addr, location, instr_byte);
	SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
			SDL_LOG_PRIORITY_CRITICAL,
			"%s", error_msg);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_msg, priv->win);

	/* Free memory and then exit. */
	SDL_free(priv->cart_ram);
	SDL_free(priv->rom);
	exit(EXIT_FAILURE);
}

/**
 * Automatically assigns a colour palette to the game using a given game
 * checksum.
 * TODO: Not all checksums are programmed in yet because I'm lazy.
 */
void auto_assign_palette(struct priv_t *priv, uint8_t game_checksum)
{
	size_t palette_bytes = 3 * 4 * sizeof(uint16_t);

	switch(game_checksum)
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		memcpy(priv->selected_palette, palette, palette_bytes);
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
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_INFO,
				"No palette found for 0x%02X.", game_checksum);
		memcpy(priv->selected_palette, palette, palette_bytes);
	}
	}
}


#if ENABLE_LCD
/**
 * Draws scanline into framebuffer.
 */
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160],
		   const uint_fast8_t line)
{
	struct priv_t *priv = gb->direct.priv;

	for(unsigned int x = 0; x < LCD_WIDTH; x++)
	{
		priv->fb[line][x] = priv->selected_palette
				    [(pixels[x] & LCD_PALETTE_ALL) >> 4]
				    [pixels[x] & 3];
	}
}
#endif






#define main main  // Undo SDL main entry rename

int main(int argc, char **argv)
{

	static struct gb_s gb;
	static struct priv_t priv =
	{
		.rom = NULL,
		.cart_ram = NULL
	};
	enum gb_init_error_e gb_ret;



	const double target_speed_ms = 1000.0 / VERTICAL_SYNC;
	double speed_compensation = 0.0;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	SDL_GameController *controller = NULL;
	uint_fast32_t new_ticks, old_ticks;
	unsigned int fast_mode = 1;
	unsigned int fast_mode_timer = 1;
	/* Record save file every 60 seconds. */
	int save_timer = 60;
	/* Must be freed */
	char *rom_file_name = NULL;
	char *save_file_name = NULL;
	int ret = EXIT_SUCCESS;

	SDL_LogSetPriority(LOG_CATERGORY_PEANUTSDL, SDL_LOG_PRIORITY_INFO);

	/* Enable Hi-DPI to stop blurry game image. */
#ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

	/* Initialise frontend implementation, in this case, SDL2. */
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0)
	{
		char buf[128];
		SDL_snprintf(buf, sizeof(buf),
				"Unable to initialise SDL2: %s", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", buf, NULL);
		ret = EXIT_FAILURE;
		goto out;
	}

	window = SDL_CreateWindow("Peanut-SDL: Opening File",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			LCD_WIDTH * 2, LCD_HEIGHT * 2,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

	if(window == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Could not create window: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}
	priv.win = window;

	switch(argc)
	{
	case 1:
		SDL_SetWindowTitle(window, "Drag and drop ROM");
		do
		{
			SDL_Delay(10);
			SDL_PollEvent(&event);

			switch(event.type)
			{
				case SDL_DROPFILE:
					rom_file_name = event.drop.file;
					break;

				case SDL_QUIT:
					ret = EXIT_FAILURE;
					goto out;

				default:
					break;
			}
		} while(rom_file_name == NULL);

		break;

	case 2:
		/* Apply file name to rom_file_name
		 * Set save_file_name to NULL. */
		rom_file_name = argv[1];
		break;

	case 3:
		/* Apply file name to rom_file_name
		 * Apply save name to save_file_name */
		rom_file_name = argv[1];
		save_file_name = argv[2];
		break;

	default:
#if ENABLE_FILE_GUI
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Usage: %s [ROM] [SAVE]", argv[0]);
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"A file picker is presented if ROM is not given.");
#else
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Usage: %s ROM [SAVE]\n", argv[0]);
#endif
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"SAVE is set by default if not provided.");
		ret = EXIT_FAILURE;
		goto out;
	}

	/* Copy input ROM file to allocated memory. */
	if((priv.rom = SDL_LoadFile(rom_file_name, NULL)) == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"%d: %s", __LINE__, SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	/* If no save file is specified, copy save file (with specific name) to
	 * allocated memory. */
	if(save_file_name == NULL)
	{
		char *str_replace;
		const char extension[] = ".sav";

		/* Allocate enough space for the ROM file name, for the "sav"
		 * extension and for the null terminator. */
		const int nameSize = SDL_strlen(rom_file_name) + SDL_strlen(extension) + 1;
		save_file_name = SDL_malloc(nameSize);

		if(save_file_name == NULL)
		{
			SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
					SDL_LOG_PRIORITY_CRITICAL,
					"%d: %s", __LINE__, SDL_GetError());
			ret = EXIT_FAILURE;
			goto out;
		}

		/* Copy the ROM file name to allocated space. */
		strcpy_s(save_file_name, nameSize, rom_file_name);

		/* If the file name does not have a dot, or the only dot is at
		 * the start of the file name, set the pointer to begin
		 * replacing the string to the end of the file name, otherwise
		 * set it to the dot. */
		if((str_replace = strrchr(save_file_name, '.')) == NULL ||
				str_replace == save_file_name)
			str_replace = save_file_name + strlen(save_file_name);

		/* Copy extension to string including terminating null byte. */
		for(unsigned int i = 0; i <= strlen(extension); i++)
			*(str_replace++) = extension[i];
	}

	/* TODO: Sanity check input GB file. */

	/* Initialise emulator context. */
	gb_ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write,
			 &gb_error, &priv);

	switch(gb_ret)
	{
	case GB_INIT_NO_ERROR:
		break;

	case GB_INIT_CARTRIDGE_UNSUPPORTED:
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Unsupported cartridge.");
		ret = EXIT_FAILURE;
		goto out;

	case GB_INIT_INVALID_CHECKSUM:
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Invalid ROM: Checksum failure.");
		ret = EXIT_FAILURE;
		goto out;

	default:
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Unknown error: %d", gb_ret);
		ret = EXIT_FAILURE;
		goto out;
	}

	/* Copy dmg_boot.bin boot ROM file to allocated memory. */
	if((priv.bootrom = SDL_LoadFile("dmg_boot.bin", NULL)) == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_INFO,
				"No dmg_boot.bin file found; disabling boot ROM");
	}
	else
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_INFO,
				"boot ROM enabled");
		gb_set_bootrom(&gb, gb_bootrom_read);
		gb_reset(&gb);
	}

	/* Load Save File. */
	if(gb_get_save_size_s(&gb, &priv.save_size) < 0)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Unable to get save size: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	/* Only attempt to load a save file if the ROM actually supports saves.*/
	if(priv.save_size > 0)
		read_cart_ram_file(save_file_name, &priv.cart_ram, priv.save_size);

	/* Set the RTC of the game cartridge. Only used by games that support it. */
	{
		time_t rawtime;
		time(&rawtime);
#ifdef _POSIX_C_SOURCE
		struct tm timeinfo;
		localtime_r(&rawtime, &timeinfo);
#else
		struct tm timeinfo;
		localtime_s(&timeinfo, &rawtime);
#endif

		/* You could potentially force the game to allow the player to
		 * reset the time by setting the RTC to invalid values.
		 *
		 * Using memset(&gb->cart_rtc, 0xFF, sizeof(gb->cart_rtc)) for
		 * example causes Pokemon Gold/Silver to say "TIME NOT SET",
		 * allowing the player to set the time without having some dumb
		 * password.
		 *
		 * The memset has to be done directly to gb->cart_rtc because
		 * gb_set_rtc() processes the input values, which may cause
		 * games to not detect invalid values.
		 */

		/* Set RTC. Only games that specify support for RTC will use
		 * these values. */
#ifdef _POSIX_C_SOURCE
		gb_set_rtc(&gb, &timeinfo);
#else
		gb_set_rtc(&gb, &timeinfo);
#endif
	}


#if ENABLE_LCD
	gb_init_lcd(&gb, &lcd_draw_line);
#endif

	/* Allow the joystick input even if game is in background. */
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	if(SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") < 0)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_INFO,
				"Unable to assign joystick mappings: %s\n",
				SDL_GetError());
	}

	/* Open the first available controller. */
	for(int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if(!SDL_IsGameController(i))
			continue;

		controller = SDL_GameControllerOpen(i);

		if(controller)
		{
			SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
					SDL_LOG_PRIORITY_INFO,
					"Game Controller %s connected.",
					SDL_GameControllerName(controller));
			break;
		}
		else
		{
			SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
					SDL_LOG_PRIORITY_INFO,
					"Could not open game controller %i: %s\n",
					i, SDL_GetError());
		}
	}

	{
		/* 12 for "Peanut-SDL: " and a maximum of 16 for the title. */
		char title_str[28] = "Peanut-SDL: ";
		gb_get_rom_name(&gb, title_str + 12);
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_INFO,
				"%s",
				title_str);
		SDL_SetWindowTitle(window, title_str);
	}

	SDL_SetWindowMinimumSize(window, LCD_WIDTH, LCD_HEIGHT);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if(renderer == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Could not create renderer: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	if(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) < 0)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Renderer could not draw color: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	if(SDL_RenderClear(renderer) < 0)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Renderer could not clear: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	SDL_RenderPresent(renderer);

	/* Use integer scale. */
	SDL_RenderSetLogicalSize(renderer, LCD_WIDTH, LCD_HEIGHT);
	SDL_RenderSetIntegerScale(renderer, 1);

	texture = SDL_CreateTexture(renderer,
				    SDL_PIXELFORMAT_RGB555,
				    SDL_TEXTUREACCESS_STREAMING,
				    LCD_WIDTH, LCD_HEIGHT);

	if(texture == NULL)
	{
		SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
				SDL_LOG_PRIORITY_CRITICAL,
				"Texture could not be created: %s",
				SDL_GetError());
		ret = EXIT_FAILURE;
		goto out;
	}

	auto_assign_palette(&priv, gb_colour_hash(&gb));

	while(SDL_QuitRequested() == SDL_FALSE)
	{
		int delay;
		static double rtc_timer = 0;
		static unsigned int selected_palette = 3;
		static unsigned int dump_bmp = 0;

		/* Calculate the time taken to draw frame, then later add a
		 * delay to cap at 60 fps. */
		old_ticks = SDL_GetTicks();

		/* Get joypad input. */
		while(SDL_PollEvent(&event))
		{
			static int fullscreen = 0;

			switch(event.type)
			{
			case SDL_QUIT:
				goto quit;

			case SDL_CONTROLLERBUTTONDOWN:
				switch(event.cbutton.button)
				{
				case SDL_CONTROLLER_BUTTON_A:
					gb.direct.joypad &= ~JOYPAD_A;
					break;

				case SDL_CONTROLLER_BUTTON_B:
					gb.direct.joypad &= ~JOYPAD_B;
					break;

				case SDL_CONTROLLER_BUTTON_BACK:
					gb.direct.joypad &= ~JOYPAD_SELECT;
					break;

				case SDL_CONTROLLER_BUTTON_START:
					gb.direct.joypad &= ~JOYPAD_START;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_UP:
					gb.direct.joypad &= ~JOYPAD_UP;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					gb.direct.joypad &= ~JOYPAD_RIGHT;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					gb.direct.joypad &= ~JOYPAD_DOWN;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					gb.direct.joypad &= ~JOYPAD_LEFT;
					break;
				}

				break;

			case SDL_CONTROLLERBUTTONUP:
				switch(event.cbutton.button)
				{
				case SDL_CONTROLLER_BUTTON_A:
					gb.direct.joypad |= JOYPAD_A;
					break;

				case SDL_CONTROLLER_BUTTON_B:
					gb.direct.joypad |= JOYPAD_B;
					break;

				case SDL_CONTROLLER_BUTTON_BACK:
					gb.direct.joypad |= JOYPAD_SELECT;
					break;

				case SDL_CONTROLLER_BUTTON_START:
					gb.direct.joypad |= JOYPAD_START;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_UP:
					gb.direct.joypad |= JOYPAD_UP;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					gb.direct.joypad |= JOYPAD_RIGHT;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					gb.direct.joypad |= JOYPAD_DOWN;
					break;

				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					gb.direct.joypad |= JOYPAD_LEFT;
					break;
				}

				break;

			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
				case SDLK_RETURN:
					gb.direct.joypad &= ~JOYPAD_START;
					break;

				case SDLK_BACKSPACE:
					gb.direct.joypad &= ~JOYPAD_SELECT;
					break;

				case SDLK_z:
					gb.direct.joypad &= ~JOYPAD_A;
					break;

				case SDLK_x:
					gb.direct.joypad &= ~JOYPAD_B;
					break;

				case SDLK_a:
					gb.direct.joypad ^= JOYPAD_A;
					break;

				case SDLK_s:
					gb.direct.joypad ^= JOYPAD_B;
					break;

				case SDLK_UP:
					gb.direct.joypad &= ~JOYPAD_UP;
					break;

				case SDLK_RIGHT:
					gb.direct.joypad &= ~JOYPAD_RIGHT;
					break;

				case SDLK_DOWN:
					gb.direct.joypad &= ~JOYPAD_DOWN;
					break;

				case SDLK_LEFT:
					gb.direct.joypad &= ~JOYPAD_LEFT;
					break;

				case SDLK_SPACE:
					fast_mode = 2;
					break;

				case SDLK_1:
					fast_mode = 1;
					break;

				case SDLK_2:
					fast_mode = 2;
					break;

				case SDLK_3:
					fast_mode = 3;
					break;

				case SDLK_4:
					fast_mode = 4;
					break;

				case SDLK_r:
					gb_reset(&gb);
					break;
#if ENABLE_LCD

				case SDLK_i:
					gb.direct.interlace = !gb.direct.interlace;
					break;

				case SDLK_o:
					gb.direct.frame_skip = !gb.direct.frame_skip;
					break;

				case SDLK_b:
					dump_bmp = ~dump_bmp;

					if(dump_bmp)
						SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
								SDL_LOG_PRIORITY_INFO,
								"Dumping frames");
					else
						SDL_LogMessage(LOG_CATERGORY_PEANUTSDL,
								SDL_LOG_PRIORITY_INFO,
								"Stopped dumping frames");

					break;
#endif

				case SDLK_p:
					
					break;
				}

				break;

			case SDL_KEYUP:
				switch(event.key.keysym.sym)
				{
				case SDLK_RETURN:
					gb.direct.joypad |= JOYPAD_START;
					break;

				case SDLK_BACKSPACE:
					gb.direct.joypad |= JOYPAD_SELECT;
					break;

				case SDLK_z:
					gb.direct.joypad |= JOYPAD_A;
					break;

				case SDLK_x:
					gb.direct.joypad |= JOYPAD_B;
					break;

				case SDLK_a:
					gb.direct.joypad |= JOYPAD_A;
					break;

				case SDLK_s:
					gb.direct.joypad |= JOYPAD_B;
					break;

				case SDLK_UP:
					gb.direct.joypad |= JOYPAD_UP;
					break;

				case SDLK_RIGHT:
					gb.direct.joypad |= JOYPAD_RIGHT;
					break;

				case SDLK_DOWN:
					gb.direct.joypad |= JOYPAD_DOWN;
					break;

				case SDLK_LEFT:
					gb.direct.joypad |= JOYPAD_LEFT;
					break;

				case SDLK_SPACE:
					fast_mode = 1;
					break;

				case SDLK_f:
					if(fullscreen)
					{
						SDL_SetWindowFullscreen(window, 0);
						fullscreen = 0;
						SDL_ShowCursor(SDL_ENABLE);
					}
					else
					{
						SDL_SetWindowFullscreen(window,		SDL_WINDOW_FULLSCREEN_DESKTOP);
						fullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
						SDL_ShowCursor(SDL_DISABLE);
					}
					break;

				case SDLK_F11:
				{
					if(fullscreen)
					{
						SDL_SetWindowFullscreen(window, 0);
						fullscreen = 0;
						SDL_ShowCursor(SDL_ENABLE);
					}
					else
					{
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
						fullscreen = SDL_WINDOW_FULLSCREEN;
						SDL_ShowCursor(SDL_DISABLE);
					}
				}
				break;
				}

				break;
			}
		}

		/* Execute CPU cycles until the screen has to be redrawn. */
		gb_run_frame(&gb);

		/* Tick the internal RTC when 1 second has passed. */
		rtc_timer += target_speed_ms / (double) fast_mode;

		if(rtc_timer >= 1000.0)
		{
			rtc_timer -= 1000.0;
			//gb_tick_rtc(&gb);
		}

		/* Skip frames during fast mode. */
		if(fast_mode_timer > 1)
		{
			fast_mode_timer--;
			/* We continue here since the rest of the logic in the
			 * loop is for drawing the screen and delaying. */
			continue;
		}

		fast_mode_timer = fast_mode;

#if ENABLE_SOUND_BLARGG
		/* Process audio. */
		audio_frame();
#endif

#if ENABLE_LCD
		/* Copy frame buffer to SDL screen. */
		SDL_UpdateTexture(texture, NULL, &priv.fb, LCD_WIDTH * sizeof(uint16_t));
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);



#endif

		/* Use a delay that will draw the screen at a rate of 59.7275 Hz. */
		new_ticks = SDL_GetTicks();

		/* Since we can only delay for a maximum resolution of 1ms, we
		 * can accumulate the error and compensate for the delay
		 * accuracy when the delay compensation surpasses 1ms. */
		speed_compensation += target_speed_ms - (new_ticks - old_ticks);

		/* We cast the delay compensation value to an integer, since it
		 * is the type used by SDL_Delay. This is where delay accuracy
		 * is lost. */
		delay = (int)(speed_compensation);

		/* We then subtract the actual delay value by the requested
		 * delay value. */
		speed_compensation -= delay;

		/* Only run delay logic if required. */
		if(delay > 0)
		{
			uint_fast32_t delay_ticks = SDL_GetTicks();
			uint_fast32_t after_delay_ticks;

			/* Tick the internal RTC when 1 second has passed. */
			rtc_timer += delay;

			if(rtc_timer >= 1000)
			{
				rtc_timer -= 1000;
				//gb_tick_rtc(&gb);

				/* If 60 seconds has passed, record save file.
				 * We do this because the blarrg audio library
				 * used contains asserts that will abort the
				 * program without save.
				 * TODO: Remove all workarounds due to faulty
				 * external libraries. */
				--save_timer;

				if(!save_timer)
				{
#if ENABLE_SOUND_BLARGG
					/* Locking the audio thread to reduce
					 * possibility of abort during save. */
					SDL_LockAudioDevice(dev);
#endif
					write_cart_ram_file(save_file_name,
						&priv.cart_ram,
						priv.save_size);
#if ENABLE_SOUND_BLARGG
					SDL_UnlockAudioDevice(dev);
#endif
					save_timer = 60;
				}
			}

			/* This will delay for at least the number of
			 * milliseconds requested, so we have to compensate for
			 * error here too. */
			//SDL_Delay(delay);

			after_delay_ticks = SDL_GetTicks();
			speed_compensation += (double)delay -
					      (int)(after_delay_ticks - delay_ticks);
		}
	}

quit:
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_DestroyTexture(texture);
	SDL_GameControllerClose(controller);
	SDL_Quit();
#ifdef ENABLE_SOUND_BLARGG
	audio_cleanup();
#endif

	/* Record save file. */
	write_cart_ram_file(save_file_name, &priv.cart_ram, priv.save_size);

out:
	SDL_free(priv.rom);
	SDL_free(priv.cart_ram);

	/* If the save file name was automatically generated (which required memory
	 * allocated on the help), then free it here. */
	if(argc == 2)
		SDL_free(save_file_name);

	if(argc == 1)
		SDL_free(rom_file_name);

	return ret;
}




















#else

















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
static mp_obj_t InitPeanut(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args) {

	int argindex = 0;

	mp_buffer_info_t errorNumBuffer;
	mp_get_buffer_raise(args[argindex++], &errorNumBuffer, MP_BUFFER_RW);
	int* errorNum = (int*)errorNumBuffer.buf;

	mp_buffer_info_t logBuffer;
	mp_get_buffer_raise(args[argindex++], &logBuffer, MP_BUFFER_RW);
	char* log = (char*)logBuffer.buf;

	mp_buffer_info_t cartRomDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRomDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRomData = (unsigned char*)cartRomDataBuffer.buf;

	mp_buffer_info_t saveFileSizeBuffer;
	mp_get_buffer_raise(args[argindex++], &saveFileSizeBuffer, MP_BUFFER_RW);
	unsigned int* saveFileSize = (unsigned int*)saveFileSizeBuffer.buf;

	PeanutInit(
		errorNum,
		log,
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

	mp_buffer_info_t logBuffer;
	mp_get_buffer_raise(args[argindex++], &logBuffer, MP_BUFFER_RW);
	char* log = (char*)logBuffer.buf;

	mp_buffer_info_t cartRomDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRomDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRomData = (unsigned char*)cartRomDataBuffer.buf;

	mp_buffer_info_t cartRamDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRamDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRamData = (unsigned char*)cartRamDataBuffer.buf;

	PeanutRun(
		log,
		LOG_BUFFER_SIZE,
		cartRomData,
		cartRamData,
		0xff);
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

	ResampleBuffer(color);
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



static char* messageLogPtr;

static struct gb_s gb;
static struct priv_t priv =
{
	.rom = NULL,
	.cart_ram = NULL
};
enum gb_init_error_e gb_ret;


unsigned char gb_rom_read(
	struct gb_s* gb,
	const uint_fast32_t addr)
{
	//const struct priv_t* const p = gb->direct.priv;
	return gb->direct.priv->rom[addr];
}
uint8_t gb_cart_ram_read(
	struct gb_s* gb,
	const uint_fast32_t addr)
{
	//const struct priv_t* const p = gb->direct.priv;
	return gb->direct.priv->cart_ram[addr];
}
void gb_cart_ram_write(
	struct gb_s* gb,
	const uint_fast32_t addr,
	const uint8_t val)
{
	//const struct priv_t* const p = gb->direct.priv;
	gb->direct.priv->cart_ram[addr] = val;
}
uint8_t gb_bootrom_read(
	struct gb_s* gb,
	const uint_fast16_t addr)
{
	//const struct priv_t* const p = gb->direct.priv;
	return gb->direct.priv->bootrom[addr];
}
void LogMessage(char* message)
{
	return;
	if (strlen(message) + 1 > LOG_BUFFER_SIZE)
	{
		strcpy_s(messageLogPtr, LOG_BUFFER_SIZE, message);
		messageLogPtr[LOG_BUFFER_SIZE-1] = '\0';
	}
	else
	{
		strcpy_s(messageLogPtr, strlen(message) + 1, message);
	}
}
void gb_error(
	struct gb_s* gb,
	const enum gb_error_e gb_err,
	const uint16_t addr)
{
	// TODO move error messages here
}
void lcd_draw_line(
	struct gb_s* gb,
	const uint8_t pixels[160],
	const uint_fast8_t line)
{
	//struct priv_t* priv = gb->direct.priv;

	for (unsigned int x = 0; x < LCD_WIDTH; x++)
	{
		gb->direct.priv->fb[line][x] = gb->direct.priv->selected_palette
			[(pixels[x] & LCD_PALETTE_ALL) >> 4]
			[pixels[x] & 3];
	}
}

void auto_assign_palette(struct priv_t* priv, uint8_t game_checksum)
{
	size_t palette_bytes = 3 * 4 * sizeof(uint16_t);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			memcpy(priv->selected_palette, palette, palette_bytes);
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
			LogMessage("No palette found.");
			memcpy(priv->selected_palette, palette, palette_bytes);
		}
	}
}

void PeanutInit(
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

	if (gb_get_save_size_s(&gb, &priv.save_size) < 0)
	{
		gb_ret = *errorNum;
		LogMessage("Unable to get save size.");
		return;
	}

	*saveFileSize = priv.save_size;

	gb_init_lcd(&gb, &lcd_draw_line);

	auto_assign_palette(&priv, gb_colour_hash(&gb));
}

void PeanutRun(
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

void ResampleBuffer(unsigned short* colorBuffer)
{
	for (int x = 0; x < DEVICE_WIDTH; ++x) {
		for (int y = 0; y < DEVICE_HEIGHT; ++y) {
			unsigned short pixelData = priv.fb[y][x];
			colorBuffer[x * DEVICE_HEIGHT + y] = pixelData;
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
unsigned short colorBuffer[DEVICE_WIDTH * DEVICE_HEIGHT * sizeof(unsigned short)];

#define main main  // Undo SDL main entry rename
int main()
{
	InitRenderer();
	char* logBuffer = malloc(LOG_BUFFER_SIZE);
	// Read cart rom and ram, ram size encoded in rom
	int errorNumber = 0;
	int saveFileSize = 0;

	LoadRom(&cartRomData, "Tetris.gb");

	PeanutInit(
		&errorNumber,
		logBuffer,
		LOG_BUFFER_SIZE,
		cartRomData,
		&saveFileSize);

	if (saveFileSize > 0)
	{
		LoadSave(&cartRamData, "Tetris.sav", saveFileSize);
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

#endif
