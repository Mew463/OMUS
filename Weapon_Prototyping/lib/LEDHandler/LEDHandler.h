#include <Freenove_WS2812_Lib_for_ESP32.h>
#include <FastLED.h>

#define LEDPIN 45

enum Colors {
	RED,
	ORANGE,
	YELLOW,
	LIME,
	GREEN,
	AQUA,
	CYAN,
	SKY,
	BLUE,
	PURPLE,
	RED_PURPLE,
	BLACK,
	WHITE
};

void init_led();

void setLeds(Colors color);

void toggleLeds(Colors color1, Colors color2, int delayMS);