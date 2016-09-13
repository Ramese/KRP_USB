#include "usb_core.h"
#include "usb_regs.h"
#include "usb_defines.h"
#include "stm32f4xx.h"


#include "debug.h"
#include "descriptors.h"
#include "usb.h"
#include "timer.h"
#include "lcd.h"

//http://www.usbmadesimple.co.uk/ums_7.htm

//ACK
//Receiver acknowledges receiving error free packet.

//NAK
//Receiving device cannot accept data or transmitting device cannot send data.

//STALL
//Endpoint is halted, or control pipe request is not supported.

//NYET
//No response yet from receiver (high speed only).

USB_OTG_GREGS *usbGREGS = USB_G_BASE;// globalni USB base
USB_OTG_DREGS *usbDREGS = USB_D_BASE;// device base

//endpoints
USB_OTG_INEPREGS *inEPAddr[2];
USB_OTG_OUTEPREGS *outEPAddr;
__IO uint32_t   *FIFOAddr[2];

EPStruct	inEPs[2];
EPStruct	outEP0;	
uint8_t		PACKET[8 * 3];
DeviceState	deviceState;  

// zapnutí pinu a mapovani
void USB_GPIO(){
  // schematicky manual (ten mensi)
  
  GPIO_InitTypeDef GPIO_Initstructure;
  
  //napajeni a pripojeni na sbernici
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE);
  
  //10 - ID
  //11 - DM
  //12 - DP
  GPIO_Initstructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_Initstructure.GPIO_Mode	= GPIO_Mode_AF;
  GPIO_Initstructure.GPIO_Speed	= GPIO_Speed_100MHz;
  GPIO_Initstructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
  GPIO_Initstructure.GPIO_OType	= GPIO_OType_PP;
  GPIO_Init(GPIOA, &GPIO_Initstructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

  //9 - VBUS
  GPIO_Initstructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_Initstructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Initstructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_Initstructure.GPIO_OType = GPIO_OType_OD;
  GPIO_Initstructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_Initstructure);
}

