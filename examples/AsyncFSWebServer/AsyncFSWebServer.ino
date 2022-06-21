/****************************************************************************************************************************
  AsyncFSWebServer.ino
  
  Dead simple AsyncFSWebServer for Teensy41 QNEthernet
  
  For Teensy41 with QNEthernet using Teensy FS (SD, PSRAM, SQI/QSPI Flash, etc.)
   
  AsyncFSWebServer_Teensy41 is a library for the Teensy41 with QNEthernet
  
  Based on and modified from ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncFSWebServer_Teensy41
  Licensed under GPLv3 license
 *****************************************************************************************************************************/

#define destLFS 1
#define destSD 2



//#define Enable_MTP

#include "defines.h"

#include <LittleFS.h>

#include <AsyncFSEditor_Teensy41.h>



#include <SD.h>
#include <SPI.h>

//#define FTP_FILESYST    FTP_SDFAT2

// Default 2048
#define FTP_BUF_SIZE    8192

//#include <FTP_Server_Teensy41.h>

// Object for FtpServer
//  Command port and Data port in passive mode can be defined here

//FtpServer ftpSrv; // Default command port is 21 ( !! without parenthesis !! )

File FTPmyFile;


const int chipSelect = BUILTIN_SDCARD;    //10;

SDClass myfs;

// set up variables using the SD utility library functions:
Sd2Card   card;
SdVolume  volume;
SdFile    root;

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

#define FTP_ACCOUNT       "teensy4x"
#define FTP_PASSWORD      "ftp_test"













#if defined(Enable_MTP)
// #include "MTP.h"
#include <MTP_Teensy.h>
/*
#if USE_EVENTS==1
  extern "C" int usb_init_events(void);
#else
  int usb_init_events(void) {}
#endif




  #if defined(__IMXRT1062__)
    // following only as long usb_mtp is not included in cores
    #if !__has_include("usb_mtp.h")
      #include "usb1_mtp.h"
    #endif
  #else
    #ifndef BUILTIN_SDCARD 
      #define BUILTIN_SDCARD 254
    #endif
    void usb_mtp_configure(void) {}
  #endif
*/
#endif


  String   webpage;


// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %ld :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char*)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";

    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char) data[i];
        }
      }
      else
      {
        char buff[3];

        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }

      Serial.printf("%s\n", msg.c_str());

      if (info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");

        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
        {
          msg += (char) data[i];
        }
      }
      else
      {
        char buff[3];

        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }

      Serial.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);

        if (info->final)
        {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");

          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

const char * hostName = "Teensy41-Async_MTP";
const char* http_username = "admin";
const char* http_password = "admin";

#define USING_PSRAM                 false
#define USING_SPI_FLASH             false
#define USING_SPI_FRAM              false
#define USING_PROGRAM_FLASH         true
#define USING_QSPI_FLASH            false
#define USING_SPI_NAND              false
#define USING_QPI_NAND              false

#if USING_PSRAM

  #define MYPSRAM 8         // compile time PSRAM size
  #define MYBLKSIZE 2048    // 2048
  
  EXTMEM char buf[MYPSRAM * 1024 * 1024];  // USE DMAMEM for more memory than ITCM allows - or remove
  
  char szDiskMem[] = "RAM_DISK";

  LittleFS_RAM LFmyfs;
  
  #warning USING_PSRAM
  
#elif USING_SPI_FLASH
  #define chipSelect        6
  
  LittleFS_SPIFlash LFmyfs;
  
  char szDiskMem[] = "SPI_DISK";

  #warning USING_SPI_FLASH

#elif USING_SPI_FRAM

  LittleFS_SPIFram LFmyfs;
  
  char szDiskMem[] = "FRAM_DISK";
  
  #warning USING_SPI_FRAM

#elif USING_PROGRAM_FLASH   //#********************************

 // const char *lfs_progm_str="PROGM";     // edit to reflect your configuration
//  const int lfs_progm_size = 2'000'000; // edit to reflect your configuration
 // const int nfs_progm = sizeof(lfs_progm_str)/sizeof(const char *);

 // LittleFS_Program progmfs[nfs_progm]; 

  LittleFS_Program LFmyfs;
  
  char szDiskMem[] = "PRO_DISK";
  
  #warning USING_PROGRAM_FLASH

#elif USING_QSPI_FLASH  //##********************

  LittleFS_QSPIFlash LFmyfs;
  
  char szDiskMem[] = "QSPI_DISK";
  
  #warning USING_QSPI_FLASH

#elif USING_SPI_NAND

  LittleFS_SPINAND LFmyfs;
  
  char szDiskMem[] = "SPI_NAND";
  
  #warning USING_SPI_NAND

#elif USING_QPI_NAND

  LittleFS_QPINAND LFmyfs;
  
  char szDiskMem[] = "QSPI_NAND";
  
  #warning USING_QPI_NAND

#endif

// Set for SPI usage
//const int FlashChipSelect = 10; // AUDIO BOARD
//const int FlashChipSelect = 4; // PJRC Mem board 64MB on #5, #6 : NAND 1Gb on #3, 2GB on #4
//const int FlashChipSelect = 5; // PJRC Mem board 64MB on #5, #6 : NAND 1Gb on #3, 2GB on #4
//const int FlashChipSelect = 6; // digital pin for flash chip CS pin

#if defined(Enable_MTP)
 MTPStorage_SD storage;
 MTPD    mtpd(&storage);

#endif

void initFS()
{
  Serial.print("Initializing LittleFS ...");

  // see if the Flash is present and can be initialized:
  // Note:  SPI is default so if you are using SPI and not SPI for instance
  //        you can just specify myfs.begin(chipSelect).
#if USING_PSRAM
  if (!LFmyfs.begin(buf, sizeof(buf)))
#elif USING_SPI_FLASH
  if (!LFmyfs.begin(chipSelect, SPI))
#elif USING_SPI_FRAM
  if (!LFmyfs.begin( FlashChipSelect ))
#elif USING_QSPI_FLASH
  if (!LFmyfs.begin())
#elif USING_SPI_NAND
  if (!LFmyfs.begin( FlashChipSelect ))
  
#elif USING_PROGRAM_FLASH

  /*
  
      if(!LFmyfs.begin(lfs_progm_size)) 
      {
        Serial.printf("Program Storage %d %s failed or missing",lfs_progm_str); Serial.println();
      }
      else
      {
#if defined(Enable_MTP)
  mtpd.begin();
        storage.addFilesystem(LFmyfs, lfs_progm_str);
#endif

        uint64_t totalSize = LFmyfs.totalSize();
        uint64_t usedSize  = LFmyfs.usedSize();
        Serial.printf("Program Storage %s ",lfs_progm_str); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
      }
  */


 if (!LFmyfs.begin(1024 * 1024 * 5))
#else
  if (!LFmyfs.begin())

#endif

  {
    Serial.printf("Error starting FS");

 //  while (1)
  //  {
      // Error, so don't do anything more - stay stuck here
 //   }
  }

  LFmyfs.quickFormat();  // performs a quick format of the created di
  Serial.println("\nFiles erased !");
  
      Serial.println("LittleFS initialized.");
}




void printSpaces(int num) {
    for (int i = 0; i < num; i++) {
        Serial.print(" ");
    }
}


void cardInit()
{
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect))
  {
    Serial.println("Initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  }
  else
  {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.print("\nCard type: ");
  
  switch (card.type())
  {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card))
  {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    
    return;
  }

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;

  Serial.print("\nVolume type is FAT"); Serial.println(volume.fatType(), DEC); Serial.println();

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters

  if (volumesize < 8388608ul)
  {
    Serial.print("Volume size (bytes): ");
    Serial.println(volumesize * 512);        // SD card blocks are always 512 bytes
  }

  Serial.print("Volume size (Kbytes): "); volumesize /= 2;    Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): "); volumesize /= 1024; Serial.println(volumesize);
}



void printDirectory(File dir, int numSpaces)
{
  while (true)
  {
    File entry = dir.openNextFile();

    if (! entry)
    {
      Serial.println("** no more files **");
      break;
    }

    printSpaces(numSpaces);
    Serial.print(entry.name());

    if (entry.isDirectory())
    {
      Serial.println("/");
      printDirectory(entry, numSpaces + 2);
    }
    else
    {
      // files have sizes, directories do not
      unsigned int n = log10(entry.size());

      if (n > 10)
        n = 10;

      printSpaces(50 - numSpaces - strlen(entry.name()) - n);
      Serial.print("  "); Serial.print(entry.size(), DEC);
      
      DateTimeFields datetime;

      if (entry.getModifyTime(datetime))
      {
        printSpaces(4);
        printTime(datetime);
      }

      Serial.println();
    }
    
    entry.close();
  }
}

void printDirectory1(FS& fs) {
    Serial.println("Directory\n---------");
    printDirectory(fs.open("/"), 0);
    Serial.println();
}


