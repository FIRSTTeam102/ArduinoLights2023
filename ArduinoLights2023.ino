#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define PIN_DATA 4 // Arduino pin that connects to strip data

#define NUM_PIXELS 60 // The number of LEDs (pixels) on strip
#define DELAY_INTERVAL 20

#define SATURATION 255
#define VALUE 130

#define YELLOW_HUE 10082
#define BLUE_HUE 43690
#define PINK_HUE 65536
#define RED_HUE 0
#define PURPLE_HUE 51651
#define ORANGE_HUE 4554
#define GREEN_HUE 21845
#define CORAL_HUE 62804
#define CYAN_HUE 32404

#define NUM_ACTIVE_GROUPS 6
// retroreflective, apriltag, coral, ele/arm, grabber, HP
int groupHues[] = {0, GREEN_HUE, ORANGE_HUE, CORAL_HUE, BLUE_HUE, CYAN_HUE, 0};

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_DATA, NEO_GRB + NEO_KHZ800);

int wrapPixel(int pixel) {
	return pixel < 0
		? NUM_PIXELS + pixel
		: pixel;
}

// not breathing comet
void comet(int hue, int saturation, int value) {
	int tailFormula;
	int tail = 13;
	for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // locate pixel
		for (int i = 0; i < tail; i++) {
			tailFormula = value / tail;
			if (i < 6) // first 6 should be max value
				strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, value))); // colors front of the comet
			else // next 7 should scale down to 0 value based on position in tail (further back = less intense)
				strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, value - ((i - 6) * (value / 7)))));
			strip.setPixelColor(wrapPixel(pixel - tail), strip.Color(0, 0, 0)); // clears end of tail
			strip.show();
		}
	}
}

// breathing comet (comet fades in and out)
#define BC_TAIL 11
void breathingComet(int hue, int saturation, int value) {
	int cyclePixel;	// pixel in the current section of the strip
	for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // locate pixel
		cyclePixel = pixel % 30;
		for (int i = 0; i < BC_TAIL; i++) {
			// true = breathe in, false = breathe out
			strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, cyclePixel > 15
				? (255 -(cyclePixel * 17))
				: (cyclePixel * 17)))); // colors front of the comet
			strip.setPixelColor(wrapPixel(pixel - BC_TAIL), strip.Color(0, 0, 0)); // clears end of tail
			strip.show();
		}
	}
}

// fade strip in and out
void breathe(int hue,int saturation, int value) {
	for (int i = 0; i < 255; i++) {
		strip.fill(strip.gamma32(strip.ColorHSV(hue, saturation, i)));
		strip.show();
	}
	for (int i = 255; i > 0; i--) {
		strip.fill(strip.gamma32(strip.ColorHSV(hue, saturation, i)));
		strip.show();
	}
}

void fillStrip_RGB(int r, int g, int b){
	for (int i = 0; i <= NUM_PIXELS; i++) {
		strip.setPixelColor(i, r, g, b);
		strip.show();
	}
}

// basic blink
void blink(int hue, int saturation, int value) {
	static bool toggle = false; // false=odd, true=even
	uint32_t color = strip.ColorHSV(hue, saturation, value);
	uint32_t off = strip.ColorHSV(0, 0, 0);
	uint32_t evenColor;
	uint32_t oddColor;

	if (toggle == false) {
		oddColor = color;
		evenColor = off;
	} else {
		evenColor = color;
		oddColor = off;
	}
	toggle = !toggle;

	for (int i = 0; i < NUM_PIXELS; i++) {
		if (i % 2 == 0) { // even
			strip.setPixelColor(i, evenColor);
		} else {
			strip.setPixelColor(i, oddColor);
		}
	}
	strip.show();
	// delay(1000); // fixme: use counter
}

void rave(int saturation, int value) {
	uint32_t hue = random(65536);
	uint32_t color = strip.ColorHSV(hue, saturation, value);
	for (int j = 0; j < NUM_PIXELS * 20; j++) {
		for (int i = 0; i < NUM_PIXELS; i++) {
			strip.setPixelColor(i, color);
		}
	}
	strip.show();
	// delay(250); // fixme: use counter
}

void setDirectControlPixel(byte group, int pixel, bool on) {
	uint32_t color = strip.gamma32(strip.ColorHSV(groupHues[group], 255, on ? 255 : 0));
	strip.setPixelColor(pixel, color);
	strip.setPixelColor(pixel + 1, color);
}

void setGroup(int startPixel, int hue) {
	for (int i = 0; i < 6; i++) {
		strip.setPixelColor(startPixel + i, strip.ColorHSV(hue, 255, 255));
	}
}

void setup() {
	pinMode(SS, INPUT); // chip select
	pinMode(SCK, INPUT); // clock
	pinMode(MOSI, INPUT); // master out *slave in*
	pinMode(MISO, OUTPUT); // master in *slave out*

	// set SPI control register
	SPI.attachInterrupt();
	SPCR |= _BV(SPE); // enable SPI
	SPI.setBitOrder(MSBFIRST);
	SPCR &= ~_BV(MSTR); // slave
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);

	Serial.begin(9600);

	strip.begin();
	strip.clear();
}

volatile byte statuses[16] = {0};
// SPI interrupt routine
ISR(SPI_STC_vect) {
	// SPDR is the incoming byte
	statuses[SPDR & 0b1111] = SPDR >> 4;
}

void loop() {
	switch (statuses[0]) { // control mode
		// regular
		case 2:
			for (byte group = 1; group <= NUM_ACTIVE_GROUPS; group++) {
				Serial.print("G");
				Serial.print(group);
				Serial.print(" ");
				Serial.println((group - 1) * 6);
				byte state = statuses[group];
				for (int pix = (group - 1) * 6; pix < NUM_PIXELS; pix += (6 * NUM_ACTIVE_GROUPS)) {
					if (state >> 3 == 0) {
						Serial.println(pix);
						// direct control
						setDirectControlPixel(group, pix, (state & 0b100) != 0);
						setDirectControlPixel(group, pix + 2, (state & 0b010) != 0);
						setDirectControlPixel(group, pix + 4, (state & 0b001) != 0);
					} else if (state == 0b1000) {
						// cone
						setGroup(pix, YELLOW_HUE);
					} else if (state == 0b1001) {
						// cube
						setGroup(pix, PURPLE_HUE);
					} else {
						Serial.println("sus");
					}
				}
			}
			strip.show();
			break;

		// disabled
		case 3:
			breathingComet(RED_HUE, SATURATION, VALUE);
			break;
		case 4:
			breathingComet(BLUE_HUE, SATURATION, VALUE);
			break;

		// off
		case 1:
		default:
			strip.clear();
			strip.show();
			break;
	}
}
