#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define PIN_RX 10
#define PIN_TX 11
#define PIN_DATA 4 // Arduino pin that connects to strip data

#define NUM_PIXELS 120 // The number of LEDs (pixels) on strip
#define DELAY_INTERVAL 20

#define SATURATION 255
#define VALUE 255

#define YELLOW_HUE 10082
#define BLUE_HUE 43690
#define PINK_HUE 65536
#define RED_HUE 0
#define PURPLE_HUE 51651
#define ORANGE_HUE 4554
#define GREEN_HUE 21845

int counter = 0;
int randColor = random(65536);

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_DATA, NEO_GRB + NEO_KHZ800);

int wrapPixel(int pixel) {
  return pixel < 0
         ? NUM_PIXELS + pixel
         : pixel;
}

void comet(int hue, int saturation, int value) {
  int tailFormula;
  int tail = 9;
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // locate pixel
    for (int i = 0; i < tail; i++) {
      strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, value))); // colors front of the comet
      strip.setPixelColor(wrapPixel(pixel - tail), strip.Color(0, 0, 0)); // clears end of tail
      strip.show();	
    }
  }
}
void fillStrip_RGB(int RED, int GREEN, int BLUE){
	for(int i=0; i<=NUM_PIXELS;i++){
		strip.setPixelColor(i, RED, GREEN, BLUE);			
		strip.show();
	}
}

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

void RAVE(int saturation, int value) {
  uint32_t hue = random(65536);
  uint32_t color = strip.ColorHSV(hue, saturation, value);
  for(int j = 0; j<NUM_PIXELS*2; j++){
  	for (int i = 0; i < NUM_PIXELS; i++) {
    	strip.setPixelColor(i, color);
  	}
  }
  strip.show();
  // delay(250); // fixme: use counter
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
      // if (counter%2000) return;
      // delay(150); // fixme: use counter
      for (int i = 0; i < NUM_PIXELS; i += 3) {
        strip.setPixelColor(i + q, 0, 0, 0); // turn every third pixel off
      }
    }
  }
}

#define NUM_ACTIVE_GROUPS 5
int groupHues[] = {0, BLUE_HUE, PINK_HUE, ORANGE_HUE, GREEN_HUE, RED_HUE};
void setDirectControlPixel(byte group, int pixel, bool on) {
  strip.setPixelColor(pixel, strip.ColorHSV(groupHues[group], 255, on ? 255 : 1));
}

void setGroup(int startPixel, int hue) {
  strip.setPixelColor(startPixel, strip.ColorHSV(hue, 255, 255));
  strip.setPixelColor(startPixel + 1, strip.ColorHSV(hue, 255, 255));
  strip.setPixelColor(startPixel + 2, strip.ColorHSV(hue, 255, 255));
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
	statuses[0] = 7;
	
  switch (statuses[0]) { // control mode
    // regular
    case 2:
      for (byte group = 1; group <= NUM_ACTIVE_GROUPS; group++) {
        byte state = statuses[group];
        for (int pix = (group - 1) * 3; pix < NUM_PIXELS; pix += (3 * NUM_ACTIVE_GROUPS)) {
          if (state & 0b1000 != 0) {
            // direct control
            setDirectControlPixel(group, pix, (state & 0b100) != 0);
            setDirectControlPixel(group, pix + 1, (state & 0b010) != 0);
            setDirectControlPixel(group, pix + 2, (state & 0b001) != 0);
          } else if (state == 0b1000) {
            // cone
            setGroup(pix, YELLOW_HUE);
          } else if (state == 0b1001) {
            // cube
            setGroup(pix, PURPLE_HUE);
          }
        }
      }
      strip.show();
      break;

    // disabled
    case 3:
      comet(RED_HUE, SATURATION, VALUE);
      break;
    case 4:
      comet(BLUE_HUE, SATURATION, VALUE);
      break;
		case 5:  //cube
			fillStrip_RGB(159,23,169);
			break;
    case 6:  //cone 
    	fillStrip_RGB(236,125,12);
    	break;
    // off
    case 7:
    	RAVE(255,255);
    	break;
    case 1: //cone
    default:
      strip.clear();
      strip.show();
      break;
  }
}
