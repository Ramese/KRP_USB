#include "stm32f4xx_conf.h"
#include "lcd.h"
#include "timer.h"
#include "usb.h"
#include <stdio.h>
#include <stdarg.h>
#include "system_stm32f4xx.h"

#include "debug.h"
#include "init.h"

int main(){
  //SystemInit();
  int status = 0;
  debug(); // LCD, LED
  init();  // clock HSE, buttons, Joystick
  timer(); // timer and mouse function
  vypis(1, "init till usb ok");

  USB_Init(); // USB
  vypis(12, "USB init DONE!");
  
  //setEnabled(); // allow mouse function
  
  for (;;) {
    led(2, status);
    status = !status;
    for (int i=0; i<1000; i++)
      for (int j=0; j<15000; j++) ;
  }
}