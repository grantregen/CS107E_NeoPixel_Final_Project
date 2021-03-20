/*
d8888b.  .d8b.  .d8888. d8888b. d8888b. d88888b d8888b. d8888b. db    db      d8888b. d888888b
88  `8D d8' `8b 88'  YP 88  `8D 88  `8D 88'     88  `8D 88  `8D `8b  d8'      88  `8D   `88'
88oobY' 88ooo88 `8bo.   88oodD' 88oooY' 88ooooo 88oobY' 88oobY'  `8bd8'       88oodD'    88
88`8b   88   88   `Y8b. 88      88   b. 88      88`8b   88`8b      88         88         88
88 `88. 88   88 db   8D 88      88   8D 88.     88 `88. 88 `88.    88         88        .88.
88   YD YP   YP `8888Y' 88      Y8888P' Y88888P 88   YD 88   YD    YP         88      Y888888P


d8b   db d88888b  .d88b.  d8888b. d888888b db    db d88888b db           db      d888888b d8888b. d8888b.  .d8b.  d8888b. db    db
888o  88 88'     .8P  Y8. 88  `8D   `88'   `8b  d8' 88'     88           88        `88'   88  `8D 88  `8D d8' `8b 88  `8D `8b  d8'
88V8o 88 88ooooo 88    88 88oodD'    88     `8bd8'  88ooooo 88           88         88    88oooY' 88oobY' 88ooo88 88oobY'  `8bd8'
88 V8o88 88      88    88 88         88     .dPYb.  88      88           88         88    88   b. 88`8b   88   88 88`8b      88
88  V888 88.     `8b  d8' 88        .88.   .8P  Y8. 88.     88booo.      88booo.   .88.   88   8D 88 `88. 88   88 88 `88.    88
VP   V8P Y88888P  `Y88P'  88      Y888888P YP    YP Y88888P Y88888P      Y88888P Y888888P Y8888P' 88   YD YP   YP 88   YD    YP

Library to drive neopixel strips from single data pin with a few preset displays
and colors. Tested for strips up to 300 pixels, but should work for even more.
Can work for both older neopixels and newwer versions (simply change RESET_TIME)
to match that needed for neopixel version (>300 for new, >50 for older models).

To set the strip, the user needs to create a neopixel strip "buffer", almost like
a frame buffer, that is then sent to a function to quickly transfer all the bits
at once.

Enjoy :-)
*/

// User defined inputs for neopixel setup
#define NUMBER_NEOPIXELS 300    // Number neopixels in strip
#define OUTPUT_PIN 18           // Output pin number
#define RESET_TIME 300          // Reset time between data sends, look at data
                                //  sheet for your neopixel for Reset bit

#include "gpio.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "system.h"
#include "gl.h"
#include "malloc.h"

// define color struct made up of red, green, and blue values
// Note: red, green, and blue values should not exceed 100 each,
//       and color doesn't include brightness/luminace data. This
//       is sent seperately when setting a pixel.
typedef struct
{
   unsigned int red, green, blue;
} color;

// define helpful colors as RGB values
const color WHITE = {20, 20, 20};
const color BLACK = {0, 0, 0};
const color RED = {20, 0, 0};
const color GREEN = {0, 20, 0};
const color BLUE = {0, 0, 20};
const color YELLOW = {20, 10, 0};
const color ORANGE = {50, 1, 0};
const color BRIGHT_PINK = {30, 0, 5};
const color PALE_PINK = {70, 1, 1};
const color VIOLET = {20, 0, 25};
const color CS107E_ELECTRIC_BLUE = {0, 30, 25};


/* Function to send bit as specifically timed high/low combination to represent
   0/1 bits for neopixel data line. Written by Julie Zelenski in assembly for
   precise timing, see neo_timing.s for details.

   Args:
      pin: pin number to send bit to, should be initialized before as output pin
      bit: either 1 or 0 and cooresponds to neopixel bit to send
*/
void send_bit(unsigned int pin, unsigned int bit);