void USB_Init(){
   //regs 1389:
  USB_OTG_GAHBCFG_TypeDef	gahbcfg;
  USB_OTG_GUSBCFG_TypeDef	gusbcfg;
  USB_OTG_GINTMSK_TypeDef       gintmsk;
  USB_OTG_GCCFG_TypeDef		gccfg;
  USB_OTG_DCFG_TypeDef		dcfg;
  USB_OTG_FSIZ_TypeDef		fifosizes; // fifo 1279
  USB_OTG_DIEPMSK_TypeDef	diepmsk;
  __IO USB_OTG_GRSTCTL_TypeDef  grstctl;
  
  USB_GPIO(); // zapnutí pinu, mapovani

  //High Speed APB (APB2) peripheral clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); 
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1Periph_PWR, ENABLE);

  //configurace str. 1324:
  gahbcfg.d32 = 0;
  gahbcfg.d32 = USB_OTG_READ_REG32(&usbGREGS->GAHBCFG);
  gahbcfg.b.glblintrmsk = 1;// interrupt on
  USB_OTG_WRITE_REG32(&usbGREGS->GAHBCFG, gahbcfg.d32);

  gusbcfg.d32 = 0;
  gusbcfg.d32 = USB_OTG_READ_REG32(&usbGREGS->GUSBCFG);
  gusbcfg.b.physel = 1; // full-speed
  gusbcfg.b.hnpcap = 0; // hnp off
  gusbcfg.b.srpcap = 0; // srp off
  gusbcfg.b.usbtrdtim = 5; //table 208
  USB_OTG_WRITE_REG32(&usbGREGS->GUSBCFG, gusbcfg.d32);

  //Reset
  do{
    grstctl.d32 = USB_OTG_READ_REG32(&usbGREGS->GRSTCTL);
  }while(grstctl.b.ahbidle == 0); // AHB bus wait
  
  grstctl.b.csftrst = 1;
  USB_OTG_WRITE_REG32(&usbGREGS->GRSTCTL, grstctl.d32);
  
  do{
    grstctl.d32 = USB_OTG_READ_REG32(&usbGREGS->GRSTCTL);
  }while(grstctl.b.csftrst == 1); // core soft reset

  // recive fifo size 
  // zmineno str 1325, 1342 
  // def.  1412
  USB_OTG_WRITE_REG32(&usbGREGS->GRXFSIZ, 128);
  
  // velikost a startovni adresa transmit fifa
  fifosizes.d32 = 0;
  fifosizes.b.depth = 64;
  fifosizes.b.startaddr = 128;
  // 1417, 1279
  USB_OTG_WRITE_REG32(&usbGREGS->DIEPTXF0_HNPTXFSIZ, fifosizes.d32);
  
  gusbcfg.d32 = USB_OTG_READ_REG32(&usbGREGS->GUSBCFG);
  gusbcfg.b.force_host = 0;// host off
  gusbcfg.b.force_dev = 1;// device on
  USB_OTG_WRITE_REG32(&usbGREGS->GUSBCFG, gusbcfg.d32);

  //device conf 1431:
  dcfg.d32 = 0;
  dcfg.d32 = USB_OTG_READ_REG32(&usbDREGS->DCFG);
  dcfg.b.devspd	= 3; // full-speed
  dcfg.b.perfrint = 0; //80%
  dcfg.b.nzstsouthshk = 0; // handshake on NAK and STALL bit in endpoint
  USB_OTG_WRITE_REG32(&usbDREGS->DCFG, dcfg.d32);
  
  //1416
  gccfg.d32 = 0;
  gccfg.b.pwdn = 1; // power down deactived
  gccfg.b.vbussensingB = 1; //enable Vbus sensing in B device
  USB_OTG_WRITE_REG32(&usbGREGS->GCCFG, gccfg.d32);

  //1279
  // tx0 = 64 // control 8-64
  // rx0 = 128
  // tx1 = 128
  fifosizes.b.depth = 128;
  fifosizes.b.startaddr = 192;
  USB_OTG_WRITE_REG32(&usbGREGS->DIEPTXF[0], fifosizes.d32);

  //1435
  diepmsk.d32 = 0;
  diepmsk.b.txfifoundrn = 1;
  USB_OTG_MODIFY_REG32(&usbDREGS->DIEPMSK, diepmsk.d32, diepmsk.d32);

  //povol urcite interrupty handlovane v IRQ
  gintmsk.d32 = 0;
  gintmsk.b.usbreset = 1; // unmasked
  gintmsk.b.enumdone = 1; // unmasked // enumeration
  gintmsk.b.inepintr = 1; // unmasked // IN endpoint interrupt
  gintmsk.b.outepintr = 1; // unmasked // out endpoint 
  gintmsk.b.rxstsqlvl = 1; // unmasked // we
  USB_OTG_MODIFY_REG32(&usbGREGS->GINTMSK, gintmsk.d32, gintmsk.d32);
  
  //1397, 1265
  gahbcfg.b.glblintrmsk = 1; //unmasked
  USB_OTG_MODIFY_REG32(&usbGREGS->GAHBCFG, 0, gahbcfg.d32);
        
  //Interrupt init  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  //1342
  EPStruct *ep;
  //EP0
  ep = &inEPs[0];
  ep->is_in = 1; // IN
  ep->num = 0; //number
  ep->tx_fifo_num = 0;
  ep->type = 0; // control
  ep->maxpacket = 64;
  ep->xfer_buff = 0;
  ep->xfer_len = 0;
  
  //EP0
  ep = &inEPs[1];
  ep->is_in = 1; // IN
  ep->num = 1;
  ep->tx_fifo_num = 1;
  ep->type = 1;
  ep->maxpacket = 64;
  ep->xfer_buff = 0;
  ep->xfer_len = 0;

  //EP0 prijmani dat
  ep = &outEP0;
  ep->is_in = 0; // OUT
  ep->num = 0;
  ep->tx_fifo_num = 0;
  ep->type = 0; // control
  ep->maxpacket = 64;
  ep->xfer_buff = 0;
  ep->xfer_len = 0;
  
  //adresy jednotlivych endpointu
  outEPAddr = EPout_Addr(0);
  inEPAddr[0] = EPin_Addr(0);
  inEPAddr[1] = EPin_Addr(1);
  
  FIFOAddr[0] = FIFO_Addr(0);
  FIFOAddr[1] = FIFO_Addr(1);
  
  led_on(3);//mám endpointy :)
}

