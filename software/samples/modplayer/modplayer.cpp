#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "apu.h"
#include "vpu.h"

#include "xm.h"
#include "micromod.h"

#define SAMPLING_FREQ  44100	// 44.1khz
#define REVERB_BUF_LEN 4110		// 100ms
#define OVERSAMPLE     2		// 2x oversampling
#define NUM_CHANNELS   2		// Stereo
#define BUFFER_SAMPLES 1024		// buffer size (max: 2048 bytes i.e. 1024 words)
#define BUFFER_SAMPLES_F 2048

static short mix_buffer[ BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE ];
static short reverb_buffer[ REVERB_BUF_LEN ];
float *mix_buffer_f;
static short *apubuffer;
static long reverb_len = REVERB_BUF_LEN, reverb_idx = 0, filt_l = 0, filt_r = 0;
static long samples_remaining = 0;

static EVideoContext vx;
static EVideoSwapContext sc;

/*
	2:1 downsampling with simple but effective anti-aliasing.
	Count is the number of stereo samples to process, and must be even.
	input may point to the same buffer as output.
*/
static void downsample( short *input, short *output, long count ) {
	long in_idx, out_idx, out_l, out_r;
	in_idx = out_idx = 0;
	while( out_idx < count ) {	
		out_l = filt_l + ( input[ in_idx++ ] >> 1 );
		out_r = filt_r + ( input[ in_idx++ ] >> 1 );
		filt_l = input[ in_idx++ ] >> 2;
		filt_r = input[ in_idx++ ] >> 2;
		output[ out_idx++ ] = out_l + filt_l;
		output[ out_idx++ ] = out_r + filt_r;
	}
}

static void downsample_f( float *input, short *output, long count )
{
	long in_idx, out_idx, out_l, out_r;
	in_idx = out_idx = 0;
	while( out_idx < count )
	{
		output[out_idx++] = (short)(input[in_idx++]*32768.f);
		output[out_idx++] = (short)(input[in_idx++]*32768.f);
	}
}

/* Simple stereo cross delay with feedback. */
static void reverb( short *buffer, long count ) {
	long buffer_idx, buffer_end;
	if( reverb_len > 2 ) {
		buffer_idx = 0;
		buffer_end = buffer_idx + ( count << 1 );
		while( buffer_idx < buffer_end ) {
			buffer[ buffer_idx ] = ( buffer[ buffer_idx ] * 3 + reverb_buffer[ reverb_idx + 1 ] ) >> 2;
			buffer[ buffer_idx + 1 ] = ( buffer[ buffer_idx + 1 ] * 3 + reverb_buffer[ reverb_idx ] ) >> 2;
			reverb_buffer[ reverb_idx ] = buffer[ buffer_idx ];
			reverb_buffer[ reverb_idx + 1 ] = buffer[ buffer_idx + 1 ];
			reverb_idx += 2;
			if( reverb_idx >= reverb_len ) {
				reverb_idx = 0;
			}
			buffer_idx += 2;
		}
	}
}

/* Reduce stereo-separation of count samples. */
static void crossfeed( short *audio, int count ) {
	int l, r, offset = 0, end = count << 1;
	while( offset < end ) {
		l = audio[ offset ];
		r = audio[ offset + 1 ];
		audio[ offset++ ] = ( l + l + l + r ) >> 2;
		audio[ offset++ ] = ( r + r + r + l ) >> 2;
	}
}

static long read_file( const char *filename, void *buffer, long length ) {
	FILE *file;
	long count;
	count = -1;
	file = fopen( filename, "rb" );
	if( file != NULL ) {
		count = fread( buffer, 1, length, file );
		if( count < length && !feof( file ) ) {
			printf("Unable to read file '%s'.\n", filename );
			count = -1;
		}
		if( fclose( file ) != 0 ) {
			printf("Unable to close file '%s'.\n", filename );
		}
	}
  else
    printf("Unable to find file '%s'.\n", filename );
	return count;
}

static void print_module_info() {
	int inst;
	char string[ 23 ];
	for( inst = 0; inst < 16; inst++ ) {
		micromod_get_string( inst, string );
		printf( "%02i - %-22s ", inst, string );
		micromod_get_string( inst + 16, string );
		printf( "%02i - %-22s\n", inst + 16, string );
	}
}