/* Helper function to find random number from 0 to max value

   Args:
      max: upper limit to possible number returned from random selction
   Returns:
      unsigned int: random number in range [0, max]
*/
unsigned int get_random_value(unsigned int max){
  unsigned int random_pixel = rand();
  return(random_pixel%max);
}

/* Helper function to reset neopixel strip buffer as all zeros

   Args:
      neopixel_set: pointer to buffer array
*/
void reset_neopixel_strip(unsigned int *neopixel_set){
  for(int i = 0; i < (24 * NUMBER_NEOPIXELS); i++){
    neopixel_set[i] = 0;
  }
}


/* Changes neopixel buffer at desired index on strip to user defined color and
   brightness/luminance.

   Args:
      color: color to set desired neopixel on strip
      luminance: float from 0 to 10 that defines pixel brightness scaling
                 note: can use a little work to match brightness reading of
                       human eye, equations available online
      pixel_index: index of neopixel on strip to set desired color and luminance
                   where 0 is the pixel closest to the data line and
                   NUMBER_NEOPIXELS - 1 would be the last defined pixel
      neopixel_set: buffer to be changed by function
*/
void set_neopixel_pixel(color color, float luminance,
                        unsigned int pixel_index, unsigned int *neopixel_set)
{
    // check color values are valid (<240)
    if ((color.red > 100) || (color.green > 100) || (color.blue > 100)){
      printf("ERROR: Color values should not exceed 100, setting neopixel to off.\n");
      color.red = 0;
      color.green = 0;
      color.blue = 0;
    }

    // check neopixel index is valid (pixel_index < NUMBER_NEOPIXELS)
    if (pixel_index >= NUMBER_NEOPIXELS){
      printf("ERROR: neopixel index larger than number of set neopixels, leaving function!\n");
      return;
    }

    // check luminance is valid (only 0-10), if not set luminace to zero (dead pixel)
    // not shouldn't be able to go below zero bc on unsigned type
    if ((luminance > 10.0) || (luminance < 0.0)){
      printf("ERROR: luminace must be between [0, 10], setting luminance to zero.\n");
      luminance = 0.0;
    }

    unsigned int red = 0;
    unsigned int green = 0;
    unsigned int blue = 0;
    // if luminance is zero, keep colors zeroed for black pixel
    // if luminance less than one but greater than zero, use dimming law
    if ((luminance < 1.0) && (luminance > 0.0)){
      // scale colors to set luminance (brightness) if not zeroed
      if (color.red != 0) { red = (unsigned int)(luminance*color.red + 14*luminance); }
      if (color.green != 0) { green = (unsigned int)(luminance*color.green + 14*luminance); }
      if (color.blue != 0) { blue = (unsigned int)(luminance*color.blue + 14*luminance); }
    }
    // if luminance is greater than one use linear law
    else if (luminance != 0.0){
      // scale colors to set luminance (brightness) if not zeroed
      if (color.red != 0) { red = (unsigned int)(color.red + 14*luminance); }
      if (color.green != 0) { green = (unsigned int)(color.green + 14*luminance); }
      if (color.blue != 0) { blue = (unsigned int)(color.blue + 14*luminance); }
    }

    // find pixel location in neopixel_set array
    unsigned int startIndex = pixel_index * 24;

    // turn color values into 8-bit binary numbers and store them into larger
    // array containing all pixel values. startIndex is starting index of 24
    // indecies corresponding with a Green, Red, and Blue byte in order of
    // most significant to least significant bits.

    // initialize pixel section by clearing current values
    for(int i = 0; i < 24; i++){
      neopixel_set[startIndex + i] = 0;
    }

    // set binary values in buffer
    // handle green bits... pixelBits[startIndex:startIndex+7]
    unsigned int i = 7;
    while (green > 0) { neopixel_set[startIndex + i] = green % 2;  green /= 2;  i--; }
    // handle red bits... pixelBits[startIndex+8:startIndex+15]
    i = 15;
    while (red > 0) { neopixel_set[startIndex + i] = red % 2; red /= 2; i--; }
    // handle blue bits... pixelBits[startIndex+16:startIndex+23]
    i = 23;
    while (blue > 0) { neopixel_set[startIndex + i] = blue % 2; blue /= 2; i--; }

}

