#define ST7789_DRIVER

#define TFT_WIDTH 240
#define TFT_HEIGHT 320

#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_MISO -1

#define TFT_CS -1
#define TFT_DC 10
#define TFT_RST 8

#define BL_CHANNEL 0
#define DAY_BRIGHTNESS 255
#define NIGHT_BRIGHTNESS 40
#define TFT_BL 0

#define SPI_FREQUENCY 10000000

#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8