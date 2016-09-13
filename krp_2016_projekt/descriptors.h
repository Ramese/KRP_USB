#include "stm32f4xx.h"

uint8_t DeviceDescriptor[] = {
	0x12,  //len
	0x01,  //typ
	
        0x00,  //USB specification number
	0x02,  //USB specification number
        
	0x00,  //trida
	0x00,  //sub trida
	0x00,  //protokol
	0x40,  //maximum packet size
        
	0x00,  //vendor ID
	0x01,  //vendor ID
	
        0x00,  //product ID
	0x01,  //product ID
	
        0x00,  //device release number
	0x01,  //device release number
        
	0x01,  //index manufacturer string deskriptor
        0x02,  //index product string deskriptor
	0x03,  //index serial number string descriptor
	0x01   //number of possible configurations
};

uint8_t ConfigurationDescriptor[] = {
	0x09,  //len
	0x02,  //typ
	0x22,  //total length
	0x00,  //total length
	0x01,  //number of interfaces
	0x01,  //configuration value
	0x00,  //index of string desc
	0xE0,  //attributes
	0x64,  //max power
        
        //mouse
	0x09,  //len
	0x04,  //typ
	0x00,  //interface number
	0x00,  //alternate setting
	0x01,  //number of endpoints
	0x03,  //interface class
	0x01,  //sub class
	0x02,  //protocol
	0x00,  //string desc
        
	//mouse
	0x09,  //len
	0x21,  //typ
	0x00,  //release number
	0x01,  //release number
	0x00,  //HW cilena zeme
	0x01,  //desc count
	0x22,  //typ of desc
	0x4A,  //size
	0x00,
	
        //EP of mouse
	0x07,	//len
	0x05,	//typ
	0x81,	//EP address (IN)
	0x03,	//interrupt endpoint
	0x04,	//max data size
	0x00,
	0x01   //freq
};

uint8_t ControlEndPointDescriptor[] = {
        0x07, // len
        0x05, // typ
        0x00, // address
        0x00, // attributes
        0x40, // max packet size
        0x00  // interval polling
};

uint8_t MouseDataEndPointDescriptor[] = {
        0x07, // len
        0x05, // typ
        0x01, // address
        0x03, // attributes
        0x40, // max packet size
        0x01  // interval polling
};

uint8_t SerialDeskriptor[] = {
        0x00,
        0x00,
        0x01
};

unsigned char ConfigDeskriptor[] = {
        'C',
        'O',
        'N',
        'F',
        'I',
        'G'
};

unsigned char ManufacturerDeskriptor[] = {
        'M',
        'S',
        'I'
};

unsigned char ProductDeskriptor[] = {
        'M',
        'O',
        'U',
        'S',
        'E'
};

uint8_t InterfaceDeskriptor[] = {
        0x09,  //len
        0x04,  //typ
        0x00,  //interface number
        0x00,  //alternate setting
        0x01,  //number of endpoints
        0x03,  //interface class
        0x01,  //sub class
        0x02,  //protokol
        0x00   //index string desc.
};

uint8_t StringDescriptor[] = {
	0x04,    //len
	0x03,    //typ
	0x0409,  //lang0
	0x0C04,  //lang1
};

uint8_t MouseDescriptor[] = {
  
//  0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
//0x09, 0x02,                    // USAGE (Mouse)
//0xa1, 0x01,                    // COLLECTION (Application)
//0x09, 0x01,                    //   USAGE (Pointer)
//0xa1, 0x00,                    //   COLLECTION (Physical)
//0x05, 0x09,                    //     USAGE_PAGE (Button)
//0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
//0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
//0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
//0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
//0x95, 0x03,                    //     REPORT_COUNT (3)
//0x75, 0x01,                    //     REPORT_SIZE (1)
//0x81, 0x02,                    //     INPUT (Data,Var,Abs)
//0x95, 0x01,                    //     REPORT_COUNT (1)
//0x75, 0x05,                    //     REPORT_SIZE (5)
//0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
//0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
//0x09, 0x30,                    //     USAGE (X)
//0x09, 0x31,                    //     USAGE (Y)
//0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
//0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
//0x75, 0x08,                    //     REPORT_SIZE (8)
//0x95, 0x02,                    //     REPORT_COUNT (2)
//0x81, 0x06,                    //     INPUT (Data,Var,Rel)
//0xc0,                          //   END_COLLECTION
//0xc0                           // END_COLLECTION
  
  //funkcni:
	0x05,   0x01,   //usage page
	0x09,   0x02,   //usage (mouse)
	0xA1,   0x01,   //collection (application)
	0x09,   0x01,   //usage (pointer)

	0xA1,   0x00,   //collection (physical)
	0x05,   0x09,   //usage page (buttons)
	0x19,   0x01,   //usage minimum 
	0x29,   0x03,   //usage maximum

	0x15,   0x00,   //logical minimum
	0x25,   0x01,   //report count
	0x95,   0x03,   //report size
	0x75,   0x01,   //logical maximum

	0x81,   0x02,   //input (data,vars)
	0x95,   0x01,   //report count
	0x75,   0x05,   //report size
	0x81,   0x01,   //input (constants,vars)

	0x05,   0x01,   //usage page
	0x09,   0x30,   //usage (x)
	0x09,   0x31,   //usage (y)
	0x09,   0x38,   //usage

	0x15,   0x81,   //usage min
	0x25,   0x7F,   //usage max
	0x75,   0x08,   //report size
	0x95,   0x03,   //report count

	0x81,   0x06,   //input (data,vars)
	0xC0,           // end colection
        0x09,           // magic:
	0x3C,   0x05,
	0xFF,   0x09,

	0x01,   0x15,
	0x00,   0x25,
	0x01,   0x75,
	0x01,   0x95,

	0x02,   0xB1,
	0x22,   0x75,
	0x06,   0x95,
	0x01,   0xB1,

	0x01,   
        0xC0 // end collection
};