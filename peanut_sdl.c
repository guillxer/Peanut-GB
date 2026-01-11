
#define SCREEN_SCALE 1
#define UPSCALE 1
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
#include "py/runtime.h"

mp_obj_t romReadCallback = MP_OBJ_NULL;

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
	window = SDL_CreateWindow("SDL Draw Line", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DEVICE_WIDTH * UPSCALE, DEVICE_HEIGHT * UPSCALE, SDL_WINDOW_SHOWN);
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
	unsigned int* saveFileSize,
	unsigned int* romFSData,
	unsigned char* bankTableData);
void PeanutRunImpl(
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned char* cartRamData,
	unsigned char controllerInput);
void ResampleBufferImpl(
	unsigned short* colorBuffer,
	unsigned short* scratchBuffer,
	int* settingsData);
void InitAPUImpl(
	int* numSamples);
void AudioCallbackImpl(
	short* audioBuffer,
	unsigned short* dmaBuffer,
	int pwmTOP, 
	int bufferIndex,
	int mixMode);
void FillSaveStateImpl(
	unsigned char* saveStateData);
void SetSaveStateImpl(
	unsigned char* saveStateData);
void SetPaletteImpl(
	unsigned char selectionIndex);

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

	mp_buffer_info_t saveFileSizeBuffer;
	mp_get_buffer_raise(args[argindex++], &saveFileSizeBuffer, MP_BUFFER_RW);
	unsigned int* saveFileSize = (unsigned int*)saveFileSizeBuffer.buf;

	mp_buffer_info_t romFSBuffer;
	mp_get_buffer_raise(args[argindex++], &romFSBuffer, MP_BUFFER_RW);
	unsigned int* romFSData = (unsigned int*)romFSBuffer.buf;

	mp_buffer_info_t bankTableBuffer;
	mp_get_buffer_raise(args[argindex++], &bankTableBuffer, MP_BUFFER_RW);
	unsigned char* bankTableData = (unsigned char*)bankTableBuffer.buf;
	
	char* msg = NULL;
	int errorNum = 0;

	PeanutInitImpl(
		&errorNum,
		msg,
		LOG_BUFFER_SIZE,
		cartRomData,
		saveFileSize,
		romFSData,
		bankTableData);
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
	
	mp_buffer_info_t controllerBuffer;
	mp_get_buffer_raise(args[argindex++], &controllerBuffer, MP_BUFFER_RW);
	unsigned char* controllerData = (unsigned char*)controllerBuffer.buf;

	mp_buffer_info_t cartRamDataBuffer;
	mp_get_buffer_raise(args[argindex++], &cartRamDataBuffer, MP_BUFFER_RW);
	unsigned char* cartRamData = (unsigned char*)cartRamDataBuffer.buf;

	romReadCallback = args[argindex++];
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
	
	mp_buffer_info_t scratchBuffer;
	mp_get_buffer_raise(args[argindex++], &scratchBuffer, MP_BUFFER_RW);
	unsigned short* scratchData = (unsigned short*)scratchBuffer.buf;

	mp_buffer_info_t settingsBuffer;
	mp_get_buffer_raise(args[argindex++], &settingsBuffer, MP_BUFFER_RW);
	int* settingsData = (int *)settingsBuffer.buf;

	ResampleBufferImpl(
		color,
		scratchData,
		settingsData);
	return mp_const_none;
}

