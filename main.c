/*
wch-linke mode switch

Dap to RV:
ffff9fcd45da6240 706500407 S Bo:1:078:2 -115 4 = 810d0101
ffff9fcd45da6240 706500542 C Bo:1:078:2 0 4 >
ffff9fcd45da6240 706500623 S Bi:1:078:3 -115 10 <
ffff9fcd45da6240 706500747 C Bi:1:078:3 0 7 = 820d0402 080201
ffff9fcd45da6240 706500936 S Bo:1:078:2 -115 4 = 81ff0152
ffff9fcd45da6240 706501055 C Bo:1:078:2 0 4 >

RV to Dap:
ffff9fcd5ec6a900 734970765 S Bo:1:079:1 -115 4 = 810d01ff
ffff9fcd5ec6a900 734970853 C Bo:1:079:1 0 4 >
ffff9fcd5ec6a900 734970913 S Bi:1:079:1 -115 4 <
ffff9fcd5ec6a900 734984158 C Bi:1:079:1 0 4 = 820d01ff
ffff9fcd5ec6a900 738192878 S Bo:1:079:1 -115 4 = 810d0101
ffff9fcd5ec6a900 738192956 C Bo:1:079:1 0 4 >
ffff9fcd5ec6a900 738193009 S Bi:1:079:1 -115 10 <
ffff9fcd5ec6a900 738193178 C Bi:1:079:1 0 7 = 820d0402 080200
ffff9fcd5ec6a900 738193322 S Bo:1:079:1 -115 4 = 81ff0141
ffff9fcd5ec6a900 738193401 C Bo:1:079:1 0 4 >
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <string.h>
#include <libusb.h>

//RV to DAP
#define RV_MODE_EP_OUT 0x01
#define RV_MODE_EP_IN 0x81

//DAP to RV
#define DAP_MODE_EP_OUT 0x02
#define DAP_MODE_EP_IN 0x83

#define EP2_IN 0x82
#define EP3_OUT 0x03



bool write_bulk(struct libusb_device_handle *handle, 
                unsigned char endpoint,
                unsigned char *buffer,
                int length)
{
  int err = 0;
  int transferred = 0;

  err = libusb_bulk_transfer(handle, 
                             endpoint, 
                             buffer, 
                             length, 
                             &transferred, 
                             2000);

  if(err == 0 && transferred == length)
    return true;
  
  return false;
}


bool read_bulk(struct libusb_device_handle *handle,
               unsigned char endpoint,
               unsigned char *buffer,
               int length)
{
  int err = 0;
  int received = 0;
  
  err = libusb_bulk_transfer(handle, 
                             endpoint, 
                             buffer, 
                             length, 
                             &received, 
                             2000);
  if(err == 0 && received == length)
    return true;
  
  return false;
}


bool claim_interface(struct libusb_device_handle *handle,
                     uint8_t interface)
{

  if(libusb_kernel_driver_active(handle, interface) == 1)
  {
    if(libusb_detach_kernel_driver(handle, interface) != 0)
      printf("Couldn't detach kernel driver!\n");
  }

  // claim util succ.
  while(libusb_claim_interface(handle, interface) < 0);

  return true;
}

// old wch-link dap mode has pid == 0x8011,
// It not supported by this software switch method.
// MounRiver studio switch modes of old wch-link by flashing firmware.

bool is_dap_mode(uint32_t vid, uint32_t pid)
{
  if(vid == 0x1a86 && pid == 0x8012)
    return true;
  return false;
}

bool is_rv_mode(uint32_t vid, uint32_t pid)
{
  if(vid == 0x1a86 && pid == 0x8010)
    return true;
  return false;
}


int main(void)
{
  struct libusb_context *ctx; 
  struct libusb_device **devs;
  struct libusb_device_handle *handle = NULL;
  struct libusb_device *dev;
  
  struct libusb_device_descriptor desc;
  
  bool found = 0;
  bool dapmode = 0;
  
  int err = 0;
  int ret = 0;
  uint32_t count;
  int config;
  
  // Init libusb 
  ret = libusb_init(&ctx);
  if(ret < 0)
  {
    printf("Failed to initialise libusb!\n");
    return -1;
  }
  
  // Get a list of USB devices
  count = libusb_get_device_list(ctx, &devs);
  if (count < 0)
  {
    printf("No USB devices on bus\n");
    return -1;
  }
  
  int i = 0;
  //iterate usb device list
  while ((dev = devs[i++]) != NULL)
  {
    ret = libusb_get_device_descriptor(dev, &desc);
    if (ret < 0) {
      printf("Failed to get device descriptor.\n");
      continue; // do not break;
    }
  
    // found wch-link or E
    if(desc.idVendor == 0x1a86) {
      found = 1;
      break;
    }
  }
  
  if(found == 0) {
    printf("WCH Link or E NOT found.\n");
    libusb_free_device_list(devs,1);
    return -1;
  }
  
  if(is_dap_mode(desc.idVendor, desc.idProduct)) {
    printf("Found WCH Link or E in DAP Mode.\n");
    dapmode = 1; 
  } else if(is_rv_mode(desc.idVendor, desc.idProduct)) {
    printf("Found WCH Link or E in RV Mode.\n");
    dapmode = 0; 
  }
  
  err = libusb_open(dev, &handle);
  if (err < 0)
  {
    printf("Error opening device!\n");
    printf("Do you have permissions to read/write the device?\n");
    goto end;
  }
  
  err = libusb_get_configuration(handle, &config);
  if(err!=0)
  {
    printf("Error in libusb_get_configuration\n");
    goto end;
  }
   
  if(config != 1 && libusb_set_configuration(handle, 1) != 0)
  {
    printf("Error in libusb_set_configuration\n");
    goto end;
  }
  
  if(!claim_interface(handle, 0) ||
     !claim_interface(handle, 1) ||
     !claim_interface(handle, 2))
  {
    printf("Failed to claim usb interface.\n");
    goto end;
  }
  
  //rv to dap
  int EP_OUT = RV_MODE_EP_OUT;
  int EP_IN = RV_MODE_EP_IN;
  
  //dap to rv
  if(dapmode) {
    EP_OUT = DAP_MODE_EP_OUT;
    EP_IN = DAP_MODE_EP_IN;
  }
  
  
  char *cmd = (char *)malloc(4 + 1);
  char *buffer = (char *)malloc(7 + 1);
  memset(cmd, '\0', 4 + 1);
  memset(buffer, '\0', 7 + 1);
  
  // seems not neccesary 
  if(!dapmode) {
    strcpy(cmd,"\x81\x0d\x01\xff");
  
    if(!write_bulk(handle, EP_OUT, cmd, 4)) {
      printf("Write error!\n");
      goto end;
    }
  
    if(!read_bulk(handle, EP_IN, buffer, 4)) {
      printf("Read error!\n");
      goto end;
    }
     
    // wch-linke reply
    if(memcmp( buffer, "\x82\x0d\x01\xff", 4 ) != 0) {
      printf("Device reply error!");
      goto end;
    }
  }
    
  
  // switch 
  strcpy(cmd,"\x81\x0d\x01\x01");

  if(!write_bulk(handle, EP_OUT, cmd, 4)) 
  {
    printf("Switch command write error!\n");
    goto end;
  }
  
  if(read_bulk(handle, EP_IN, buffer, 7))
  {
    char major_ver = buffer[3];
    char minor_ver = buffer[4];
    char mcu = buffer[5];
    char mode = buffer[6];
    if(!(memcmp( buffer, "\x82\x0d\x04", 3 ) == 0 &&
         (mcu == 0x02 || mcu == 0x12) &&
         major_ver == 0x02 &&
         minor_ver >= 0x08 &&
         (mode == 0x00 || mode == 0x01))) {
      printf("Device reply error!\nIt's not wch-linke?");
      goto end;
    }
  } else {
    printf("Switch command read error!\n");
    goto end;
  }
    
  //reset
  if(!dapmode)
    strcpy(cmd, "\x81\xff\x01\x41");
  else
    strcpy(cmd,"\x81\xff\x01\x52");
    
  if(!write_bulk(handle, EP_OUT, cmd, 4)) {
    printf("Reset command write error!\n");
    goto end;
  }
  
  if(!dapmode)
    printf("Switch to DAP Mode.\n");
  else
    printf("Switch to RV Mode.\n");
    
end: 
  libusb_release_interface(handle, 0);
  libusb_release_interface(handle, 1);
  libusb_release_interface(handle, 2);
 
  libusb_free_device_list(devs,1);
  libusb_close(handle);
  libusb_exit(NULL);
  
  return 0;
}

