#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RX 10
#define PIN_TX 11
#define PIN_NEO_PIXEL 4 // Arduino pin that connects to strip

#define NUM_PIXELS 60 // The number of LEDs (pixels) on strip
#define DELAY_INTERVAL 20

#define SATURATION 255
#define VALUE 160

#define YELLOW_HUE 10082
#define BLUE_HUE 43690
#define PINK_HUE 65536
#define RED_HUE 0
#define PURPLE_HUE 52651
#define ORANGE_HUE 4554

int counter = 0;
int randColor = random(65536);

SoftwareSerial rioSerial{PIN_RX, PIN_TX};

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

int wrapPixel(int pixel) {
	return pixel < 0
		? NUM_PIXELS + pixel
		: pixel;
}

void cometHSV(int hue, int saturation, int value) {
	int tailFormula;
	int tail = 9;
	for (int pixel = 0; pixel<NUM_PIXELS; pixel++) { // locate pixel
		for (int i = 0; i < tail; i++) {
			tailFormula = value/tail;
			strip.setPixelColor(pixel-i, strip.gamma32(strip.ColorHSV(hue, saturation, value))); // colors front of the comet
			strip.setPixelColor(wrapPixel(pixel - tail), strip.Color(0,0,0)); // clears end of tail
			strip.show();
		}
	}
}

void blink(int hue, int saturation, int value) {
	static bool toggle = false; // false will be odd on, 1 will be even on
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
		if (i%2 == 0) { // even
			strip.setPixelColor(i, evenColor);
		} else {
			strip.setPixelColor(i, oddColor);
		}
	}
	strip.show();
	delay(1000);
}

void flashBang(){
	for (int i = 0; i < NUM_PIXELS; i++) {
		strip.setPixelColor(i, 255, 255, 255);
		strip.show();
	}
}

void showoffRGB() {
	int red = random(255);
	int green = random(255);
	int blue = random(255);
	for(int i = 0; i < NUM_PIXELS; i++){
		strip.setPixelColor(i, red, green, blue);
		strip.show();
	}
}

void showoffHSV(int saturation, int value) {
		uint32_t hue = random(65536);
		uint32_t color = strip.ColorHSV(hue, saturation, value);
		for (int i = 0; i < NUM_PIXELS; i++) {
			strip.setPixelColor(i, color);
		}
		strip.show();
		//delay(500);
}

// adapted from Hans Luitjen's theatre chase effect (https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/)
void theaterChase(int hue, int saturation, int value) {
	uint32_t color = strip.ColorHSV(hue, saturation, value);
	for (int j = 0; j < 10; j++) { // do 10 cycles of chasing
		for (int q = 0; q < 3; q++) {
			for (int i = 0; i < NUM_PIXELS; i += 3) {
				strip.setPixelColor(i + q, color); // turn every third pixel on
			}
			strip.show();
			//if (counter%2000) return;
			delay(150);
			for (int i = 0; i < NUM_PIXELS; i += 3) {
				strip.setPixelColor(i + q, 0, 0, 0); // turn every third pixel off
			}
		}
	}
}

#define NUM_ACTIVE_GROUPS 5
int groupHues[] = {0, YELLOW_HUE, BLUE_HUE, PINK_HUE, PURPLE_HUE, ORANGE_HUE};
void setDirectControlPixel(byte group, int pixel, bool on) {
	strip.setPixelColor(pixel, strip.ColorHSV(groupHues[group], 255, on ? 255 : 1));
}

void setup() {
	pinMode(PIN_RX, INPUT);
	pinMode(PIN_TX, OUTPUT);
	rioSerial.begin(9600);
	Serial.begin(9600);
	strip.begin(); // INITIALIZE strip strip object (REQUIRED)
	strip.clear();
}

byte message = 0;
byte statuses[16] = {0};
void loop() {
	// read in all new messages
	while (rioSerial.available() > 0) {
		message = rioSerial.read();
		statuses[message & 0b1111] = message >> 4;

		Serial.print("group: ");
		Serial.print(message & 0b1111, DEC);
		Serial.print(" status: ");
		Serial.println(message >> 4, DEC);
	}

	counter++;

	// control mode
	switch (statuses[0]) {
		// regular
		case 2:
			for (byte group = 1; group <= NUM_ACTIVE_GROUPS; group++) {
				byte state = statuses[group];
				if (state & 0b1000 != 0) {
					// direct control
					for (int pix = (group - 1) * 3; pix < NUM_PIXELS; pix += (3 * NUM_ACTIVE_GROUPS)) {
						setDirectControlPixel(group, pix, (state & 0b100) != 0);
						setDirectControlPixel(group, pix + 1, (state & 0b010) != 0);
						setDirectControlPixel(group, pix + 2, (state & 0b001) != 0);
					}
				} else {
					// future use
				}
			}
			strip.show();
			break;
		
		// disabled
		case 3:
			cometHSV(RED_HUE, SATURATION, VALUE);
			break;
		case 4:
			cometHSV(BLUE_HUE, SATURATION, VALUE);
			break;

 		// off
		case 1:
		default:
			strip.clear();
			strip.show();
			break;
	}
}
