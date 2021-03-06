# Cfg
Library for configuring an ESP8266 app persistently.
It requires the [Nvm](https://github.com/maarten-pennings/Nvm) library.

## Introduction
Cfg is a library that adds a list of fields (key-value pairs) to an 
application, and it implements persistent storage and editing of 
these fields.

As a concrete example, think of an app that gets the time from some http server and then displays it; a "web synced clock".
You write the program, flash it to an ESP8266, and add a display and battery.
You 3D print a nice case, and put the flashed ESP8266, display and battery in it.
Then you give it as a present to a friend.

Your friend needs a way to configure her SSID, her password, and maybe even the URL of her favorite http server (where to get the time from).
This _configuration_ is where the Cfg library helps.

## Some details 
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
  Instead it will start a WiFi access point, start a webserver, and serve out a page that allows any browser to edit the fields.
  After a reset, the normal program flow starts again.
- "Correct integration" means (1) Cfg should be at the head of `setup()`. For example
  ```cpp
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
  ```cpp
  void loop() {
    // If the config button was pressed, do the loop of the Cfg library (instead of the app's loop).
    if( cfg.cfgmode() ) { cfg.loop(); return; }

    // Do loop of the app
    ...
  }
  ```
- Of course you'll need an instance of the Cfg class
  ```cpp
    Cfg cfg( "CfgDemo", CfgFieldsDefault );
  ```
  The `CfgFieldsDefault` is an array of fields, this default one contains a field for the ssid and the password.  
  Your time application needs more, for example
  ```cpp
  NvmField CfgTimeFields[] = {
    {"ssid"    , "MySSID"       , 32, "The ssid of the wifi network this device should connect to." },
    {"password", "MyPassword"   , 32, "The password of the wifi network this device should connect to."},
    {"server"  , "www.google.nl", 32, "The webserver that will be asked for time (http head request)."},
    {"interval", "300"          , 10, "The time (in seconds) between webserver requests for time."},
    {0         , 0              ,  0, 0},  
  };
  ```
  
## Even more details
See [Cfg.h](src/Cfg.h) for detailed documentation


## Examples
There are [examples](examples)
- The application [cfgtime](examples/cfgtime) has ssid, password, a webserver and a refresh time as configuration parameters.  
  On startup, it connects to WiFi using the configured ssid/password.  
  Then, every refresh time, it gets the timestamp of the index page of the webserver, and prints that time on the console.
- The application [cfgmsg](examples/cfgmsg) has ssid and password as configuration parameters.  
  On startup, it connects to WiFi using the configured ssid/password.  
  Then, it starts a webserver. Each request (URL) send to the server is printed on the console.
- The application [cfgtest](examples/cfgtest) has a large set of configuration parameters.
  It illustrates how to more easily test a larger web page.
  And demo's web page markup features (headings, white lines, html features like bold/links)  .
  

## CfgTime in detail
When the CfgTime application is flashed and started it will have the following output over serial.
```
Welcome to CfgTime

Cfg: Press button on pin 0 to enter configuration mode
Connecting to MySSID .........................
```

Dots will keep om appearing, while the application tries to connect to `MySSID` (using `MyPassword`).
Since the SSID in my home is `GuestPennings` (using `no_password`), this will not succeed.
We will have to configure the application.

We press the "reset" button first

```
Welcome to CfgTime

Cfg: Press button on pin 0 to enter configuration mode
```

and then have about 3 seconds to press the "flash" button on the NodeMCU board.

```
Cfg: Entering configuration mode
Cfg: Join WiFi 'CfgTime-F1A251' (open)
Cfg: Then browse to any page (e.g. '10.10.10.10')
```

Next, open your smartphone or laptop, connect to access point `CfgTime-F1A251`, fire
up the browser and type any address. The CfgTime app will serve its configuration page.
Note that the browser tries to get a `favicon.ico`, and the operating system (?) tries
to reach the internet `connecttest.txt` (which fails through this access point).

```
Cfg: web: '/' (config)
Cfg: web: '/favicon.ico' not found
Cfg: web: '/connecttest.txt' not found
Cfg: web: '/connecttest.txt' not found
Cfg: web: '/connecttest.txt' not found
```

The browser shows the configuration page with defaults

![The configuration page with defaults](cfgtime1.png)

Fill out the correct SSID and password (the default server and poll interval are left as-is)

![Correct SSID and password filled out](cfgtime2.png)

and press save

![Saved the configuration](cfgtime3.png)

The serial port shows that after saving, the CfgTime app reboots

```
Cfg: web: '/connecttest.txt' not found
Cfg: web: '/connecttest.txt' not found
Cfg: web: '/save'
Cfg: Saved: 'ssid' = 'GuestPennings'
Cfg: Saved: 'password' = 'no_password'
Cfg: Saved: 'server' = 'www.google.nl'
Cfg: Saved: 'interval' = '300'
Cfg: Restart will now be invoked...
```

This time, we do not press the configuration button, and the app starts it time getting application.

```
Welcome to CfgTime

Cfg: Press button on pin 0 to enter configuration mode
Connecting to GuestPennings .......... connected
IP is 192.168.179.52
Time obtained from 'www.google.nl' every 300 seconds

Date: Tue, 26 Dec 2017 11:08:47 GMT
Date: Tue, 26 Dec 2017 11:13:47 GMT
Date: Tue, 26 Dec 2017 11:18:47 GMT
Date: Tue, 26 Dec 2017 11:23:47 GMT
```

(end of doc)
