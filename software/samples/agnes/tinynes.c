#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "basesystem.h"
#include "core.h"
#include "vpu.h"
#include "dma.h"
#include "leds.h"
#include "task.h"
#include "keyboard.h"

#include "agnes.h"

#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 480

static void get_input(agnes_input_t *out_input);
static void* read_file(const char *filename, size_t *out_len);
static struct EVideoContext s_vx;
static uint32_t *s_framebuffer = NULL;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename.nes\n", argv[0]);
        return 1;
    }

    const char *ines_name = argv[1];

    size_t ines_data_size = 0;
    void* ines_data = read_file(ines_name, &ines_data_size);
    if (ines_data == NULL) {
        fprintf(stderr, "Reading %s failed.\n", ines_name);
        return 1;
    }
    
    agnes_t *agnes = agnes_make();
    if (agnes == NULL) {
        fprintf(stderr, "Making agnes failed.\n");
        return 1;
    }

    bool ok = agnes_load_ines_data(agnes, ines_data, ines_data_size);
    if (!ok) {
        fprintf(stderr, "Loading %s failed.\n", ines_name);
        return 1;
    }

	// Start video (single buffered for now)
	s_framebuffer = (uint32_t*)VPUAllocateBuffer(320 * AGNES_SCREEN_HEIGHT);
	VPUSetWriteAddress(&s_vx, (uint32_t)s_framebuffer);
	VPUSetScanoutAddress(&s_vx, (uint32_t)s_framebuffer);
	VPUSetDefaultPalette(&s_vx);

    s_vx.m_vmode = EVM_320_Wide;
    s_vx.m_cmode = ECM_8bit_Indexed;
	VPUSetVMode(&s_vx, EVS_Enable);
	VPUClear(&s_vx, 0x3F3F3F3F); // 63==black

	// Apply the NES color palette to our 12bit device
	agnes_color_t *palette = agnes_get_palette(agnes);
	for (uint32_t i=0; i<256; ++i)
		VPUSetPal(i, palette[i].r>>4, palette[i].g>>4, palette[i].b>>4);

    agnes_input_t input;

	uint32_t ledState = 0;
    while (true)
	{
		LEDSetState(ledState);

        get_input(&input);
        agnes_set_input(agnes, &input, NULL);
        agnes_next_frame(agnes);
		TaskYield();

		ledState ^= 0xFFFFFFFF;

		uint32_t source = agnes_get_raw_screen_buffer(agnes);
		for (uint32_t y = 0; y<AGNES_SCREEN_HEIGHT; ++y)
			__builtin_memcpy(s_framebuffer+80*y, (void*)(source+256*y), 256);
		CFLUSH_D_L1;
	}

    agnes_destroy(agnes);

    return 0;
}

static void get_input(agnes_input_t *out_input)
{
	static uint16_t enterdown = 0, tabdown = 0, leftdown = 0, rightdown = 0;
	static uint16_t updown = 0, downdown = 0, commadown = 0, perioddown = 0;

    memset(out_input, 0, sizeof(agnes_input_t));

	// Keyboard
	uint16_t *keystates = GetKeyStateTable();
	static uint32_t prevGen = 0xFFFFFFFF;
	uint32_t keyGen = GetKeyStateGeneration();
	// Do not read same generation twice
	if (keyGen != prevGen)
	{
		// Refresh key states
		for(int i=0; i<255; ++i)
		{
			uint16_t updown = keystates[i]&3;
			switch(i)
			{
				case HKEY_ENTER:	{ enterdown = updown&1; break; }
				case HKEY_TAB:		{ tabdown = updown&1; break; }
				case HKEY_A:		{ leftdown = updown&1; break; }
				case HKEY_D:		{ rightdown = updown&1; break; }
				case HKEY_W:		{ updown = updown&1; break; }
				case HKEY_S:		{ downdown = updown&1; break; }
				case HKEY_COMMA:	{ commadown = updown&1; break; }
				case HKEY_PERIOD:	{ perioddown = updown&1; break; }
				default:			break;
			}
		}
		prevGen = keyGen;
	}

	// Joystick
	int32_t *jposxy_buttons = (int32_t*)JOYSTICK_POS_AND_BUTTONS;

    if (commadown || jposxy_buttons[2]&0x20)	out_input->a = true;
    if (perioddown || jposxy_buttons[2]&0x40)	out_input->b = true;
    if (leftdown || jposxy_buttons[0]==0x00)	out_input->left = true;		// NOTE: 0x7F is 'centered' for direction buttons
    if (rightdown || jposxy_buttons[0]==0xFF)	out_input->right = true;
    if (updown || jposxy_buttons[1]==0x00)		out_input->up = true;
    if (downdown || jposxy_buttons[1]==0xFF)	out_input->down = true;
    if (enterdown || jposxy_buttons[3]&0x10)	out_input->select = true;
    if (tabdown || jposxy_buttons[3]&0x20)		out_input->start = true;
}

static void* read_file(const char *filename, size_t *out_len)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
	{
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    long pos = ftell(fp);
    if (pos < 0)
	{
        fclose(fp);
        return NULL;
    }
    size_t file_size = pos;
    rewind(fp);
    unsigned char *file_contents = (unsigned char *)malloc(file_size);
    if (!file_contents)
	{
        fclose(fp);
        return NULL;
    }
    if (fread(file_contents, file_size, 1, fp) < 1)
	{
        if (ferror(fp))
		{
            fclose(fp);
            free(file_contents);
            return NULL;
        }
    }
    fclose(fp);
    *out_len = file_size;
    return file_contents;
}
