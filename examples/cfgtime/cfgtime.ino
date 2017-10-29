/*
  cfgtime.ino - Demo for Cfg module: a web client that gets the time periodically
  Created by Maarten Pennings 2017 April 24
*/
#include <Cfg.h>


/*
This app is a demo of the Cfg module. 
It is a "clock" printing time obtained from a web server to the serial port.

To get the demo running:
 - Identify the pins for a led and a button connected to your ESP8266.
 - If the pins differ from D4 and D3 edit the macros LED_PIN and BUT_PIN.
 - Compile and flash the demo to your ESP8266 board.
 - Hook a terminal to the serial port and reset the board (115200 baud).
 - Typically you will see the led flash fast for five seconds.
 - Then the terminal will read
   | Welcome to CfgTime
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
   | Welcome to CfgTime
   | 
   | Cfg: Press button on pin 0 to enter configuration mode
   | Cfg: Entering configuration mode
   | Cfg: Join WiFi 'CfgTime-BAD12C' (open)
   | Cfg: Then browse to any page (e.g. '10.10.10.10')
 - The led wil now flash much slower; the ESP8266 is now access point with web browser and DNS server;
   as long as the server is serving pages the led will falsh slowly.
 - In a PC or mobile phone, connect to new WiFi network (there is no password)
 - Enter any url e.g. cfg.com, the DNS server will route to the built-in webserver
   (do not enter a simple string like 'cfg', because the Chrome will search instead of doing DNS)
 - The configuration page will be displayed in the web browser.
 - Edit the 'ssid' and 'password' to match the available Access Point.
   Optionaly enter another 'server' or the poll 'interval'
 - Press 'Save'; the configuration will be saved to eeprom and the device will be restarted
   The Serial output will look like this, but with different ssid/password
   | Cfg: Web browser requests '/', response config page
   | Cfg: Web browser requests '/?ssid=MySSID&password=MyPassword&server=www.google.nl&interval=300', response submit page
   | Cfg: Saved: 'ssid' = 'MySSID'
   | Cfg: Saved: 'password' = 'MyPassword'
   | Cfg: Saved: 'server' = 'www.google.nl'
   | Cfg: Saved: 'interval' = '300'
   | Cfg: Restart will now be invoked...

The clock application is now configured (fields are saved persistently in eeprom).
 - After the boot, the led flashes fast again, this time do not press the button.
 - The clock app will start, connecting to the specified access point
 - It will ask the specified 'server' for the time and display that.
 - Every 300 seconds (or other value if 'interval' is changed) a new time is printed
   | Welcome to CfgTime
   | 
   | Cfg: Press button on pin 0 to enter configuration mode
   | Connecting to GuestFamPennings .... connected
   | IP is 192.168.179.73
   | Time obtained from 'www.google.nl' every 300 seconds
   | 
   | Date: Mon, 24 Apr 2017 20:34:04 GMT
   | Date: Mon, 24 Apr 2017 20:39:09 GMT
   | Date: Mon, 24 Apr 2017 20:44:12 GMT
*/


// Pin definitions
const int LED_PIN = D4;
const int BUT_PIN = D3;
#define led_on()  digitalWrite(LED_PIN, LOW)
#define led_off() digitalWrite(LED_PIN, HIGH)
#define led_tgl() digitalWrite(LED_PIN, (HIGH+LOW)-digitalRead(LED_PIN) );


// The fields used by this demo
NvmField CfgTimeFields[] = {
  {"ssid"    , "MySSID"       , 32, "The ssid of the wifi network this device should connect to." },
  {"password", "MyPassword"   , 32, "The password of the wifi network this device should connect to."},
  {"server"  , "www.google.nl", 32, "The webserver that will be asked for time (http head request)."},
  {"interval", "300"          , 10, "The time (in seconds) between webserver requests for time."},
  {0         , 0              ,  0, 0},  
};
Cfg cfg("CfgTime", CfgTimeFields, CFG_SERIALLVL_USR, LED_PIN);
int interval;


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\n\nWelcome to CfgTime\n\n");

  // On boot: check if config button is pressed
  cfg.check(100,BUT_PIN); // Wait 100 flashes (of 50ms) for a change on pin BUT_PIN
  // if in config mode, do config setup (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.setup(); return; }

  // Do normal setup
  Serial.printf("Connecting to %s ", cfg.getval("ssid") );
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.getval("ssid"), cfg.getval("password") );
  while( WiFi.status()!=WL_CONNECTED ) {
    delay(500);
    led_tgl();
    Serial.print(".");
  }
  led_off();
  Serial.printf(" connected\n");
  Serial.printf("IP is %s\n",WiFi.localIP().toString().c_str());
  interval = String(cfg.getval("interval")).toInt();
  Serial.printf("Time obtained from '%s' every %d seconds\n\n",cfg.getval("server"),interval);
}


void loop() {
  // if in config mode, do config loop (when config completes, it restarts the device)
  if( cfg.cfgmode() ) { cfg.loop(); return; }

  // Do normal loop
  String now = gettime(false);
  if( now!="" ) Serial.printf("%s\n",now.c_str()); else Serial.printf("Date: retrieve failed\n");
  delay( interval*1000 ); 
}


String gettime(bool print) {
  String result = "";
  led_on();
  WiFiClient client;
  if( print ) Serial.printf("Connecting to %s ... ", cfg.getval("server") );
  if( client.connect(cfg.getval("server"),80) ) {
    if( print ) Serial.printf("connected\n");
    if( print ) Serial.printf("  Sending request\n");
    client.print(String("HEAD / HTTP/1.1\r\n")+
                 "Host: "+cfg.getval("server")+"\r\n" +
                 "Connection: close\r\n" +
                 "\r\n"
                );
    if( print ) Serial.printf("  Response\n");
    int timeout = 0;
    while( client.connected() && timeout<10 ) {
      if( client.available() ) {
        timeout=0;
        String line = client.readStringUntil('\n');
        if( line.endsWith("\r") ) line.remove(line.length()-1);
        if( print ) Serial.printf("  | %s\n", line.c_str());
        if( line.startsWith("Date: ") ) { result= line; break; } // Now that we have the time, abort the connection
      } else {
        timeout++;
        delay(100);
      }
    }
    client.stop();
    if( print ) Serial.printf("Disconnected\n");
  } else {
    if( print ) Serial.printf("failed!\n");
    client.stop();
  }
  if( print ) Serial.printf("\n" );
  led_off();
  return result;
}