void Read(){
  // 1344
  // 1277
  //vypis(13, "read start");
  //vypis(14, "            ");
  USB_OTG_DRXSTS_TypeDef   grxstsp;
  // 1274
  USB_OTG_GINTMSK_TypeDef  gintmask;
  int i, len;
  uint8_t *destination;
  __IO uint32_t *faddr = FIFOAddr[0];
  destination = PACKET;
  len = (8 + 3) / 4; 
  grxstsp.d32 = USB_OTG_READ_REG32(&usbGREGS->GRXSTSP);
  
  gintmask.d32 = 0;
  gintmask.b.rxstsqlvl = 1; // interrupt disable
  USB_OTG_MODIFY_REG32(&usbGREGS->GINTMSK, gintmask.d32, 0);
  
  if (grxstsp.b.pktsts == STS_SETUP_UPDT){
    for (i = 0; i < len; i++, destination += 4){
      *(__packed uint32_t *)destination = USB_OTG_READ_REG32(faddr); // read fifo
    }
  }
  USB_OTG_MODIFY_REG32(&usbGREGS->GINTMSK, 0, gintmask.d32); //interrupt enable
  
  //vypis(14, "read done");
  //vypis(13, "            ");
}

void WritePacket(uint8_t *buffernum, uint8_t epnum, uint16_t length){
  uint32_t i, x = (length + 3) / 4;
  __IO uint32_t *fifoaddr = FIFOAddr[epnum];
  for (i = 0; i < x; i++, buffernum+=4){
    USB_OTG_WRITE_REG32(fifoaddr, *((__packed uint32_t *)buffernum)); // write to fifo
  }
}

void ControlEndPointSend(uint8_t * data, uint16_t length){
  //1301
  USB_OTG_DEPCTL_TypeDef      depctl; // device endpoint controll
  //1310
  USB_OTG_DEP0XFRSIZ_TypeDef  deptsiz; // device IN endpoint 0 transfer size register
  EPStruct *ep = &inEPs[0];
  
  ep->xfer_buff = data; // data
  ep->xfer_count = 0;
  ep->xfer_len = length;
  
  depctl.d32 = 0;
  depctl.d32 = USB_OTG_READ_REG32(&inEPAddr[0]->DIEPCTL);
  
  deptsiz.d32 = 0;
  deptsiz.d32 = USB_OTG_READ_REG32(&inEPAddr[0]->DIEPTSIZ);
  
  if (length == 0)
  {
    deptsiz.b.xfersize = 0;
    deptsiz.b.pktcnt = 1;
  }
  else
  {
    if(length > ep->maxpacket)
    {
      ep->xfer_len = ep->maxpacket;
      deptsiz.b.xfersize = ep->maxpacket;
    }
    else
    {
      deptsiz.b.xfersize = length;
    }
    deptsiz.b.pktcnt = 1;
  }
  USB_OTG_WRITE_REG32(&inEPAddr[0]->DIEPTSIZ, deptsiz.d32);

  depctl.b.cnak = 1;
  depctl.b.epena = 1; // enable endpoint
  USB_OTG_WRITE_REG32(&inEPAddr[0]->DIEPCTL, depctl.d32);

  //zapni tx fifo mask
  if (ep->xfer_len > 0){
    //1299
    //co, clear mask, set mask
    USB_OTG_MODIFY_REG32(&usbDREGS->DIEPEMPMSK, 0, 1);
  }
}