/* Runs neopixel buffer by reseting data line and then sending bits to OUTPUT_PIN.
   Note: run inline to decrease run time slightly

   Args:
      neopixel_set: neopixel strip buffer to be run (contains all 24 bits for
                    each pixel in order). Note: buffer is cleared after run.
*/
inline void run_pixels(unsigned int *neopixel_set){
  timer_delay_us(RESET_TIME);        //bits always end with low, so waiting after or before bit will always wait low
  unsigned int bit_size = NUMBER_NEOPIXELS * 24;
  for(unsigned int i = 0; i < bit_size; i++){
    send_bit(OUTPUT_PIN, neopixel_set[i]);
  }
  reset_neopixel_strip(neopixel_set);
}

/* Future addition to code, adds malloc to allow sequences of buffers in heap memory
   that can be run at a slightly faster rate.

   Args:
      sequence: array of neopixel strip buffers in order of run
      sequence_length: length of array of buffers
*/
// inline void run_neopixel_sequence(unsigned int* *sequence, unsigned int sequence_length){
//     unsigned int bit_size = NUMBER_NEOPIXELS * 24;
//     for (int i = 0; i < sequence_length; i++){
//       for(unsigned int j = 0; j < bit_size; j++){
//         send_bit(OUTPUT_PIN, sequence[i][j]);
//       }
//     }
// }

/* run solid color on all pixels

   Args:
      color: color to set full strip to
      luminace: brightness to set whole strip to
      neopixel_set: empty buffer to store pixel bit data in
*/
void run_solid_color(color color,
                     float luminance,
                     unsigned int *neopixel_set){

   for(int i = 0; i < NUMBER_NEOPIXELS; i++){
     set_neopixel_pixel(color, luminance, i, neopixel_set);
   }

   run_pixels(neopixel_set);
}

/* run strobe on all pixels

   Args:
      color_1: color for one step of strobe
      color_2: color for other step of strobe
      luminace: brightness to set whole strip during strobe
      wait_time: time between strobe steps in milliseconds
      neopixel_set: empty buffer to store pixel bit data in
*/
void run_strobe(color color_1, color color_2, float luminance,
                unsigned int wait_time, unsigned int *neopixel_set){
      // set first strobe color
      run_solid_color(color_1, luminance, neopixel_set);
      timer_delay_ms(wait_time);
      // set second strobe color
      run_solid_color(color_2, luminance, neopixel_set);
      timer_delay_ms(wait_time);
}

/* run fade in/out on all pixels

   Args:
      color: color to set full strip to
      time: timeing scale in ms to set fade in and out
            (multiplied by 200 for time of full fade in and out)
      neopixel_set: empty buffer to store pixel bit data in
*/
void run_fade(color color, unsigned int time, unsigned int *neopixel_set){
    //fade into top brightness
    for(int i = 10; i<100; i++){
      run_solid_color(color, 0.02*i, neopixel_set);
      timer_delay_ms(time);
    }
    //fade out to zero brightness
    for(int i = 99; i>10; i--){
      run_solid_color(color, 0.02*i, neopixel_set);
      timer_delay_ms(time);
    }
}

