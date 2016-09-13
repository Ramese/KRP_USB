#include "stm32f4xx_conf.h"
#include "lcd.h"

void led(int i, int status){
  if(i == 1)
    GPIO_WriteBit(GPIOG, 1 << 6, (BitAction) status);
  else if(i == 2)
    GPIO_WriteBit(GPIOG, 1 << 8, (BitAction) status);
  else if(i == 3)
    GPIO_WriteBit(GPIOI, 1 << 9, (BitAction) status);
  else
    GPIO_WriteBit(GPIOC, 1 << 7, (BitAction) status);
}

void led_off(int i){
  if(i == 1)
    GPIO_WriteBit(GPIOG, 1 << 6, (BitAction) 0);
  else if(i == 2)
    GPIO_WriteBit(GPIOG, 1 << 8, (BitAction) 0);
  else if(i == 3)
    GPIO_WriteBit(GPIOI, 1 << 9, (BitAction) 0);
  else
    GPIO_WriteBit(GPIOC, 1 << 7, (BitAction) 0);
}

void led_on(int i){
   if(i == 1)
    GPIO_WriteBit(GPIOG, 1 << 6, (BitAction) 1);
  else if(i == 2)
    GPIO_WriteBit(GPIOG, 1 << 8, (BitAction) 2);
  else if(i == 3)
    GPIO_WriteBit(GPIOI, 1 << 9, (BitAction) 3);
  else
    GPIO_WriteBit(GPIOC, 1 << 7, (BitAction) 4);
}

void ledky(){
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  //napajeni a pripojeni na sbernici
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOI, ENABLE);
  
  /* nastavime kazdy pin pro led jako vystupni - push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOI, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void vypis(int row, char *c) {
  int i = 0;
  while(c[i] != '\0')
  {
    LCD_DisplayChar(row*10, 320 - i * 8, c[i]);
    i++;
  }
}

void debug(){
   LCD_Init();
   /* zvolime font - maly vs velky */
  LCD_SetFont(&Font8x8);
  //LCD_SetFont(&Font16x24);
  
  LCD_Clear(0xffff);
  
  vypis(1, "LCD :)");
  
  ledky();
  
  vypis(10, "LEDs :)");
  
//  led(1, getButtonValue(1));
//  led(2, getButtonValue(1));
//  led(3, getButtonValue(1));
//  led(4, getButtonValue(1));
  
}