void DescriptorRequest(SetupPacket* req){
  uint16_t length;
  uint8_t *data;
  
  switch (req->wValue >> 8)
  {
    case 1:
      data = DeviceDescriptor;
      length = sizeof(DeviceDescriptor);
      break;
    case 2: 
      data = ConfigurationDescriptor;
      data[1] = 2; //id
      length = sizeof(ConfigurationDescriptor);
      break;
    case 3: // sub desc
      switch ((uint8_t)(req->wValue)){ 
      case 0:	
        data = StringDescriptor; 
        length = sizeof(StringDescriptor); 
        break;
      case 1:	
        data = ManufacturerDeskriptor; 
        length = sizeof(ManufacturerDeskriptor);
        break;
      case 2:	
        data = ProductDeskriptor; 
        length = sizeof(ProductDeskriptor); 
        break;
      case 3:	
        data = SerialDeskriptor;
        length = sizeof(SerialDeskriptor); 
        break;
      case 4:
        data = 	ConfigDeskriptor;
        length = sizeof(ConfigDeskriptor);
        break;
      case 5:  
        data = InterfaceDeskriptor; 
        length = sizeof(InterfaceDeskriptor);	
        break;
      default: 
        return;
      }
      break;
    default: 
      return;
  }

  if ((length != 0) && (req->wLength != 0))
  {
    length = MIN(length, req->wLength);
    inEPs[0].total_data_len = length; 
    inEPs[0].rem_data_len = length;
    deviceState = USB_EP0_DATA_IN;
    ControlEndPointSend(data, length);
  }
}

void AddressRequest(SetupPacket* req){
  //1343
  uint8_t  adress = (uint8_t)(req->wValue) & 0x7F; //7 bitu
  USB_OTG_DCFG_TypeDef  dcfg; // device configuration registrer
  
  dcfg.d32 = 0;
  dcfg.b.devaddr = adress;
  
  USB_OTG_MODIFY_REG32( &usbDREGS->DCFG, 0, dcfg.d32);
  
  deviceState = USB_EP0_STATUS_IN; //status in packet
  ControlEndPointSend(0,0); //handshake
}

void InterfaceRequest(SetupPacket* req){
  static uint32_t  HIDProtocol;
  static uint32_t  HIDIdle;
  //1311
  USB_OTG_DEP0XFRSIZ_TypeDef  doeptsize0; //device OUT endpoint 0 transfer size register
  uint16_t length = 0;
  uint8_t  *pbuf = 0;

  switch (req->bmRequest & 0x60){
    case 0x00: //request class desc
      length = MIN(74, req->wLength);
      pbuf = MouseDescriptor;
      inEPs[0].total_data_len = length;
      inEPs[0].rem_data_len = length;
      deviceState = USB_EP0_DATA_IN;
      ControlEndPointSend(pbuf, length);
      break;
    
    case 0x20: //request na type class
      switch (req->bRequest){
        case 0x0B: //set protocol
          HIDProtocol = (uint8_t)(req->wValue);
          break;

        case 0x03: //get protocol
          inEPs[0].total_data_len = length;
          inEPs[0].rem_data_len = length;
          deviceState = USB_EP0_DATA_IN;
          ControlEndPointSend((uint8_t *)&HIDProtocol, length);
          break;

        case 0x0A: //set idle
          HIDIdle = (uint8_t)(req->wValue >> 8);
          break;

        case 0x02: //get idle
          inEPs[0].total_data_len = length;
          inEPs[0].rem_data_len = length;
          deviceState = USB_EP0_DATA_IN;
          ControlEndPointSend((uint8_t *)&HIDIdle, length);
          break;

        default: break;
      }
    break;
   }

  if (req->wLength == 0){
    deviceState = USB_EP0_STATUS_IN;
    ControlEndPointSend(0, 0); // handshake
    
    doeptsize0.d32 = 0;
    doeptsize0.b.supcnt = 3; // setup count
    doeptsize0.b.pktcnt = 1; // packet count
    doeptsize0.b.xfersize = 8 * 3; // transfer size
    USB_OTG_WRITE_REG32(&outEPAddr->DOEPTSIZ, doeptsize0.d32);
  }
}

