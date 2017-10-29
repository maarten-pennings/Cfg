/*
  cfgmsg.ino - Demo for Cfg module: a web server that prints messages arriving via GET
  Created by Maarten Pennings 2017 May 3
*/
#include <Cfg.h>


/*
This app is a demo of the Cfg module. 
It is a message board; messages send via a GET to the webserver are printed to the Serial port.
A next step would be to print them to a LED matrix board, but that requires extra hardware.

To get the demo running:
 - Identify the pins for a led and a button connected to your ESP8266.
 - If the pins differ from D4 and D3 edit the macros LED_PIN and BUT_PIN.
 - Compile and flash the demo to your ESP8266 board.
 - Hook a terminal to the serial port and reset the board (115200 baud).
 - Typically you will see the led flash fast for five seconds.
 - Then the terminal will read
   | Welcome to CfgMsg
   | 
   | Cfg: Press button on pin 0 to enter configuration mode
   | Connecting to MySSID ................................................ 
 - See one line printed by the Cfg module, this was the call causing the flashing blue led.
 - Connecting likely fails because there is no access point named MySSID.
 
Let's configure (the ssid, password, etc)
 - Reset the board
 - The led will start flashing and the terminal will say "Cfg: Press button on pin 0 to enter configuration mode"
 - Press the button
 - Terminal will read
   | Welcome to CfgMsg
   | 
   | Cfg: Press button on pin 0 to enter configuration mode
   | Cfg: Entering configuration mode
   | Cfg: Join WiFi 'CfgMsg-BAD12C' (open)
   | Cfg: Then browse to any page (e.g. '10.10.10.10')
 - The led wil now flash much slower; the ESP8266 is now access point with web browser and DNS server;
   as long as the server is serving pages the led will falsh slowly.
 - In a PC or mobile phone, connect to new WiFi network (there is no password)
 - Enter any url e.g. cfg.com, the DNS server will route to the built-in webserver
   (do not enter a simple string like 'cfg', because the Chrome will search instead of doing DNS)
 - The configuration page will be displayed in the web browser.
 - Edit the 'ssid' and 'password' to match the available Access Point.
 - Press 'Save'; the configuration will be saved to eeprom and the device will be restarted
   The Serial output will look like this, but with different ssid/password
   | Cfg: Web browser requests '/', response config page
   | Cfg: Web browser requests '/?ssid=MySSID&password=MyPassword', response submit page
   | Cfg: Saved: 'ssid' = 'MySSID'
   | Cfg: Saved: 'password' = 'MyPassword'
   | Cfg: Restart will now be invoked...

The msg application is now configured (fields are saved persistently in eeprom).
 - After the boot, the led flashes fast again, this time do not press the button.
 - The clock app will start, connecting to the specified access point
 - It will wait till a message is pushed via a http GET
   type http://cfgmsg/Hello_World!
   or   http://192.168.179.73/Hello_World!
 - The serial log reads 
   Note that the second connect is for /favicon.ico, which is suppressed in the server
   | Welcome to CfgMsg
   | 
   | Cfg: Press button on pin 0 to enter configuration mode
   | Connecting to GuestFamPennings ...... connected
   | IP is 192.168.179.74
   | Http server listening on port 80
   | 
   | Web browser connected
   | url='/Hello_World!'
   | Disconnected
   | 
   | Web browser connected
   | Disconnected
*/


// Pin definitions
const int LED_PIN = D4;
const int BUT_PIN = D3;
#define led_on()  digitalWrite(LED_PIN, LOW)
#define led_off() digitalWrite(LED_PIN, HIGH)
#define led_tgl() digitalWrite(LED_PIN, (HIGH+LOW)-digitalRead(LED_PIN) );

Cfg cfg("CfgMsg", CfgFieldsDefault, CFG_SERIALLVL_USR, LED_PIN);
WiFiServer  httpsrv(80);

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\n\nWelcome to CfgMsg\n\n");

  // On boot: check if config button is pressed
  cfg.check(100,BUT_PIN); // Wait 100 flashes (of 50ms) for a change on pin BUT_PIN
  // if in config mode, do config setup (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.setup(); return; }

  // Do normal setup
  WiFi.hostname("CfgMsg"); // or wifi_station_set_hostname("CfgMsg"); // needs extern "C" { #include "user_interface.h" }
  Serial.printf("Connecting to %s ", cfg.getval("ssid") );
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.getval("ssid"), cfg.getval("password") );
  while( WiFi.status()!=WL_CONNECTED ) {
    led_tgl();
    delay(500);
    Serial.print(".");
  }
  led_off();
  Serial.printf(" connected\n");
  Serial.printf("IP is %s\n",WiFi.localIP().toString().c_str());

  // Start http server
  httpsrv.begin();
  Serial.printf("Http server listening on port 80\n");
}


void loop() {
  // if in config mode, do config loop (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.loop(); return; }

  // Do normal loop

  // Is there an http client
  WiFiClient client = httpsrv.available(); 
  if( client==0 ) { delay(100); return; }
  
  // Feedback 'connected'
  Serial.printf("\nWeb browser connected\n");

  // Get request
  String request = "";
  int timeout = 0;
  while( client.connected() && timeout<10 ) {
    // read line by line what the client (web browser) is requesting
    if( client.available() ) {
      timeout = 0;
      // Get a request line (standard says it ends in CRLF or \r\n, we also except \n only )
      String line = client.readStringUntil('\n');
      if( line.endsWith("\r") ) line.remove(line.length()-1);
      // Save very first line (it has the URL)
      if( request.equals("") ) request= line;
      // HTTP request is terminated with an empty line
      if( line.length()==0 ) {
        String url = request;
        int p1 = request.indexOf(' ');
        int p2 = request.lastIndexOf(' ');
        //Serial.printf("  Request '%s' at %d..%d\n",url.c_str(),p1+1,p2);
        if( p1!=-1 && p2!=-1 && p1+1<p2 ) {
          url = request.substring(p1+1,p2);
          if( url!="/favicon.ico" ) {
            Serial.printf("  Message '%s'\n", url.substring(1).c_str());
            client.print("HTTP/1.1 200 OK\r\n\r\nThx\r\n");
          } else {
            Serial.printf("  url='%s'\n", url.c_str());
            client.print("HTTP/1.1 404 Not Found\r\n\r\nFile not found\r\n");
          }
        } else {
            Serial.printf("  Error\n");
            client.print("HTTP/1.1 404 Not Found\r\n\r\n");
        }
        break;
      }
    } else {
      timeout++;
      //TRACE Serial.printf("n");
      delay(100); 
    }
  }
  delay(1); // Give the web browser time to receive the data, this is like yield()
  //TRACE Serial.printf("\n");
  // close the connection:
  client.stop();
  Serial.printf("Disconnected\n");
}