static mp_obj_t InitAPU(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	mp_buffer_info_t numSamplesBuffer;
	mp_get_buffer_raise(args[argindex++], &numSamplesBuffer, MP_BUFFER_RW);
	int* numSamples = (int*)numSamplesBuffer.buf;
	
	InitAPUImpl(
		numSamples);

	return mp_const_none;
}
static mp_obj_t SampleAPU(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	mp_buffer_info_t audioSampleBuffer;
	mp_get_buffer_raise(args[argindex++], &audioSampleBuffer, MP_BUFFER_RW);
	short* audioSampleData = audioSampleBuffer.buf;
	
	mp_buffer_info_t audioDMABuffer;
	mp_get_buffer_raise(args[argindex++], &audioDMABuffer, MP_BUFFER_RW);
	unsigned short* audioDMAData = audioDMABuffer.buf;
	
	mp_buffer_info_t samplingSettingsBuffer;
	mp_get_buffer_raise(args[argindex++], &samplingSettingsBuffer, MP_BUFFER_RW);
	int* samplingSettings = samplingSettingsBuffer.buf;
	int pwmTOP = samplingSettings[0];
	int bufferIndex = samplingSettings[1];
	
	int mixMode = 0;
	
	AudioCallbackImpl(
		audioSampleData,
		audioDMAData,
		pwmTOP,
		bufferIndex,
		mixMode);
	
	return mp_const_none;
}
static mp_obj_t FillSaveState(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	mp_buffer_info_t saveStateBuffer;
	mp_get_buffer_raise(args[argindex++], &saveStateBuffer, MP_BUFFER_RW);
	unsigned char* saveStateData = saveStateBuffer.buf;
	
	FillSaveStateImpl(saveStateData);
	
	return mp_const_none;
}
static mp_obj_t SetSaveState(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;

	mp_buffer_info_t saveStateBuffer;
	mp_get_buffer_raise(args[argindex++], &saveStateBuffer, MP_BUFFER_RW);
	unsigned char* saveStateData = saveStateBuffer.buf;
	
	SetSaveStateImpl(saveStateData);
	
	return mp_const_none;
}

static mp_obj_t SetPalette(
	mp_obj_fun_bc_t* self,
	size_t n_args,
	size_t n_kw,
	mp_obj_t* args)
{
	int argindex = 0;
	
	mp_buffer_info_t paletteIndexBuffer;
	mp_get_buffer_raise(args[argindex++], &paletteIndexBuffer, MP_BUFFER_RW);
	unsigned char* paletteIndexData = paletteIndexBuffer.buf;
	
	SetPaletteImpl(paletteIndexData[0]);
	
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
	mp_store_global(
		MP_QSTR_InitAPU,
		MP_DYNRUNTIME_MAKE_FUNCTION(InitAPU));
	mp_store_global(
		MP_QSTR_SampleAPU,
		MP_DYNRUNTIME_MAKE_FUNCTION(SampleAPU));
	mp_store_global(
		MP_QSTR_FillSaveState,
		MP_DYNRUNTIME_MAKE_FUNCTION(FillSaveState));
	mp_store_global(
		MP_QSTR_SetSaveState,
		MP_DYNRUNTIME_MAKE_FUNCTION(SetSaveState));
	mp_store_global(
		MP_QSTR_SetPalette,
		MP_DYNRUNTIME_MAKE_FUNCTION(SetPalette));
	MP_DYNRUNTIME_INIT_EXIT
}
#endif

uint8_t audio_read(uint16_t addr);
void audio_write(uint16_t addr, uint8_t val);

#include "peanut_gb.h"
#include "minigb_apu.h"

char* messageLogPtr;

struct gb_s gb;
struct priv_t priv =
{
	.rom = NULL,
	.cart_ram = NULL
};
enum gb_init_error_e gb_ret;

struct minigb_apu_ctx apu;

#if !RP2
// ~ 550 * sizeof(unsigned int) bytes
audio_sample_t audioSamples[AUDIO_SAMPLES_TOTAL * sizeof(audio_sample_t)];
#endif

void audio_callback(void* ptr, audio_sample_t* data, unsigned short* dmaData, int pwmTOP, int bufferIndex, int mixMode);

uint8_t audio_read(uint16_t addr)
{
	return minigb_apu_audio_read(&apu, addr);
}
void audio_write(uint16_t addr, uint8_t val)
{
	minigb_apu_audio_write(&apu, addr, val);
}
void audio_callback(void* ptr, audio_sample_t* data, unsigned short* dmaData, int pwmTOP, int bufferIndex, int mixMode)
{
	minigb_apu_audio_callback(&apu, data, dmaData, pwmTOP, bufferIndex, mixMode);
}
#define numTableEntries 4
#define bankSize (16 * 1024)
#define bankShift 14
#define bankAddressMask 0x3fff
volatile unsigned int* romFSPipe;
volatile unsigned char* bankTable;
int bankIDArray[numTableEntries];
int bankTableIDArray[numTableEntries];
char lifoLRU[numTableEntries];
int lastBankIndex;
int lastTableIndex;