void ConfigRequest(SetupPacket* req){
  // 1343
  // 1301
  USB_OTG_DEPCTL_TypeDef  depctl; // device endpoint-x control register
  // 1298
  USB_OTG_DAINT_TypeDef  daintmsk; //all endpoints interrupt mask register
  
  depctl.d32 = 0;
  depctl.d32 = USB_OTG_READ_REG32(&inEPAddr[1]->DIEPCTL);
  
  if (!depctl.b.usbactep) // not if already setup
  { 
    depctl.b.mps = 4;
    depctl.b.eptype = 3;
    depctl.b.txfnum = 1;
    depctl.b.setd0pid = 1;
    depctl.b.usbactep = 1;
    USB_OTG_WRITE_REG32(&inEPAddr[1]->DIEPCTL, depctl.d32); // setup EP1
  }
  
  daintmsk.d32 = 0;
  daintmsk.ep.in = 2;
  
  USB_OTG_MODIFY_REG32(&usbDREGS->DAINTMSK, 0, daintmsk.d32); //interrupt on
  
  deviceState = USB_EP0_STATUS_IN;
  ControlEndPointSend(0,0); // handshake packet
}

void OutEndpointInterrupt(){
  //1309
  USB_OTG_DOEPINTn_TypeDef  doepint; // device endpoint-x interrupt register
  SetupPacket req;
  
  doepint.d32 = 0;
  doepint.d32 = USB_OTG_READ_REG32(&outEPAddr->DOEPINT);
  doepint.d32 = doepint.d32 & USB_OTG_READ_REG32(&usbDREGS->DOEPMSK); //mask

  if ((doepint.d32 & 0x00000008) == 0x00000008){ //incomming packet
    //read packet and save it to req register
    req.bmRequest = *(uint8_t *)(PACKET); // precti packet a uloz req
    req.bRequest = *(uint8_t *)(PACKET + 1);
    req.wValue = SWAPBYTE(PACKET + 2);
    req.wIndex = SWAPBYTE(PACKET + 4);
    req.wLength = SWAPBYTE(PACKET + 6);
    inEPs[0].ctl_data_len = req.wLength;
    deviceState = USB_EP0_SETUP;
    
    switch (req.bmRequest & 0x1F){
      case 0: //device request
        switch (req.bRequest)
        {
          case 5:
            AddressRequest(&req);
            break;
          case 6:	
            DescriptorRequest(&req);
            break;
          case 9:
            ConfigRequest(&req);
            break; 
          default:
            break;
        }
        break;
      case 1:
        InterfaceRequest(&req);
        break;
      default:
        break;
      }
      
      doepint.b.setup = 1;
      USB_OTG_WRITE_REG32(&outEPAddr->DOEPINT, doepint.d32); // setup done
  }
}