static long read_module_length( const char *filename ) {
	long length;
	signed char header[ 1084 ];
	length = read_file( filename, header, 1084 );
	if( length == 1084 ) {
		length = micromod_calculate_mod_file_len( header );
		if( length < 0 ) {
			printf("Module file type not recognised.\n");
		}
	} else {
		printf("Unable to read module file '%s'.\n", filename );
		length = -1;
	}
	return length;
}

void draw_wave()
{
	VPUClear(&vx, 0x00000000);

	for (uint32_t i=0; i<256; ++i)
	{
		int16_t L = 120 + (apubuffer[i*2+0]>>8);
		int16_t R = 120 + (apubuffer[i*2+1]>>8);
		L = L<0 ? 0 : (L>239 ? 239 : L);
		R = R<0 ? 0 : (R>239 ? 239 : R);
		sc.writepage[i+32 + L*320] = 0x37;
		sc.writepage[i+32 + R*320] = 0x27;
	}

	//VPUWaitVSync();
	CFLUSH_D_L1;
	VPUSwapPages(&vx, &sc);
}

void draw_wave_f()
{
	VPUClear(&vx, 0x00000000);

	for (uint32_t i=0; i<256; ++i)
	{
		int16_t L = 120 + (int16_t)(mix_buffer_f[i*2+0]*128.f);
		int16_t R = 120 + (int16_t)(mix_buffer_f[i*2+1]*128.f);
		L = L<0 ? 0 : (L>239 ? 239 : L);
		R = R<0 ? 0 : (R>239 ? 239 : R);
		sc.writepage[i+32 + L*320] = 0x37;
		sc.writepage[i+32 + R*320] = 0x27;
	}

	//VPUWaitVSync();
	CFLUSH_D_L1;
	VPUSwapPages(&vx, &sc);
}

static long play_module( signed char *module )
{
	long result;

	result = micromod_initialise( module, SAMPLING_FREQ * OVERSAMPLE );
	if( result == 0 )
	{
		print_module_info();
		samples_remaining = micromod_calculate_song_duration();
		printf( "Song Duration: %li seconds.\n", samples_remaining / ( SAMPLING_FREQ * OVERSAMPLE ) );
		fflush( NULL );

		// Set up buffer size for all future transfers
		APUSetBufferSize(BUFFER_SAMPLES);
		APUSetSampleRate(ASR_44_100_Hz);
		uint32_t prevframe = APUFrame();

		int playing = 1;
		while( playing )
		{
			int count = BUFFER_SAMPLES * OVERSAMPLE;
			if( count > samples_remaining )
				count = samples_remaining;

			__builtin_memset( mix_buffer, 0, BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE * sizeof( short ) );
			micromod_get_audio( mix_buffer, count );
			downsample( mix_buffer, apubuffer, BUFFER_SAMPLES * OVERSAMPLE );
			crossfeed( apubuffer, BUFFER_SAMPLES );
			reverb( apubuffer, BUFFER_SAMPLES );

			samples_remaining -= count;

			// NOTE: Buffer size is in multiples of 32bit words
			// h/w loops over (wordcount/4)-1 sample addresses and reads blocks of 16 bytes each time
			// Every time the APU flips to a new buffer, it'll change the value returned by APUFrame()

			// Make sure the writes are visible by the DMA
			CFLUSH_D_L1;

			// Fill current write buffer with new mix data
			APUStartDMA((uint32_t)apubuffer);

			// Draw the waveform in the mix buffer so we don't clash with apu buffer
			draw_wave();

			// Wait for the APU to finish playing back current read buffer
			// Meanwhile the playback buffer will still be going without interruptions
			uint32_t currframe;
			do
			{
				// Still working on same frame?
				currframe = APUFrame();
			} while (currframe == prevframe);

			// At this point the APU has switched
			// playback to the other buffer, we can
			// now fill the previous buffer.

			// Remember this frame
			prevframe = currframe;

			if( samples_remaining <= 0 || result != 0 )
				playing = 0;
		}
	}
	else
		printf("micromod_initialise failed\n");

	return result;
}

void PlayMODFile(const char *fname)
{
	signed char *module;
	long count, length;

	/* Read module file.*/
	length = read_module_length( fname );
	if( length > 0 )
	{
		printf( "Playing %s\n", fname);
		printf( "Module Data Length: %li bytes.\n", length );
		module = (signed char*)calloc( length, 1 );
		if( module != NULL )
		{
			count = read_file( fname, module, length );
			if( count < length )
				printf("Module file is truncated. %li bytes missing.\n", length - count );
			play_module( module );
			free( module );
		}
	}
}