unsigned int AddressToBankID(unsigned int address)
{
	return address >> bankShift;
}
unsigned int AddressToBankAddress(unsigned int address)
{
	return address & bankAddressMask;
}
int GetLRUTableID()
{
	return lifoLRU[numTableEntries - 1];
}
inline void UpdateLRU(unsigned int tableID)
{
	bool bShift = false;
	for (int i = numTableEntries - 1; i >= 1; --i)
	{
		if (lifoLRU[i] == tableID)
		{
			bShift = true;
		}
		if (bShift)
		{
			lifoLRU[i] = lifoLRU[i - 1];
		}
	}
	lifoLRU[0] = tableID;
}
int GetTableID(unsigned int bankID)
{
    int tableID = -1;
    for (int i = 0; i < numTableEntries; ++i)
	{
		int tableBankID = bankTableIDArray[i];
        if ((int)bankID == tableBankID)
		{
			tableID = i;
            break;
		}
	}
    return tableID;
}
#if 0
int GetLRUTableID()
{
    int ittrIndex = 0;
    int lruIndex = 0;
    int maxCounterValue = -1024 * 1024 * 1024;
    for (int i = 0; i < numTableEntries; ++i)
	{
		int countSinceUsed = bankLastUsedCounterArray[i];
        if (countSinceUsed > maxCounterValue)
		{
            lruIndex = ittrIndex;
            maxCounterValue = countSinceUsed;
		}
        ittrIndex += 1;
	}
    return lruIndex;
}
void UpdateLRUCounters(unsigned int tableID)
{
    bankLastUsedCounterArray[tableID] -= 1;
}
#endif
unsigned char GetDataInTable(unsigned int tableID, unsigned int address)
{
    int bankAddress = AddressToBankAddress(address);
    return bankTable[tableID * bankSize + bankAddress];
}
void FillTableEntry(int tableID, unsigned int bankID)
{
	romFSPipe[1] = bankID;
	romFSPipe[2] = tableID;
	romFSPipe[0] = 1;
	while (romFSPipe[0] != 0){}
    bankTableIDArray[tableID] = bankID;
}
unsigned char GetRomData(unsigned int address)
{
    unsigned int bankID = AddressToBankID(address);
	if (bankID != lastBankIndex)
	{
		int tableID = GetTableID(bankID);
		if (tableID < 0)
		{
			tableID = GetLRUTableID();
			FillTableEntry(tableID, bankID);
		}
		UpdateLRU(tableID);
		lastBankIndex = bankID;
		lastTableIndex = tableID;
	}
    return GetDataInTable(lastTableIndex, address);
}
unsigned char RomReadFromFS(const uint_fast32_t addr)
{
	// Callbacks are not working with this MP firmware
	// Instead update missing cached banks in Core1 loop
	// only when the emulatore is running
	unsigned char data = GetRomData(addr);
	return data;
}
void InitBankCache()
{
	for (int i = 0; i < numTableEntries; ++i)
	{
		bankIDArray[i] = -1;
		bankTableIDArray[i] = -1;
		lifoLRU[i] = i;
	}
	lastBankIndex = -1;
	lastTableIndex = -1;
}
unsigned char gb_rom_read(
	struct gb_s* gb,
	const uint_fast32_t addr)
{
	const struct priv_t* const p = gb->direct.priv;
	unsigned char romValue = 0;
	if (addr < ((1024 + 512) << 10))
	{
		romValue = p->rom[addr];
	}
	else
	{
		romValue = RomReadFromFS(addr);
	}
	return romValue;
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
#if RP2
__attribute__((optimize("O0")))
#endif
void SetSaveStateImpl(unsigned char* saveStateData)
{
	// Set gb state
	
	// Save off function pointers
	void* gb_rom_read = gb.gb_rom_read;
	void* gb_cart_ram_read = gb.gb_cart_ram_read;
	void* gb_cart_ram_write = gb.gb_cart_ram_write;
	void* gb_error = gb.gb_error;
	struct priv_t* priv = gb.direct.priv;

	void* gb_serial_tx = gb.gb_serial_tx;
	void* gb_serial_rx = gb.gb_serial_rx;
	
	void* lcd_draw_line = gb.display.lcd_draw_line;

	void* gb_bootrom_read = gb.gb_bootrom_read;

	unsigned char* gbBytes = (unsigned char*)&gb;
	int gbSize = sizeof(gb);
	for (int i = 0; i < gbSize; ++i)
	{
		gbBytes[i] = saveStateData[i];
	}
		
	gb.gb_rom_read = gb_rom_read;
	gb.gb_cart_ram_read = gb_cart_ram_read;
	gb.gb_cart_ram_write = gb_cart_ram_write;
	gb.gb_error = gb_error;
	gb.direct.priv = priv;

	gb.gb_serial_tx = gb_serial_tx;
	gb.gb_serial_rx = gb_serial_rx;
	
	gb.display.lcd_draw_line = lcd_draw_line;

	gb.gb_bootrom_read = gb_bootrom_read;
}
#if RP2
__attribute__((optimize("O0")))
#endif
void FillSaveStateImpl(unsigned char* saveStateData)
{
	// Copy gb state
	unsigned char* gbBytes = (unsigned char*)(void*)&gb;
	int gbSize = sizeof(gb);
	for (int i = 0; i < gbSize; ++i)
	{
		saveStateData[i] = gbBytes[i];
	}
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

void auto_assign_palette(struct priv_t* priv, uint8_t game_checksum);

uint8_t game_checksum_saved;
void select_palette(
	struct priv_t* priv, 
	unsigned char selection_index)
{
	switch (selection_index)
	{
		case 0:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 1:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 2:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7f77, 0x6650, 0x41a4, 0x28a0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 3:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 4:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 5:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 6:{
			const uint16_t palette[3][4] =
			{
				{ 0x7ff4, 0x7e31, 0x463f, 0x0, },
				{ 0x7ff4, 0x7e31, 0x463f, 0x0, },
				{ 0x7ff4, 0x7e31, 0x463f, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 7:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 8:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x7fe0, 0x3900, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 9:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 10:{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be5, 0x197, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 11:{
			const uint16_t palette[3][4] =
			{
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 12:{
			auto_assign_palette(priv, game_checksum_saved);
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
		}
	}
}

void auto_assign_palette(struct priv_t* priv, uint8_t game_checksum)
{
	game_checksum_saved = game_checksum;
	//size_t palette_bytes = 3 * 4 * sizeof(uint16_t);
	switch (game_checksum)
	{
		case 0xB3:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7da0, 0x4500, 0x0, },
				{ 0x7fff, 0x7da0, 0x4500, 0x0, },
				{ 0x7fff, 0x56b0, 0x21ae, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x59:
		case 0xC6:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7da0, 0x4500, 0x0, },
				{ 0x7fff, 0x2adf, 0x7c00, 0x1f, },
				{ 0x7fff, 0x56b0, 0x21ae, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x8C:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7ff2, 0x46df, 0x322d, 0xe7, },
				{ 0x7ff2, 0x46df, 0x322d, 0xe7, },
				{ 0x7ff2, 0x46df, 0x322d, 0xe7, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x86:
		case 0xA8:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7ee8, 0x7f40, 0x44e0, 0x2000, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7ff2, 0x46df, 0x322d, 0xe7, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xBF:
		case 0xCE:
		case 0xD1:
		case 0xF0:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7fff, 0x329f, 0x1f, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x37e0, 0x7fff, 0x7d28, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x36:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7fff, 0x329f, 0x1f, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x2740, 0x7e00, 0x7fe0, 0x7fff, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x34:
		case 0x66:
		case 0xF4:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be0, 0x59a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x3D:
		case 0x6A:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x95:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
				{ 0x7fff, 0x2adf, 0x7c00, 0x1f, },
				{ 0x7fff, 0x27e0, 0x7d00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x71:
		case 0xFF:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x19:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x3E:
		case 0xE0:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
				{ 0x7fff, 0x2adf, 0x7c00, 0x1f, },
				{ 0x7fff, 0x7e40, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x15:
		case 0xDB:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x0D:
		case 0x69:
		case 0xF2:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
				{ 0x7fff, 0x2adf, 0x7c00, 0x1f, },
				{ 0x7fff, 0x7fe0, 0x7c00, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x88:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x525f, 0x7fe0, 0x180, 0x0, },
				{ 0x525f, 0x7fe0, 0x180, 0x0, },
				{ 0x525f, 0x7fe0, 0x180, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x1D:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7d89, 0x6800, 0x3000, 0x0, },
				{ 0x7d89, 0x6800, 0x3000, 0x0, },
				{ 0x525f, 0x7fe0, 0x180, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x27:
		case 0x49:
		case 0x5C:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7d89, 0x6800, 0x3000, 0x0, },
				{ 0x1f, 0x7fff, 0x7fee, 0x21f, },
				{ 0x525f, 0x7fe0, 0x180, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xC9:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7da0, 0x4500, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7ff9, 0x33bd, 0x4a05, 0x294a, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x46:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x0, 0x7fff, 0x7e10, 0x44e7, },
				{ 0x0, 0x7fff, 0x7e10, 0x44e7, },
				{ 0x5adf, 0x7ff1, 0x5548, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x61:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x3C:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x4E:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7fee, 0x21f, 0x7c00, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x9C:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
				{ 0x7ee8, 0x7f40, 0x44e0, 0x2000, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x18:
		case 0x6B:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7ee8, 0x7f40, 0x44e0, 0x2000, },
				{ 0x7fff, 0x2adf, 0x7c00, 0x1f, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xD3:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x9D:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x463a, 0x2531, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x28:
		case 0x4B:
		case 0x90:
		case 0x9A:
		case 0xBD:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x17:
		case 0x8B:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x39:
		case 0x43:
		case 0x97:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
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
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x14:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x70:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x3e0, 0x1600, 0x100, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x0C:
		case 0x16:
		case 0x35:
		case 0x67:
		case 0x75:
		case 0x92:
		case 0x99:
		case 0xB7:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xA5:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xA2:
		case 0xF7:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x3be5, 0x200, 0x0, },
				{ 0x7fff, 0x329f, 0x1f, 0x0, },
				{ 0x7fff, 0x7eac, 0x40a0, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xE8:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
				{ 0x0, 0x210, 0x7f40, 0x7fff, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x58:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
				{ 0x7fff, 0x5294, 0x2529, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x6F:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7f20, 0x4980, 0x0, },
				{ 0x7fff, 0x7f20, 0x4980, 0x0, },
				{ 0x7fff, 0x7f20, 0x4980, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0xAA:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be5, 0x197, 0x0, },
				{ 0x7fff, 0x3be5, 0x197, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}
		case 0x00:
		case 0x3F:
		{
			const uint16_t palette[3][4] =
			{
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x7e10, 0x44e7, 0x0, },
				{ 0x7fff, 0x3be5, 0x197, 0x0, },
			};
		   for (int x = 0; x < 3; ++x) {
				for (int y = 0; y < 4; ++y) {
					priv->selected_palette[x][y] = palette[x][y];
				}
			}
			break;
		}

#if 0
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

		/* Mega Man [1/2/3] */
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
#endif

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
			//memcpy(priv->selected_palette, palette, palette_bytes);
		}
	}
}

void SetPaletteImpl(
	unsigned char selectionIndex)
{
	select_palette(&priv, selectionIndex);
}

void PeanutInitImpl(
	int* errorNum,
	char* logBuffer,
	int logBufferSize,
	unsigned char* cartRomData,
	unsigned int* saveFileSize,
	unsigned int* romFSData,
	unsigned char* bankTableData)
{
	messageLogPtr = logBuffer;
	
	romFSPipe = romFSData;
	
	bankTable = bankTableData;
	
	InitBankCache();

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

	if (gb_get_save_size_s(&gb, &priv.save_size) < 0)
	{
		gb_ret = *errorNum;
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

void InitAPUImpl(int* numSamples)
{
	minigb_apu_audio_init(&apu, numSamples);
}

void AudioCallbackImpl(
	short* bufferArg, 
	unsigned short* dmaData, 
	int pwmTOP, 
	int bufferIndex, 
	int mixMode)
{
	minigb_apu_audio_callback(&apu, bufferArg, dmaData, pwmTOP, bufferIndex, mixMode);
}

typedef struct 
{
	float x;
	float y;
	float z;
} Vec3;

typedef struct 
{
	int x;
	int y;
	int z;
} Vec3i;

Vec3 Color555ToVec3(unsigned short color555)
{
	float red = (float)((color555 >> 10) & 0x1f) / 31.0f;
	float green = (float)((color555 >> 5) & 0x1f) / 31.0f;
	float blue = (float)((color555) & 0x1f) / 31.0f;
	Vec3 vecRGB = { red, green, blue };
	return vecRGB;
}

Vec3 Color565ToVec3(unsigned short color565)
{
	float red = (float)((color565 >> 11) & 0x1f) / 31.0f;
	float green = (float)((color565 >> 5) & 0x3f) / 63.0f;
	float blue = (float)((color565) & 0x1f) / 31.0f;
	Vec3 vecRGB = { red, green, blue };
	return vecRGB;
}

Vec3i Color555ToVec3i(unsigned short color555)
{
	int red = ((color555 >> 10) & 0x1f);
	int green = ((color555 >> 5) & 0x1f);
	int blue = ((color555) & 0x1f);
	Vec3i vecRGB = { red, green, blue };
	return vecRGB;
}

Vec3i Color565ToVec3i(unsigned short color565)
{
	int red = ((color565 >> 11) & 0x1f);
	int green = ((color565 >> 5) & 0x3f);
	int blue = ((color565) & 0x1f);
	Vec3i vecRGB = { red, green, blue };
	return vecRGB;
}

unsigned short Vec3ToColor565(Vec3 vecRGB)
{
	unsigned short color565 = 0x0;
	color565 = ((int)(vecRGB.x * 31.f) & 0x1f) << 11;
	color565 |= ((int)(vecRGB.y * 63.f) & 0x3f) << 5;
	color565 |= ((int)(vecRGB.z * 31.f) & 0x1f);
	return color565;
}

unsigned short Vec3ToColor555(Vec3 vecRGB)
{
	unsigned short color555 = 0x0;
	color555 = ((int)(vecRGB.x * 31.f) & 0x1f) << 10;
	color555 |= ((int)(vecRGB.y * 31.f) & 0x1f) << 5;
	color555 |= ((int)(vecRGB.z * 31.f) & 0x1f);
	return color555;
}

unsigned short Vec3iToColor555(Vec3i vecRGB)
{
	unsigned short color555 = 0x0;
	color555 = (vecRGB.x & 0x1f) << 10;
	color555 |= (vecRGB.y & 0x1f) << 5;
	color555 |= (vecRGB.z & 0x1f);
	return color555;
}

unsigned short Vec3iToColor565(Vec3i vecRGB)
{
	unsigned short color565 = 0x0;
	color565 = (vecRGB.x & 0x1f) << 11;
	color565 |= (vecRGB.y & 0x3f) << 5;
	color565 |= (vecRGB.z & 0x1f);
	return color565;
}

unsigned short Color555ToColor565(unsigned short color555)
{
	int red = ((color555 >> 10) & 0x1f);
	int green = ((color555 >> 5) & 0x1f);
	int blue = ((color555) & 0x1f);
	unsigned short color565 = 0x0;
	color565 = red << 11;
	color565 |= (green << 1) << 5;
	color565 |= blue;
	return color565;
}

static inline float fclampf(float x, float min_val, float max_val) {
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

#include <math.h>
#define maxi(a, b) (((a) > (b)) ? (a) : (b))
#define mini(a, b) (((a) < (b)) ? (a) : (b))
void ResampleBufferImpl(
	unsigned short* colorBuffer,
	unsigned short* scratchBuffer,
	int* settingsData)
{
	int scalingType = settingsData[0];
	int offsetX = settingsData[1];
	int offsetY = settingsData[2];

	float aspect = (float)EMU_HEIGHT / (float)EMU_WIDTH;
	int trimDeviceHeight = (int)(aspect * (float)DEVICE_HEIGHT);
	int marginDeviceY = DEVICE_HEIGHT - trimDeviceHeight;

	if (scalingType == 0)
	{
		#if !RP2
		// Buffer already zerored by RP2
		// Here loop unrolling will exhaust stack with GCC
		for (int y = 0; y < marginDeviceY; ++y)
		{
			for (int x = 0; x < DEVICE_WIDTH; ++x)
			{
				colorBuffer[y * DEVICE_WIDTH + x] = 0x00;
			}
		}
		#endif
		int blendScales[4][2] = {
			{4, 1}, {3, 2}, {2, 3}, {1, 4}
		};
		const float weightAndNormalizeFactor = 1.0f / 5.0f / 31.0f;
		
		for (int y = 0; y < EMU_HEIGHT; ++y)
		{
			int sourceX = 0;
			for (int x = 0; x < DEVICE_WIDTH; x += 4)
			{
				Vec3i vecRGB = { 0, 0, 0 };
				for (int i = 0; i < 4; ++i)
				{
					Vec3i leftSample = Color555ToVec3i(priv.fb[y][sourceX + i + 0]);
					Vec3i rightSample = Color555ToVec3i(priv.fb[y][sourceX + i + 1]);
					vecRGB.x = leftSample.x * blendScales[i][0];
					vecRGB.y = leftSample.y * blendScales[i][0];
					vecRGB.z = leftSample.z * blendScales[i][0];
					vecRGB.x += rightSample.x * blendScales[i][1];
					vecRGB.y += rightSample.y * blendScales[i][1];
					vecRGB.z += rightSample.z * blendScales[i][1];
					Vec3 vecRGBFloat = { 0, 0, 0 };
					vecRGBFloat.x = (float)vecRGB.x * weightAndNormalizeFactor;
					vecRGBFloat.y = (float)vecRGB.y * weightAndNormalizeFactor;
					vecRGBFloat.z = (float)vecRGB.z * weightAndNormalizeFactor;
					scratchBuffer[y * DEVICE_WIDTH + x + i] = Vec3ToColor555(vecRGBFloat);
				}
				sourceX += 5;
			}
		}
		for (int x = 0; x < DEVICE_WIDTH; ++x)
		{
			int sourceY = 0;
			for (int y = 0; y < trimDeviceHeight; y += 4)
			{
				Vec3i vecRGB = { 0, 0, 0 };
				for (int i = 0; i < 4; ++i)
				{
					if ((y + marginDeviceY + i) >= DEVICE_HEIGHT)
					{
						break;
					}
					Vec3i leftSample = Color555ToVec3i(scratchBuffer[(sourceY + i + 0) * DEVICE_WIDTH + x]);
					Vec3i rightSample = Color555ToVec3i(scratchBuffer[(sourceY + i + 1) * DEVICE_WIDTH + x]);
					vecRGB.x = leftSample.x * blendScales[i][0];
					vecRGB.y = leftSample.y * blendScales[i][0];
					vecRGB.z = leftSample.z * blendScales[i][0];
					vecRGB.x += rightSample.x * blendScales[i][1];
					vecRGB.y += rightSample.y * blendScales[i][1];
					vecRGB.z += rightSample.z * blendScales[i][1];
					Vec3 vecRGBFloat = { 0, 0, 0 };
					vecRGBFloat.x = (float)vecRGB.x * weightAndNormalizeFactor;
					vecRGBFloat.y = (float)vecRGB.y * weightAndNormalizeFactor;
					vecRGBFloat.z = (float)vecRGB.z * weightAndNormalizeFactor;
					colorBuffer[(y + marginDeviceY + i) * DEVICE_WIDTH + x] = Vec3ToColor565(vecRGBFloat);
				}
				sourceY += 5;
			}
		}
	}
	else
	{
		int maxDiffX = EMU_WIDTH - DEVICE_WIDTH;
		int maxDiffY = EMU_HEIGHT - DEVICE_HEIGHT;
		offsetX = maxi(0, mini(offsetX, maxDiffX));
		offsetY = maxi(0, mini(offsetY, maxDiffY));
		for (int y = 0; y < DEVICE_HEIGHT; ++y) 
			{
			for (int x = 0; x < DEVICE_WIDTH; ++x) 
			{
				colorBuffer[y * DEVICE_WIDTH + x] = Color555ToColor565(priv.fb[y + offsetY][x + offsetX]);
			}
		}
	}
}

#if !RP2
void Present(unsigned short* colorBuffer)
{
	for (int x = 0; x < DEVICE_WIDTH * UPSCALE; ++x)
	{
		for (int y = 0; y < DEVICE_HEIGHT * UPSCALE; ++y)
		{
			unsigned int color565 = colorBuffer[x/ UPSCALE * DEVICE_HEIGHT + y/ UPSCALE];
			float red = (float)((color565 >> 11) & 0x1F) / 31.0f;
			float green = (float)((color565 >> 5) & 0x3F) / 63.0f;
			float blue = (float)((color565) & 0x1F) / 31.0f;
			SDL_SetRenderDrawColor(renderer, 255.0f * red, 255.0f * green, 255.0f * blue, 255);
			SDL_RenderDrawPoint(renderer, y, x);
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
#define COLOR_BUFFER_SIZE DEVICE_WIDTH * DEVICE_HEIGHT * sizeof(unsigned short)
unsigned short deviceColorBuffer[COLOR_BUFFER_SIZE];

void Save(int saveSize)
{
	if (cartRamData)
	{
		FILE* saveFile = NULL;
		char* name = "PokemonRed.sav";
		int err = fopen_s(&saveFile, name, "wb");
		assert(err == 0);
		fwrite(cartRamData, sizeof(unsigned char), saveSize, saveFile);
		fclose(saveFile);
	}
}

#define main main  // Undo SDL main entry rename
int main()
{
	memset(deviceColorBuffer, 0, COLOR_BUFFER_SIZE);

	InitRenderer();
	// Read cart rom and ram, ram size encoded in rom
	int errorNumber = 0;
	unsigned int saveFileSize = 0;
	char* msg = NULL;
	int errorNum = 0;

	LoadRom(&cartRomData, "PokemonRed.gb");

	PeanutInitImpl(
		&errorNum,
		msg,
		LOG_BUFFER_SIZE,
		cartRomData,
		&saveFileSize);

	if (saveFileSize > 0)
	{
		LoadSave(
			&cartRamData,
			"PokemonRed.sav",
			saveFileSize);
	}

	minigb_apu_audio_init(&apu);

	int displaySettings[3] = {0, 0, 0};

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

					case SDLK_1:
						displaySettings[0] = 0;
						break;
					case SDLK_2:
						displaySettings[0] = 1;
						break;
					case SDLK_3:
						displaySettings[0] = 2;
						break;
				}
			}
			break;
		}

		PeanutRunImpl(
			msg,
			LOG_BUFFER_SIZE,
			cartRomData,
			cartRamData,
			controllerInput);

		ResampleBufferImpl(
			deviceColorBuffer,
			displaySettings);
		Present(deviceColorBuffer);
		UpdateFrame();

		
		audio_callback(NULL, audioSamples, 0);
	}
	SDL_Quit();
	exit(0);
	return 0;
}
#endif