void InEndpointInterrupt(){
  //1308
  USB_OTG_DIEPINTn_TypeDef  diepint; //device endpoint-x interrupt register
  USB_OTG_DEPCTL_TypeDef      depctl; //device endpoint-x control register
  //1311
  USB_OTG_DEP0XFRSIZ_TypeDef  deptsiz; // device OUT endpoint 0 transfer size register
  USB_OTG_DEP0XFRSIZ_TypeDef  doeptsize0;
  //1313
  USB_OTG_DTXFSTSn_TypeDef  txstatus; // device IN endpoint transmit FIFO status register
  
  uint32_t intr;
  uint32_t epnumber = 0;
  uint32_t msk1,msk2;
  diepint.d32 = 0;
  
  //1298
  intr = USB_OTG_READ_REG32(&usbDREGS->DAINT); // device all endpoints interrupt register
  // only in interrupts
  intr &= USB_OTG_READ_REG32(&usbDREGS->DAINTMSK);// all endpoints interrupt mask register
  //vypis(15, "while start");
  //vypis(16, "             ");
  while (intr){
  //for(i = 0; i < 32; i++) {
    if (intr&0x1){ //mask if interrupt
      msk1 = USB_OTG_READ_REG32(&usbDREGS->DIEPEMPMSK);
      msk2 = USB_OTG_READ_REG32(&usbDREGS->DIEPMSK);
      msk2 |= ((msk1 >> epnumber) & 0x1) << 7;
      if(epnumber > 1)
        break;
      diepint.d32 = USB_OTG_READ_REG32(&inEPAddr[epnumber]->DIEPINT) & msk2;
      //vypis(21, "d");
      if (diepint.b.xfercompl){ //transfer completed
        USB_OTG_MODIFY_REG32(&usbDREGS->DIEPEMPMSK, 0x1, 0);
        diepint.b.xfercompl = 1; //set interrupt
        USB_OTG_WRITE_REG32(&inEPAddr[epnumber]->DIEPINT,diepint.d32);
        if(epnumber == 0){ //ep0
          EPStruct *ep = &inEPs[0]; 
          if (deviceState == 2){ //DATA IN
            if (ep->rem_data_len > ep->maxpacket){ // data jeste budou
              ep->rem_data_len -= ep->maxpacket;
              //vypis(17, "1");
              ControlEndPointSend(ep->xfer_buff, ep->rem_data_len);
            }else{ //poslendi paket 
              if ((ep->total_data_len % ep->maxpacket == 0) &&
                      (ep->total_data_len >= ep->maxpacket) &&
                      (ep->total_data_len < ep->ctl_data_len)){
                //vypis(18, "2");
                ControlEndPointSend(0, 0);
                ep->ctl_data_len = 0;
              }else{ // prijem prazdneho packetu
                //vypis(19, "3");
                deviceState = USB_EP0_STATUS_OUT;
                
                ep = &outEP0;
                
                depctl.d32 = 0;
                depctl.d32 = USB_OTG_READ_REG32(&outEPAddr->DOEPCTL);
                
                deptsiz.d32 = 0;
                deptsiz.d32 = USB_OTG_READ_REG32(&outEPAddr->DOEPTSIZ);
                
                if (ep->xfer_len == 0){
                  deptsiz.b.xfersize = ep->maxpacket;
                  deptsiz.b.pktcnt = 1;
                }else{
                  ep->xfer_len = ep->maxpacket;
                  deptsiz.b.xfersize = ep->maxpacket;
                  deptsiz.b.pktcnt = 1;
                }
                USB_OTG_WRITE_REG32(&outEPAddr->DOEPTSIZ, deptsiz.d32);

                depctl.b.cnak = 1;
                depctl.b.epena = 1;
                USB_OTG_WRITE_REG32(&outEPAddr->DOEPCTL, depctl.d32);
                
                doeptsize0.d32 = 0;
                doeptsize0.b.supcnt = 3;
                doeptsize0.b.pktcnt = 1;
                doeptsize0.b.xfersize = 8 * 3;
                USB_OTG_WRITE_REG32(&outEPAddr->DOEPTSIZ, doeptsize0.d32);
              }
            }
          }
        }
      }
      if (diepint.b.emptyintr){// fifo is empty
        EPStruct *ep;
        uint32_t len = 0;
        uint32_t len32b;
        txstatus.d32 = 0;
        ep = &inEPs[epnumber];    
        len = ep->xfer_len - ep->xfer_count;
        if (len > ep->maxpacket){
                len = ep->maxpacket;
        }
        len32b = (len + 3) / 4;
        txstatus.d32 = USB_OTG_READ_REG32( &inEPAddr[0]->DTXFSTS);
        //vypis(13, "while2 start");
        //vypis(14, "             ");
        while (txstatus.b.txfspcavail > len32b && ep->xfer_count < ep->xfer_len &&
                ep->xfer_len != 0){
          len = ep->xfer_len - ep->xfer_count;
          if (len > ep->maxpacket){
            len = ep->maxpacket;
          }
          len32b = (len + 3) / 4;
          WritePacket(ep->xfer_buff, epnumber, len); // send data
          ep->xfer_buff  += len;
          ep->xfer_count += len;
          txstatus.d32 = USB_OTG_READ_REG32(&inEPAddr[0]->DTXFSTS);
        }
        
        //vypis(13, "             ");
        //vypis(14, "while2 end");
        
        diepint.b.emptyintr = 1;
        USB_OTG_WRITE_REG32(&inEPAddr[epnumber]->DIEPINT,diepint.d32); //set interrupt
      }
    }
    epnumber+=1;
    intr >>= 1;
  }
  //vypis(15, "             ");
  //vypis(16, "while end");
}