void PlayXMFile(const char *fname)
{
	FILE *fp = fopen(fname, "rb");
	if (fp)
	{
		unsigned int filebytesize = 0;
		fpos_t pos, endpos;
		fgetpos(fp, &pos);
		fseek(fp, 0, SEEK_END);
		fgetpos(fp, &endpos);
		fsetpos(fp, &pos);
		filebytesize = (unsigned int)endpos;

		char *xmfile = (char*)calloc(filebytesize, 1);
		fread(xmfile, 1, filebytesize, fp);
		fclose(fp);

		// Set up buffer size for all future transfers
		APUSetBufferSize(BUFFER_SAMPLES_F/2); // word size, i.e. number of stereo sample pairs
		APUSetSampleRate(ASR_22_050_Hz);
		uint32_t prevframe = APUFrame();

		struct xm_context_s *ctx;
		int res = xm_create_context_safe(&ctx, xmfile, filebytesize, 22050);
		if (res == 0)
		{
			xm_set_max_loop_count(ctx, 1);
			uint16_t num_patterns = xm_get_number_of_patterns(ctx);
			uint16_t num_channels = xm_get_number_of_channels(ctx);
			uint16_t length = xm_get_module_length(ctx);

			const char* module_name = xm_get_module_name(ctx);
			const char* tracker_name = xm_get_tracker_name(ctx);
			printf("==> Playing: %s\n", module_name == NULL ? fname : module_name);
			if(tracker_name != NULL) printf("==> Tracker: %s\n", tracker_name);

			while(xm_get_loop_count(ctx) < 1)
			{
				xm_generate_samples(ctx, mix_buffer_f, BUFFER_SAMPLES_F);
				downsample_f(mix_buffer_f, apubuffer, BUFFER_SAMPLES_F);

				// Make sure the writes are visible by the DMA
				CFLUSH_D_L1;

				// Fill current write buffer with new mix data
				APUStartDMA((uint32_t)apubuffer);

				// Draw the waveform in the mix buffer so we don't clash with apu buffer
				//draw_wave_f();

				// Wait for the APU to finish playing back current read buffer
				// Meanwhile the playback buffer will still be going without interruptions
				uint32_t currframe;
				do
				{
					// Still working on same frame?
					currframe = APUFrame();
				} while (currframe == prevframe);

				// At this point the APU has switched
				// playback to the other buffer, we can
				// now fill the previous buffer.

				// Remember this frame
				prevframe = currframe;
			}
		}
		xm_free_context(ctx);
	}
}

int main(int argc, char *argv[])
{
	apubuffer = (short*)APUAllocateBuffer(BUFFER_SAMPLES*NUM_CHANNELS*sizeof(short));
	mix_buffer_f = (float*)APUAllocateBuffer(BUFFER_SAMPLES_F*NUM_CHANNELS*sizeof(float));
	printf("\nAPU mix buffer: 0x%.8x\n", (unsigned int)apubuffer);

	char currpath[48] = "sd:/";
	if (getcwd(currpath, 48))
		printf("Working directory:%s\n", currpath);

	char fullpath[128];
	strcpy(fullpath, currpath);
	strcat(fullpath, "/");

	if (argc<=1)
		strcat(fullpath, "test.mod");
	else
		strcat(fullpath, argv[1]);

	uint8_t *bufferB = VPUAllocateBuffer(320*240);
	uint8_t *bufferA = VPUAllocateBuffer(320*240);

    vx.m_vmode = EVM_320_Wide;
    vx.m_cmode = ECM_8bit_Indexed;
	VPUSetVMode(&vx, EVS_Enable);

	sc.cycle = 0;
	sc.framebufferA = bufferA;
	sc.framebufferB = bufferB;
	VPUSwapPages(&vx, &sc);
	VPUClear(&vx, 0x00000000);
	VPUSwapPages(&vx, &sc);
	VPUClear(&vx, 0x00000000);

	if (strstr(fullpath,".mod"))
		PlayMODFile(fullpath);
	else
		PlayXMFile(fullpath);

	printf("Playback complete\n");

	return 0;
}
