
#ifdef USE_NEOPIXEL_V1
#include <Adafruit_NeoPixel.h>


// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        SDA // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels
#endif

#include "menu_cli.h"

#include "clockmatrix.h"

void setup() {
#ifdef USE_NEOPIXEL_V1
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
#endif


  Serial.begin();
  while ( !Serial ) {
    delay(1);
  }
  init_menu_cli();

  init_clockmatrix();


  // a bit of a hack, call this after everything else is initialized
  // makes sure the root directory on start up has everything properly
  addMenuCLI_current_directory();
}

void loop() {
#ifdef USE_NEOPIXEL_V1
  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(0, 150, 0));
    pixels.setBrightness(255);
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(DELAYVAL); // Pause before next pass through loop
  }
#endif


  //Serial.println("Hi");
  delay(1000);
}