void Reset(){
  // 1342
  // 1298
  USB_OTG_DAINT_TypeDef    daintmsk; // device all endpoints interrupt register
  // 1297
  USB_OTG_DOEPMSK_TypeDef  doepmsk; // device OUT endpoint common interrupt mask register
  USB_OTG_DIEPMSK_TypeDef  diepmsk; // device IN endpoint common interrupt mask register
  // 1294
  USB_OTG_DCTL_TypeDef     dctl; // device control register
  USB_OTG_GINTSTS_TypeDef  gintsts; 
  // 1311
  USB_OTG_DEP0XFRSIZ_TypeDef  doeptsize0; // device OUT endpoint 0 transfer size register

  USB_OTG_DCFG_TypeDef     dcfg;
  
  dctl.d32 = 0;
  daintmsk.d32 = 0;
  doepmsk.d32 = 0;
  diepmsk.d32 = 0;
  dcfg.d32 = 0;
  gintsts.d32 = 0;
  
  //doepctl
  dctl.d32 = 0;
  dctl.b.rmtwkupsig = 1;
  USB_OTG_MODIFY_REG32(&usbDREGS->DCTL, dctl.d32, 0);
  
  //USBFlushTxFIFO(0x10);
  //USBFlushRxFIFO();
  
  USB_OTG_WRITE_REG32(&inEPAddr[0]->DIEPINT,0xFF);
  USB_OTG_WRITE_REG32(&inEPAddr[1]->DIEPINT,0xFF);
  USB_OTG_WRITE_REG32(&outEPAddr->DOEPINT, 0xFF);
  
  USB_OTG_WRITE_REG32(&usbDREGS->DAINT, 0xFFFFFFFF );
  
  daintmsk.d32 = 0;
  daintmsk.ep.in = 1; // interrupt on EP0 in
  daintmsk.ep.out = 1; // interrupt on EP0 out
  USB_OTG_WRITE_REG32(&usbDREGS->DAINTMSK, daintmsk.d32);
  
  doepmsk.d32 = 0;
  doepmsk.b.setup = 1; 
  doepmsk.b.xfercompl = 1;
  doepmsk.b.epdisabled = 1;
  USB_OTG_WRITE_REG32(&usbDREGS->DOEPMSK, doepmsk.d32); //setup, transfer done
  
  diepmsk.d32 = 0;
  diepmsk.b.xfercompl = 1;
  diepmsk.b.timeout = 1;
  diepmsk.b.epdisabled = 1;
  USB_OTG_WRITE_REG32(&usbDREGS->DIEPMSK, diepmsk.d32);
  
  /* Reset Device Address */
  dcfg.d32 = USB_OTG_READ_REG32(&usbDREGS->DCFG);
  dcfg.b.devaddr = 0;
  USB_OTG_WRITE_REG32(&usbDREGS->DCFG, dcfg.d32);
  
  doeptsize0.d32 = 0;
  doeptsize0.b.supcnt = 3;
  doeptsize0.b.pktcnt = 1;
  doeptsize0.b.xfersize = 3 * 8;
  USB_OTG_WRITE_REG32(&outEPAddr->DOEPTSIZ, doeptsize0.d32);

  gintsts.d32 = 0;
  gintsts.b.usbreset = 1; // usb reset on
  USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts.d32); // clear interrupt
  
  setEnabled();
}

static int irqFS = 0;

