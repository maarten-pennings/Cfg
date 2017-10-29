# Cfg
Library for configuring an ESP8266 app persistently.
It requires the [Nvm](https://github.com/maarten-pennings/Nvm) library.

## Introduction
Cfg is a library that adds a list of fields (key-value pairs) to an 
application, and it implements persistent storage and editing of 
these fields.

As a concrete example, think of an app that gets the time from some server and then displays is; a "web synced clock".
You write the program, flash it to an ESP8266, add a display and a battery, and you 3D print a nice case to package the whole stuff.
Then you give it as a present to a friend.

Your friend needs a way to configure her SSID, her password, and maybe even the URL of the time server (or the local time offset).
This is where this Cfg library helps.

Some details 
- Cfg stores fields persistently in EEPROM.  
  Example of fields: ssid, password, server url.
- The clock app you write can retrieve these values.  
  For example `WiFi.begin(cfg.getval("ssid"), cfg.getval("password") )`.
- When the Cfg library is correctly integrated (added to `setup` and `loop`), 
  it does not alter the normal flow of your application. 
  There is one exception: when your application starts, 
  Cfg will flash a LED a couple of seconds, and the user can press a button during that period.
  When the button is pressed at startup, Cfg is said th be in _config mode_.
  If that happens, Cfg will start a WiFi access point, start a webserver, and server a page that allows any browser to edit the fields.
- "Correct integration" means (1) Cfg should be at the head of your `setup()`. For example
  ```
  void setup() {
    // Some init before Cfg checking for the button is allowed
    Serial.begin(115200);
    Serial.printf("\n\n\nWelcome\n\n");

    // On boot: check if config button is pressed
    cfg.check(100,BUT_PIN); // Wait 100 flashes (of 50ms) for a change on pin BUT_PIN
    // If in config mode (user pressed button), do the setup of the Cfg library.
    if( cfg.cfgmode() ) { cfg.setup(); return; }
    
    // Do setup of the app
    ...
  }
  ```
- "Correct integration" means (2) Cfg should be at the head of your `loop()`. For example
  ```
  void loop() {
    // If in config mode (user pressed button), do the config of the Cfg library.
    if( cfg.cfgmode() ) { cfg.loop(); return; }

    // Do loop of the app
    ...
  }
  ```


## Details
See [Cfg.h](src/Cfg.h) for details.
There are [examples](examples)
- The application [cfgtime](examples/cfgtime) has ssid, password, a webserver and a refresh time as configuration parameters.
  On startup, it connects the the configured ssid/password.
  Then, every refresh time, it gets the timestamp of the index page of the webserver, and prints that time on the console.
- The application [cfgmsg](examples/cfgtime) has ssid, password as configuration parameters.
  On startup, it connects the the configured ssid/password.
  Then, it starts a webserver. Each request (URL) is printed on the console.
  

