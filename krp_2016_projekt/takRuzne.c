// timer ze cviceni - nepouzivam
//void timer2() {
//  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//  NVIC_InitTypeDef NVIC_InitStructure;
//  
//  /* inicializuj casovac */
//  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);
//
//  TIM_TimeBaseStructure.TIM_Period = 46875; // 1000-1 = 1ms?
//  TIM_TimeBaseStructure.TIM_Prescaler = 1024;
//  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
//  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
//  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
//  
//  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
//   
//  TIM_Cmd(TIM2, ENABLE);
//}

//// fifo str 1401
//void USBFlushTxFIFO(uint32_t val){
//  usbGREGS->GRSTCTL = ( 0x00000020 |(uint32_t)( val << 5 )); 
//  while ((usbGREGS->GRSTCTL & 0x00000020) == 0x00000020);
//}
//
//// fifo str 1401
//void USBFlushRxFIFO(){
//  usbGREGS->GRSTCTL = 0x00000010;
//  while ((usbGREGS->GRSTCTL & 0x00000010) == 0x00000010);
//}

  //flushing
//  USBFlushTxFIFO(0x10);
//  USBFlushRxFIFO();