extern "C" {
  void OTG_FS_IRQHandler(void){
    USB_OTG_GINTSTS_TypeDef gintsts; //global interrupt register
    USB_OTG_GINTSTS_TypeDef gintsts2; //global interrupt register
    gintsts.d32 = USB_OTG_READ_REG32(&usbGREGS->GINTSTS);
    
    led(1, irqFS);
    irqFS = !irqFS;
    
    if (!gintsts.d32) /* avoid spurious interrupt */
    {
      //vypis(1, "spurious  ");
      return;
    } else {
      //vypis(1, "           ");
    }
    
    if (gintsts.b.rxstsqlvl){ // usb prijmani dat
      Read();
      vypis(2, "read  ");
    } else {
      vypis(2, "           ");
    }
    
    if (gintsts.b.outepintr){ // incomming
      OutEndpointInterrupt();
      //vypis(3, "outepintr  ");
    } else {
      //vypis(3, "           ");
    }
    
    
    
    if (gintsts.b.inepint){ // sending
      InEndpointInterrupt();
      //vypis(4, "inepint  ");
    } else {
      //vypis(4, "           ");
    }
    
    if (gintsts.b.modemismatch)
    {
      /* Clear interrupt */
      gintsts2.d32 = 0;
      gintsts2.b.modemismatch = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts2.d32);
      //vypis(5, "mismatch  ");
    } else {
      //vypis(5, "           ");
    }
    
    if (gintsts.b.wkupintr)
    {
      //vypis(6, "wkupintr  ");
    } else {
      //vypis(6, "           ");
    }
    
    if (gintsts.b.sofintr)
    {
      //start of frame = SOF
      gintsts2.d32 = 0;
      gintsts2.b.sofintr = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts2.d32);
      
      vypis(7, "sofintr  ");
    } else {
      vypis(7, "           ");
    }
    
    if (gintsts.b.usbreset){
      //USB_Init();
      Reset();
      vypis(8, "usbreset  ");
    } else {
      vypis(8, "           ");
    }
    
    if (gintsts.b.enumdone){
      gintsts.d32 = 0;
      gintsts.b.enumdone = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts.d32); // interrupt clear
      vypis(9, "enumdone  ");
    } else {
      vypis(9, "           ");
    }
    
    if (gintsts.b.usbsuspend)
    {
      setDisabled();
      gintsts.d32 = 0;
      gintsts.b.usbsuspend = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts.d32); // interrupt clear
      vypis(10, "usbsuspend  ");
    } else {
      vypis(10, "           ");
    }
    
    if (gintsts.b.incomplisoin)
    {
      gintsts2.d32 = 0;
      gintsts2.b.incomplisoin = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts2.d32); // interrupt clear
      //vypis(11, "incomplisoin  ");
    } else {
      //vypis(11, "           ");
    }

    if (gintsts.b.incomplisoout)
    {
      gintsts2.d32 = 0;
      gintsts2.b.incomplisoout = 1;
      USB_OTG_WRITE_REG32(&usbGREGS->GINTSTS, gintsts2.d32); // interrupt clear
      //vypis(12, "incomplisoout  ");
    } else {
      //vypis(12, "           ");
    }
  }
}

void MoveMouse(uint8_t *position){
  
  //1443
  USB_OTG_DEPCTL_TypeDef     depctl; // device endpoint control register
  USB_OTG_DEPXFRSIZ_TypeDef  deptsiz; //device endpoint fifo size
  uint32_t fifoemptymsk = 0;
  
  //1356
  EPStruct *ep = &inEPs[1]; // pouziti endpointu pro mys
  
  ep->xfer_buff = position; // data set
  ep->xfer_count = 0;
  ep->xfer_len = 4;

  depctl.d32 = 0;
  depctl.d32  = USB_OTG_READ_REG32(&(inEPAddr[1]->DIEPCTL));
  
  deptsiz.d32 = 0;
  deptsiz.d32 = USB_OTG_READ_REG32(&(inEPAddr[1]->DIEPTSIZ));
  deptsiz.b.xfersize = ep->xfer_len;
  deptsiz.b.pktcnt = (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
  USB_OTG_WRITE_REG32(&inEPAddr[1]->DIEPTSIZ, deptsiz.d32);

  //zapni prazdne tx pro ep
  if (ep->xfer_len > 0){
    fifoemptymsk = 2; //posli na fifo 1
    USB_OTG_MODIFY_REG32(&usbDREGS->DIEPEMPMSK, 0, fifoemptymsk);
  }
  
  depctl.b.cnak = 1; // clear nak
  depctl.b.epena = 1; // start transmit data
  USB_OTG_WRITE_REG32(&inEPAddr[1]->DIEPCTL, depctl.d32);
}