/* runs random pixels at random colors and random brightness (like christmas lights)

   Args:
      colors: array of colors to set random pixels to,
              Note: to get more pixels a certain color, simply add it more times
                    to this array
      number_colors: length of colors array
      percent_on: percent of pixels desired to be on
                  Note: a smaller amount of pixels will actually be on bc some
                        will have a luminance of 0 randomly
      luminance_high: max luminace a pixel could be at
      neopixel_set: empty buffer to store pixel bit data in
*/
void run_random(color *colors, unsigned int number_colors,
                unsigned int percent_on, float luminance_high,
                unsigned int *neopixel_set){

  unsigned int num_pixels_on = (unsigned int)(NUMBER_NEOPIXELS*(percent_on/100.));
  unsigned int random_color_index = 0;
  unsigned int random_pixel_index = 0;
  unsigned int random_luminance_scaling = 1;
  float luminance = 1.0;
  // set percent of buffer to random colors
  for(int i = 0; i < num_pixels_on; i++){
    random_pixel_index = get_random_value(NUMBER_NEOPIXELS - 1);
    random_color_index = get_random_value(number_colors - 1);
    random_luminance_scaling = get_random_value(100) + 1;
    luminance = (float) luminance_high/random_luminance_scaling;
    set_neopixel_pixel(colors[random_color_index], luminance, random_pixel_index, neopixel_set);
  }
  // run buffer
  run_pixels(neopixel_set);
}

/* Runs lightening strike with bright flash and then random line of lightening

   Args:
      neopixel_set: empty buffer to store pixel bit data in
*/
void run_lightening(unsigned int *neopixel_set){
  // initial flash
  run_strobe(WHITE, BLACK, 3., 3, neopixel_set);

  // run line of lightening
  unsigned int lightening_start_index = get_random_value(NUMBER_NEOPIXELS - 1);
  unsigned int lightening_length = get_random_value(NUMBER_NEOPIXELS/2);

  // half the time have lightening stike go forward
  if (lightening_length % 2 == 0){
    // dont allow index to go out of bounds
    for(int i = 0; ((i < lightening_length) && (lightening_start_index+i < NUMBER_NEOPIXELS)); i++){
        // set two pixels at a time to make line look faster
        set_neopixel_pixel(WHITE, 1, lightening_start_index+i, neopixel_set);
        if(lightening_start_index+i+1 < NUMBER_NEOPIXELS){
          set_neopixel_pixel(WHITE, 1, lightening_start_index+i+1, neopixel_set);
        }
        run_pixels(neopixel_set);
    }
  }
  // half the time have lightening stike go backward
  else{
    // dont allow index to go out of bounds
    for(int i = 0; ((i < lightening_length) && (lightening_start_index-i > 0)); i++){
      if(lightening_start_index-i != 0){
        set_neopixel_pixel(WHITE, 1, lightening_start_index-i-1, neopixel_set);
      }
      set_neopixel_pixel(WHITE, 1, lightening_start_index-i, neopixel_set);
      run_pixels(neopixel_set);
    }
 }
 // reset pixels to zero
 run_pixels(neopixel_set);

}

void main (void)
{
    // must set up uart peripheral before using, init once
    uart_init();
    // configure output pin
    gpio_set_output(OUTPUT_PIN);
    // create array that holds "buffer" of LED values
    unsigned int neopixel_set[NUMBER_NEOPIXELS * 24] = {0};

    // EXAMPLES... simply uncomment to run different display...

    run_solid_color(CS107E_ELECTRIC_BLUE, 2, neopixel_set);

    // while(1){
    // run_lightening(neopixel_set);
    // timer_delay_ms(800);
    // }

    // color rainbow[7] = {RED, ORANGE, YELLOW, GREEN, BLUE, BRIGHT_PINK, VIOLET};
    // while(1){
    //   run_random(rainbow, 7, 100, 6, neopixel_set);
    //   timer_delay_ms(10);
    // }

    // while(1){
    // run_fade(ORANGE, 1, neopixel_set);
    // }

    // while(1){
    //   run_strobe(WHITE, BLACK, 1.0, 50, neopixel_set);
    // }

    uart_putchar(EOT); // not strictly necessary, but signals to rpi-install that program is done
}