void printTime(const DateTimeFields tm)
{
  const char *months[12] =
  {
      "January",  "February", "March",      "April",    "May",        "June",
      "July",     "August",   "September",  "October",  "November",   "December"
  };

  if (tm.hour < 10)
    Serial.print('0');

  Serial.print(tm.hour);
  Serial.print(':');

  if (tm.min < 10)
    Serial.print('0');

  Serial.print(tm.min);
  Serial.print("  ");
  Serial.print(months[tm.mon]);
  Serial.print(" ");
  Serial.print(tm.mday);
  Serial.print(", ");
  Serial.print(tm.year + 1900);
}



void SDCardTest()
{
  Serial.println("\n===============================");
  Serial.print("SDCard Initialization : ");

  
  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD failed!");
    return;
  }

  Serial.println("SD done.");

  File root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("SD done!");
}


//-------------------------------------------------------
void setup()
{
    delay(1500);
    Serial.begin(115200);
    
  while (!Serial && millis() < 5000)
  {
      Serial.println(".");
  }


  Serial.print("\nStart AsyncFSBrowser on "); Serial.print(BOARD_NAME);
  delay(600);




  Serial.print(" with "); Serial.println(SHIELD_TYPE);
  Serial.println(ASYNC_FSWEBSERVER_TEENSY41_VERSION);
  delay(2000);

#if USING_DHCP
  // Start the Ethernet connection, using DHCP
  Serial.print("Initialize Ethernet using DHCP => ");
  Ethernet.begin();
#else
  // Start the Ethernet connection, using static IP
  Serial.print("Initialize Ethernet using static IP => ");
  Ethernet.begin(myIP, myNetmask, myGW);
  Ethernet.setDNSServerIP(mydnsServer);

  // Initialize the FTP server
 // ftpSrv.init();
 // ftpSrv.credentials(FTP_ACCOUNT, FTP_PASSWORD);

  Serial.print("FTP Server Credentials => account = "); Serial.print(FTP_ACCOUNT);
  Serial.print(", password = "); Serial.println(FTP_PASSWORD);

#endif

  if (!Ethernet.waitForLocalIP(5000))
  {
    Serial.println("Failed to configure Ethernet");

    if (!Ethernet.linkStatus())
    {
      Serial.println("Ethernet cable is not connected.");
    }

    // Stay here forever
    while (true)
    {
      delay(1);
    }
  }

  if (!Ethernet.waitForLink(5000))
  {
    Serial.println(F("Failed to wait for Link"));
  }
  else
  {
    Serial.print("IP Address = ");
    Serial.println(Ethernet.localIP());
  }
delay(2000);
  initFS();


  Serial.print("\n Space Used = ");
  Serial.println(LFmyfs.usedSize());
  Serial.print("Filesystem Size = ");
  Serial.println(LFmyfs.totalSize());

  printDirectory1(LFmyfs);

  Serial.println("########################");

  Serial.println("LittleFS finisched.");

#if (defined(ARDUINO_TEENSY41))

  Serial.println("Initializing SD card...");

  //////////////////////////////////////////////////////////////////

  cardInit();

  ////////////////////////////////////////////////////////////////

  SDCardTest();

  //////////////////////////////////////////////////////////////////
#endif



  //Serial.println("MTP_test 2");
  /*
  #if USE_EVENTS==1
    usb_init_events();
  #endif

  #if !__has_include("usb_mtp.h")
    usb_mtp_configure();
  #endif
  //storage_configure();

  */

  delay(3000);
  
  // xferSD ( destSD ); // do MediaTransfer LFS TO SD
  xferSD(destLFS); // do MediaTransfer SD TO LFS
  Serial.println("");
  Serial.println("################################################################################");
  Serial.println("Starting Dir on LittleFs");


  printDirectory1(LFmyfs);


//###



  server.serveStatic("/", &LFmyfs, "/").setDefaultFile("index.htm");
  server.serveStatic("/index.htm", &LFmyfs, "index.htm").setDefaultFile("index.htm");
  server.serveStatic("/favicon", &LFmyfs, "favicon.ico");
  server.serveStatic("/icon", &LFmyfs, "favicon.ico");
  //server.serveStatic("icon", &LFmyfs, "/favicon.ico");

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest* request) {
      Serial.println("Home Page...");
      Home(); // Build webpage ready for display
      request->send(200, "text/html", webpage);
      });



  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient * client)
  {
    client->send("hello!", NULL, millis(), 1000);
  });

  server.addHandler(&events);

  server.addHandler(new AsyncFSEditor(&LFmyfs, http_username, http_password));

 
  server.onNotFound([](AsyncWebServerRequest * request) 
  {
    Serial.printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      Serial.printf("GET");
    else if (request->method() == HTTP_POST)
      Serial.printf("POST");
    else if (request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if (request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if (request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
      
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength()) 
    {
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    
    for (i = 0; i < headers; i++) 
    {
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();

    for (i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);

      if (p->isFile())
      {
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      }
      else if (p->isPost())
      {
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
      else
      {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
  {
    if (!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());

    Serial.printf("%s", (const char*)data);

    if (final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
  });

  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total)
  {
    if (!index)
      Serial.printf("BodyStart: %u\n", total);

    Serial.printf("%s", (const char*)data);

    if (index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });


  server.begin();
}

//--------------------------------------
void loop()
{
 //  ftpSrv.service();

    
    #if defined(Enable_MTP)
      ws.cleanupClients();



      mtpd.loop();
    /*
      #if USE_EVENTS==1
        if(Serial.available())
        {
          char ch=Serial.read();
          Serial.println(ch);
          if(ch=='r') 
          {
            Serial.println("Reset");
            mtpd.send_DeviceResetEvent();
          }
        }
      #endif
      */
    #endif
}


void Home() {
    webpage = HTML_Header();
    webpage += "<h1>Home Page</h1>";
    webpage += "<h2>ESP Asychronous WebServer Example</h2>";
    webpage += "<img src = 'icon' alt='icon'>";
    webpage += "<h3>File Management - Directory, Upload, Download, Stream and Delete File Examples</h3>";
    webpage += HTML_Footer();
}
//#############################################################################################
void Page_Not_Found() {
    webpage = HTML_Header();
    webpage += "<div class='notfound'>";
    webpage += "<h1>Sorry</h1>";
    webpage += "<p>Error 404 - Page Not Found</p>";
    webpage += "</div><div class='left'>";
    webpage += "<p>The page you were looking for was not found, it may have been moved or is currently unavailable.</p>";
    webpage += "<p>Please check the address is spelt correctly and try again.</p>";
    webpage += "<p>Or click <b><a href='/'>[Here]</a></b> for the home page.</p></div>";
    webpage += HTML_Footer();
}

//#############################################################################################
String HTML_Header() {
    String page;
    page = "<!DOCTYPE html>";
    page += "<html lang = 'en'>";
    page += "<head>";
    page += "<title>Web Server</title>";
    page += "<meta charset='UTF-8'>"; // Needed if you want to display special characters like the � symbol
    page += "<style>";
    page += "body {width:75em;margin-left:auto;margin-right:auto;font-family:Arial,Helvetica,sans-serif;font-size:16px;color:blue;background-color:#e1e1ff;text-align:center;}";
    page += "footer {padding:0.08em;background-color:cyan;font-size:1.1em;}";
    page += "table {font-family:arial,sans-serif;border-collapse:collapse;width:70%;}"; // 70% of 75em!
    page += "table.center {margin-left:auto;margin-right:auto;}";
    page += "td, th {border:1px solid #dddddd;text-align:left;padding:8px;}";
    page += "tr:nth-child(even) {background-color:#dddddd;}";
    page += "h4 {color:slateblue;font:0.8em;text-align:left;font-style:oblique;text-align:center;}";
    page += ".center {margin-left:auto;margin-right:auto;}";
    page += ".topnav {overflow: hidden;background-color:lightcyan;}";
    page += ".topnav a {float:left;color:blue;text-align:center;padding:0.6em 0.6em;text-decoration:none;font-size:1.3em;}";
    page += ".topnav a:hover {background-color:deepskyblue;color:white;}";
    page += ".topnav a.active {background-color:lightblue;color:blue;}";
    page += ".notfound {padding:0.8em;text-align:center;font-size:1.5em;}";
    page += ".left {text-align:left;}";
    page += ".medium {font-size:1.4em;padding:0;margin:0}";
    page += ".ps {font-size:0.7em;padding:0;margin:0}";
    page += ".sp {background-color:silver;white-space:nowrap;width:2%;}";
    page += "</style>";
    page += "</head>";
    page += "<body>";
    page += "<div class = 'topnav'>";
    page += "<a href='/'>Home</a>";
    page += "<a href='/dir'>Directory</a>";
    page += "<a href='/upload'>Upload</a> ";
    page += "<a href='/download'>Download</a>";
    // page += "<a href='/stream'>Stream</a>";
    page += "<a href='/delete'>Delete</a>";
    page += "<a href='/rename'>Rename</a>";
    page += "<a href='/system'>Status</a>";
    //page += "<a href='/format'>Format FS</a>";
    page += "<a href='/newpage'>New Page</a>";
    page += "<a href='/logout'>[Log-out]</a>";
    page += "</div>";
    return page;
}
//#############################################################################################
String HTML_Footer() {
    String page;
    page += "<br><br><footer>";
    page += "<p class='medium'>Teensy Asynchronous WebServer Example</p>";
    page += "<p class='ps'><i>Copyright &copy;&nbsp;D L Bird 2021 Version ";
    page += "</footer>";
    page += "</body>";
    page += "</html>";
    return page;
}



void xferSD(int copyType) { // do MediaTransfer with SD

    static bool initSD = true;
    Serial.print("COPY Initializing SD card...");
 /*   if (initSD && !SD.begin(BUILTIN_SDCARD)) { // see if the card is present and can be initialized:
        Serial.println("\n\n SD Card failed, or not present - Cannot do Xfer\n");
    }
    else {
        */
        initSD = false;
        Serial.println("Copy card initialized.\n\n");
        if (copyType == destSD) {
            // char szSDdir[] = "/"; // COPY to SD ROOT
            char szSDdir[] = "LFS_CPY/"; // COPY to SD subdirectory
            if ('/' != szSDdir[0])
                SD.mkdir(szSDdir);
          //  makeRootDirsTest(); // BUGBUG DEBUG make extra subdirs and files to show function
            Serial.println("\n STARTING :: LittleFS copy to SD card XFER ...\n\n");
            mediaTransfer(myfs.open("/"), szSDdir, destSD); // TOO SD
            Serial.println("\n LittleFS copy to SD card XFER COMPLETE.\n\n");
        }
        else {  //Copy to littlefs
            char szLFSdir[] = "/";
            char szSDdir[] = "/";
            Serial.println("\n STARTING :: SD card copy to LittleFS XFER ...\n\n");
            mediaTransfer(SD.open(szSDdir), szLFSdir, destLFS);	// FROM SD
            Serial.println("\n SD card copy to LittleFS XFER COMPLETE.\n\n");
        }
 //   }
}

void mediaTransfer(File dir, char* szDir, int destMedia) {
    char szNewDir[36];
    while (true) {
        File entry = dir.openNextFile();

        if (!entry) {
            Serial.println("sd leer      ");
            break;
        }
        if (entry.isDirectory()) {
            strcpy(szNewDir, szDir);
            if (destMedia == destLFS)
                LFmyfs.mkdir(szNewDir);
            else
                SD.mkdir(szNewDir);
            strcat(szNewDir, entry.name());
            if (destMedia == destLFS) LFmyfs.mkdir(szNewDir);
            else SD.mkdir(szNewDir);
            strcat(szNewDir, "/");
            mediaTransfer(entry, szNewDir, destMedia);
        }
        else {
          //  Serial.println(entry);
            uint64_t fileSize, sizeCnt = 0, xfSize = 1;
            char mm[512];
            strcpy(szNewDir, szDir);
            strcat(szNewDir, entry.name());
            File dataFile;
            if (destMedia == destLFS) {
                dataFile = LFmyfs.open(szNewDir, FILE_WRITE_BEGIN);
            }
            else {
                dataFile = SD.open(szNewDir, FILE_WRITE_BEGIN);
            }
            if (!dataFile)
                Serial.print("\td_FILE: NOT open\n");
            fileSize = entry.size();
            while (entry.available()) {
                if (fileSize < sizeCnt) break;
                if (fileSize - sizeCnt >= sizeof(mm)) xfSize = sizeof(mm);
                else xfSize = fileSize - sizeCnt;
                entry.read(&mm, xfSize);
                dataFile.write(&mm, xfSize);
                sizeCnt += xfSize;
            }
            if (fileSize != sizeCnt) {
                Serial.print("\n File Size Error:: ");
                Serial.println(entry.name());
            }
            dataFile.close();
        }
        entry.close();
    }
}
/*
void makeRootDirsTest() {
    char szDir[36];
    for (uint32_t ii = 1; ii <= NUMDIRS; ii++) {
        sprintf(szDir, "/%lu_dir", ii);
        myfs.mkdir(szDir);
        sprintf(szDir, "/%lu_dir/aFile.txt", ii); // BUGBUG DEBUG
        file3 = myfs.open(szDir, FILE_WRITE);
        file3.write(szDir, 12);
        file3.close();

        sprintf(szDir, "/%lu_dir/TEST", ii); // BUGBUG DEBUG
        myfs.mkdir(szDir); // BUGBUG DEBUG
        sprintf(szDir, "/%lu_dir/TEST/bFile.txt", ii); // BUGBUG DEBUG
        file3 = myfs.open(szDir, FILE_WRITE);
        file3.write(szDir, 12);
        file3.close();
    }
    filecount = printDirectoryFilecount(myfs.open("/"));  // Set base value of filecount for disk
}
*/
