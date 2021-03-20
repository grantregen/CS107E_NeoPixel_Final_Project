# CS107E NeoPixel Final Project

## Project Rundown 
Here I create a basic driver for arduino neopixels to be run by output pin of raspberry pi. I origianlly tried for a long time to use the Pi's Timer Control Register to increase the counting frequency to a point where one could avoid using asembly to send data bits to the neopixels. I increased to clock frequency to 10Mhz (and eventually 250 Mhz) with the following code: 
```
// Code to run counter at 10 per microsecond!!!
volatile unsigned int *pinAddress;
pinAddress = (unsigned int *)0x2000B408; // Free Running Counter
volatile unsigned int *pinAddress_counter;
pinAddress_counter = (unsigned int *)0x2000B420; // Free Running Counter
// before changing clock frequency
printf("Time-Control Register = %d\n", *pinAddress);
unsigned int start = *pinAddress_counter;
//printf("%d\n", *pinAddress_counter);
timer_delay(10);
unsigned int stop = *pinAddress_counter;
unsigned int difference = stop - start;
printf("%d\n", difference);
*pinAddress = 0b000110000000001000000000;
printf("Time-Control Register = %d\n", *pinAddress);
start = *pinAddress_counter;
timer_delay(10);
stop = *pinAddress_counter;
difference = stop - start;
printf("%d\n", difference);
```
However, after much troubleshooting, trying inline code vs. function calls, and changing the system caching system I decided to get a hold of an occiliscope to verify the signals. At a clock frequency of 250Mhz, the delays only could reach a frequency of 1MHz due to limitations in the speed of a non-assembly for loop and gpio_set high function call. Thus, I changed to Julie's awesome assembly neopixel timing code (which was a lifesaver) and did some tests with the occiliscope. 

## Setting up your NeoPixels to use the driver (and things I learned)
