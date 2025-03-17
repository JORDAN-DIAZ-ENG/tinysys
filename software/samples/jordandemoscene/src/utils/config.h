#ifndef CONFIG_H
	#define CONFIG_H

#ifndef USE_16BIT_COLOR
	#define USE_16BIT_COLOR 0  // Or set to 1 depending on your needs
#endif

constexpr uint32_t backbuffer_size = 640 * 480 * 2;  // 2 bytes per pixel (16-bit)


#endif // CONFIG_H
