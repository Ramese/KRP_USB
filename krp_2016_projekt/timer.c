#include <string.h>
#include "timer.h"
#include "debug.h"
#include "usb.h"
#include "stm32f4xx_conf.h"
#include "stm324xg_eval_ioe.h"

#define MOVEMENT 5

// priznak pro poznani USB init done
static int isEnabled = 0;
static int timerLed = 0;
static int timerEnabled = 0;

void setEnabled(){
  isEnabled = 1;
}

void setDisabled(){
  isEnabled = 0;
}

int getButtonValue(int i) { // nepouzivat pred init()
  if(i == 1) 
    return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == Bit_RESET;
  else if(i == 2)
    return GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_15) == Bit_RESET;
  return 0;
}

void timer(){
  uint16_t PrescalerValue = 0;
  PrescalerValue = (uint16_t) ((SystemCoreClock/2)/1000000) -1;
  
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_SetPriority(TIM3_IRQn, 0x3); 

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  
  TIM_TimeBaseStructure.TIM_Period = 100000-1; // 1ms
  TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  TIM_Cmd(TIM3, ENABLE);
  
}

extern "C" 
{
  // dobre vedet ze jsou preddefinovane nazvy funkci ktere se volaji pri interrup
  void TIM3_IRQHandler(void)
  {
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
      // reset priznaku preruseni
      TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

      //led(2, timerLed);
      timerLed = !timerLed;
      if(isEnabled != 0){
        /* DEBUG */
        led(4, timerEnabled);
        timerEnabled = !timerEnabled;
        //buffer[0] - buttons
        //buffer[1] - X -
        //buffer[2] - Y |
        int8_t xpos = 0;
        int8_t ypos = 0;
        static uint8_t buffer[4] = {0,0,0,0};
        vypis(16, "  ");
        switch (IOE_JoyStickGetState()){ // fce from eval lib.
          case JOY_LEFT: 
            xpos -= MOVEMENT; 
            vypis(16, "<-");
            break;
          case JOY_RIGHT: 
            xpos += MOVEMENT; 
            vypis(16, "->");
            break;
          case JOY_UP: 
            ypos -= MOVEMENT; 
            vypis(16, "/\\");
            break;
          case JOY_DOWN: 
            ypos += MOVEMENT; 
            vypis(16, "\\/");
            break;
        }
        
        buffer[0] = 0;
        //vypis(13, "0 LEFT");
        if(getButtonValue(1) == 1){
          buffer[0] = 1;
          //vypis(13, "1 LEFT");
        }
        //vypis(14, "0 RIGHT");
        if(getButtonValue(2) == 1){
          buffer[0] |= 2;
          //vypis(14, "1 RIGHT");
        }
        buffer[1] = xpos;
        buffer[2] = ypos;
        MoveMouse(buffer);
      }
    }
  }
}




