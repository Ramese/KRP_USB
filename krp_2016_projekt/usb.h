// prevzato od kolegu

#include <stdint.h>

//swapni byty
#define  SWAPBYTE(addr)        ( ((uint16_t)(*((uint8_t *)(addr)))) + \
                               (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8) )

//vypuceno z netu
#define USB_G_BASE (USB_OTG_GREGS *)(USB_OTG_FS_BASE_ADDR )
#define USB_D_BASE (USB_OTG_DREGS *)(USB_OTG_FS_BASE_ADDR + USB_OTG_DEV_GLOBAL_REG_OFFSET)
#define EPin_Addr(x) (USB_OTG_INEPREGS *) (USB_OTG_FS_BASE_ADDR + USB_OTG_DEV_IN_EP_REG_OFFSET + (x * USB_OTG_EP_REG_OFFSET))
#define EPout_Addr(x) (USB_OTG_OUTEPREGS *) (USB_OTG_FS_BASE_ADDR + USB_OTG_DEV_OUT_EP_REG_OFFSET + (x * USB_OTG_EP_REG_OFFSET))
#define FIFO_Addr(x) (uint32_t *)(USB_OTG_FS_BASE_ADDR + USB_OTG_DATA_FIFO_OFFSET + (x * USB_OTG_DATA_FIFO_SIZE))

void USB_Init();
void MoveMouse(uint8_t *_mouseStat);

// stavy
enum DeviceState{
  USB_EP0_IDLE = 0,
  USB_EP0_SETUP = 1,
  USB_EP0_DATA_IN = 2,
  USB_EP0_DATA_OUT = 3,
  USB_EP0_STATUS_IN = 4,
  USB_EP0_STATUS_OUT = 5,
  USB_EP0_STALL = 6,
};

// struktura endpointu
struct EPStruct{ 
  uint8_t        num; // number
  uint8_t        is_in; // IN 1/0
  uint8_t        is_stall;
  uint8_t        type; // 0 - ridici
  uint8_t        data_pid_start;
  uint16_t       tx_fifo_num;
  uint32_t       maxpacket; 
  uint8_t        *xfer_buff; //data
  uint32_t       xfer_len; //velikost
  uint32_t       xfer_count;//pocet
  uint8_t	 even_odd_frame;//nvm
  uint32_t       total_data_len;//celkova velikost
  uint32_t       rem_data_len;//zbyvajici
  uint32_t       ctl_data_len;//ctrl data
};

//setup packet z netu
struct SetupPacket { //struktura prichozich paketu
  uint8_t   bmRequest;
  uint8_t   bRequest;
  uint16_t  wValue;
  uint16_t  wIndex;
  uint16_t  wLength;
};

