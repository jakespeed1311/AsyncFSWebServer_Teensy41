/****************************************************************************************************************************
  defines.h
  
  Dead simple AsyncFSWebServer for Teensy41 QNEthernet
  
  For Teensy41 with QNEthernet using Teensy FS (SD, PSRAM, SQI/QSPI Flash, etc.)
   
  AsyncFSWebServer_Teensy41 is a library for the Teensy41 with QNEthernet
  
  Based on and modified from ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncFSWebServer_Teensy41
  Licensed under GPLv3 license
 *****************************************************************************************************************************/

#ifndef defines_h
#define defines_h

#define TEENSY41_DEBUG_PORT                 Serial

// Debug Level from 0 to 4
#define _FTP_SERVER_LOGLEVEL_               2

#if !( defined(CORE_TEENSY) && defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41) )
  #error Only Teensy 4.1 supported
#endif

// Debug Level from 0 to 4
#define _TEENSY41_ASYNC_TCP_LOGLEVEL_       0
#define _AWS_TEENSY41_LOGLEVEL_            0


 #include "QNEthernet.h"       // https://github.com/ssilverman/QNEthernet
  using namespace qindesign::network;
  //#warning Using QNEthernet lib for Teensy 4.1. Must also use Teensy Packages Patch or error
  #define SHIELD_TYPE           "QNEthernet"
 
#if (_AWS_TEENSY41_LOGLEVEL_ > 3)
  #warning Using QNEthernet lib for Teensy 4.1. Must also use Teensy Packages Patch or error
#endif

//#define USING_DHCP            true
#define USING_DHCP            false

#if !USING_DHCP
  // Set the static IP address to use if the DHCP fails to assign
    // Set the static IP address to use if the DHCP fails to assign
    IPAddress myIP(192, 168, 33, 115);
    IPAddress myNetmask(255, 255, 255, 0);
    IPAddress myGW(192, 168, 33, 1);
    IPAddress mydnsServer(192, 168, 33, 1);
#endif



#include <AsyncFSWebServer_Teensy41.h>

#endif    //defines_h
