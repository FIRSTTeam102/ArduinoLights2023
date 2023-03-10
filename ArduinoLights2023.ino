#include <Adafruit_NeoPixel.h>
#include <SPI.h>

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

#define NUM_ACTIVE_GROUPS 5
int groupHues[] = {0, BLUE_HUE, PINK_HUE, ORANGE_HUE, GREEN_HUE, RED_HUE};

Adafruit_NeoPixel strip(NUM_PIXELS, PIN_DATA, NEO_GRB + NEO_KHZ800);


//co-function to assist comet
int wrapPixel(int pixel) {
  return pixel < 0
         ? NUM_PIXELS + pixel
         : pixel;
}

//default comet
void comet(int hue, int saturation, int value) {
	int tailFormula;
	int tail = 9;
	for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // locate pixel
		for (int i = 0; i < tail; i++) {
			tailFormula = value / tail;
			strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, value))); // colors front of the comet
			strip.setPixelColor(wrapPixel(pixel - tail), strip.Color(0, 0, 0)); // clears end of tail
			strip.show();
		}
	}
}

// breathing comet
void breatingComet(int hue, int saturation, int value) {
	bool increase;
  int tailFormula;
  int Value = 0;
  int tail = 9;
  int cyclePixel;
  bool breatheStatus;  //true = breathe in, false = breathe out
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // locate pixel
  	cyclePixel = pixel % 30;
    for (int i = 0; i < tail; i++) {
    	if(cyclePixel>15){
    		breatheStatus = false;
    	}
    	else{
    		breatheStatus = true;
    	}
    	if(breatheStatus == false){
    		strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, 255-(cyclePixel * 17)))); // colors front of the comet
    	}
    	else{
    		strip.setPixelColor(pixel - i, strip.gamma32(strip.ColorHSV(hue, saturation, cyclePixel * 17))); // colors front of the comet
    	}
      strip.setPixelColor(wrapPixel(pixel - tail), strip.Color(0, 0, 0)); // clears end of tail
      strip.show();	
    }
  }
}

//fade strip in and out
void breathe(int hue,int saturation, int value){
	int Saturation = 0;
	bool increase;
	int i;
	for(i = 0; i < 255; i++){
		strip.fill(strip.gamma32(strip.ColorHSV(hue,saturation,i)));
		strip.show();
	}
	for(i = 255; i>0; i--){
		strip.fill(strip.gamma32(strip.ColorHSV(hue,saturation,i)));
		strip.show();
	}
}




//Call for cone or cube
void fillStrip_RGB(int RED, int GREEN, int BLUE){
	for(int i=0; i<=NUM_PIXELS;i++){
		strip.setPixelColor(i, RED, GREEN, BLUE);			
		strip.show();
	}
}


//basic blink
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
  for(int j = 0; j<NUM_PIXELS*4; j++){
  	for (int i = 0; i < NUM_PIXELS; i++) {
    	strip.setPixelColor(i, color);
  	}
  }
  strip.show();
  // delay(250); // fixme: use counter
}

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
volatile byte statuses[16] = {3, 1, 1, 1, 1, 1};
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
    	rave(255,255);
    	break;
    case 8:
    	breathe(RED_HUE, SATURATION ,VALUE);
    	break;
    case 1: //cone
    default:
      strip.clear();
      strip.show();
      break;
  }
}
