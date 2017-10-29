# Cfg
Library for configuring an ESP8266 app persistently.
It requires the [Nvm](https://github.com/maarten-pennings/Nvm) library.

## Introduction
Cfg is a library that adds a list of fields (key-value pairs) to an 
application, and it implements persistent storage and editing of 
these fields.

As a concrete example, think of an app that gets the time from some server and then displays is; a "web synced clock".
You write the program, flash it to an ESP8266, and add a display and a battery.
You 3D print a nice case, and put the flashed ESP8266 and display in it.
Then you give it as a present to a friend.

Your friend needs a way to configure her SSID, her password, and maybe even the URL of her favorite http server.
This _configuration_ is where the Cfg library helps.

Some details 
- Cfg stores fields persistently in EEPROM.  
  Example of fields: ssid, password, server url.
- The clock app you write can retrieve these values.  
  For example `WiFi.begin( cfg.getval("ssid"), cfg.getval("password") )`.
- When the Cfg library is correctly integrated (added to `setup` and `loop`), 
  it does not alter the normal flow of your application. 
  Your app configures Wifi (see above) in `setup`, and periodically polls the http server in `loop` to get and print the time.
- There is one exception to the normal flow: when your application starts, 
  Cfg will flash a LED a couple of seconds, and the user can press a button during that period.
  By default, it is the blue LED on the ESP8266 module, and the flash button on the NodeMCU board.
  When the button is pressed at startup, Cfg is said to be in _config mode_.
  If that happens, Cfg will prevent your app from running.
  Instead it will start a WiFi access point, start a webserver, and server a page that allows any browser to edit the fields.
  After a reset, normal flows starts again.
- "Correct integration" means (1) Cfg should be at the head of `setup()`. For example
  ```
  void setup() {
    // Some steps before Cfg will check for the button is allowed ...
    Serial.begin(115200);
    Serial.printf("\n\n\nWelcome\n\n");

    // Let Cfg check if the config button is pressed.
    cfg.check(100,BUT_PIN); // Wait 100 flashes (of 50ms) for a change on pin BUT_PIN.
    // If the config button was pressed, do the setup of the Cfg library (instead of the app's setup).
    if( cfg.cfgmode() ) { cfg.setup(); return; }
    
    // Do setup of the app
    ...
  }
  ```
- "Correct integration" means (2) Cfg should be at the head of `loop()`. For example
  ```
  void loop() {
    // If the config button was pressed, do the loop of the Cfg library (instead of the app's loop).
    if( cfg.cfgmode() ) { cfg.loop(); return; }

    // Do loop of the app
    ...
  }
  ```
- Of course you'll need an instance of the Cfg class
  ```
    Cfg cfg("CfgDemo", CfgFieldsDefault );
  ```
  The `CfgFieldsDefault` is an array of fields, this default one contains a field for the ssid and the password.  
  Your time application needs more, for example
  ```
  NvmField CfgTimeFields[] = {
    {"ssid"    , "MySSID"       , 32, "The ssid of the wifi network this device should connect to." },
    {"password", "MyPassword"   , 32, "The password of the wifi network this device should connect to."},
    {"server"  , "www.google.nl", 32, "The webserver that will be asked for time (http head request)."},
    {"interval", "300"          , 10, "The time (in seconds) between webserver requests for time."},
    {0         , 0              ,  0, 0},  
  };
  ```
  
  
## Details
See [Cfg.h](src/Cfg.h) for detailed dopcumentation

## Examples
There are [examples](examples)
- The application [cfgtime](examples/cfgtime) has ssid, password, a webserver and a refresh time as configuration parameters.  
  On startup, it connects the the configured ssid/password.  
  Then, every refresh time, it gets the timestamp of the index page of the webserver, and prints that time on the console.
- The application [cfgmsg](examples/cfgtime) has ssid, password as configuration parameters.  
  On startup, it connects the the configured ssid/password.  
  Then, it starts a webserver. Each request (URL) is printed on the console.
  

(end of doc)
