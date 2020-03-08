/*
  Cfg.h - Library for configuring an ESP8266 app
  Created by Maarten Pennings 2017 April 16
*/
#ifndef _CFG_H_
#define _CFG_H_


/*
SYNOPSIS
Cfg is a library that adds a list of fields (key-value pairs) to an 
application, and it implements persistent storage and editing of 
these fields.


CONCEPTS
To store the fields persistently, Cfg uses the Nvm module which stores 
key-value pairs in the eeprom of the ESP8266. To edit the fields, Cfg
creates an access point, starts a webserver, and start a DNS server. The
DNS server routes any url to the webserver. The webserver shows a form
that allows the user to edit the fields. The webserver even supports
mobile devices.

The Cfg library act as a bootloader. Just after booting, it is in the 
"check" phase for a couple of seconds. During the check phase a LED is
blinking in high frequency. If no special action is taken by the user, 
the Cfg library stops after the check phase; it hands over control to 
the real application, making all persistent values available.

If, on the other hand, the user presses a button during the check phase,
the Cfg library starts its "configuration" phase. In this phase it starts
and access point and a webserver. As long as the configuration phase 
is active, the same LED is blinking at low frequency. The configuration 
phase is ended if the user visits the build-in webserver and saves
the configuration (after editing it). 

When the configuration phase ends the Cfg lib forces a device restart 
(software reset). Another way to end the configuration phase is to press 
the reset button on the board. When the device restarts, the Cfg library
will be in the check phase for a couple of seconds. Typically the user
will not press the button this time, so that the normal application will
run.


USING CFG
To use Cfg in an application, #include it, and create an instance of it.
Pass the instance the name of the application (there are other optional 
parameters, see below). Note that Cfg uses Nvm, so that module should also
be available.

Since, by default, Cfg writes progress to Serial, it is customary to 
open the Serial port in setup(). The next step is to check() for a couple
of seconds if "the" button is pressed. If so, the cfg object remembers that.
Next comes a fork: either run cfg.setup() when the user did press the 
button, or run the actual setup of the application.

Recall that when the configuration terminates, the device is restarted,
so setup() is ran again, this time the user does not press a button, so the
application's setup is run.

| #include "Cfg.h"
| Cfg cfg("CfgDemo");
| 
| void setup() {
|   // Open serial port
|   Serial.begin(115200);
|   Serial.printf("Welcome\n");
| 
|   // Check if config button is pressed, if so do config's setup (only)
|   cfg.check(); 
|   if( cfg.cfgmode() ) { cfg.setup(); return; }
| 
|   // Do normal setup
|   ...
| }
| 
| void loop() {
|   // If in config mode, do config loop (when config ends, device restarts)
|   if( cfg.cfgmode() ) { cfg.loop(); return; }
| 
|   // Do normal loop
|   ...
| }

The loop() uses a similar construct. When in config mode, run cfg.loop() 
otherwise run the applications loop().


DETAILS
When creating an instance of Cfg, there is one mandatory parameter 'appname',
the application's name. It is used to form an SSID of the access point, and 
to style the web page of the webserver.

| Cfg(const char *appname, NvmField*fields=CfgFieldsDefault, int seriallvl=CFG_SERIALLVL_USR, int ledpin=D4);

So possible calls are
| Cfg cfg("CfgDemo", CfgFieldsDefault, CFG_SERIALLVL_DBG, LED_PIN);
| Cfg cfg("CfgDemo", CfgFieldsDefault, CFG_SERIALLVL_DBG);
| Cfg cfg("CfgDemo", CfgFieldsDefault );
| Cfg cfg("CfgDemo");

But the constructor has more parameters, all of which have a default. The 
second parameter 'field' is an array describing the fields (key-value pairs), 
see section FIELDS below for details.

The third parameter 'seriallvl' determines how much the Cfg module prints to 
Serial, there are three flavors:
| #define CFG_SERIALLVL_NON  0 // Cfg will not print to Serial
| #define CFG_SERIALLVL_USR  1 // Cfg will only print user messages
| #define CFG_SERIALLVL_DBG  2 // Cfg will print debug/trace messages too

The fourth parameter 'ledpin' identifies the pin of the LED that gives 
feedback (high speed flashing during check, low speed flashing during 
configuration). By default pin D4 is used, this seems to be the usual pin 
for the blue led on the ESP8266 module. But any other pin can be specified, 
or a value <0 to not have a feedback LED. If a pin is specified, it is 
configured for output and toggled. 

The check() function has two optional parameters.
| check(int cfgwait=100, int butpin=D3);
The first parameter 'cfgwait' determines the duration of the check() phase.
By default it waits 100 flashes of the ledpin, each flash taking 50ms.
The second parameter 'but pin' is the pin that has a button, most boards 
have a button labeled "flash" connected to P4, so that was chosen as default. 
Note that the Cfg module configures this pin for input, senses its values 
and then waits 'cfgwait' to see if the value changes.


FIELDS
The most complex parameter is the 'fields' in the constructor. It has three
major uses. First of all Cfg passes the fields to Nvm, which creates a memory
layout for the eeprom (with offsets, sizes, checksums). Secondly, Cfg uses
fields to create a webpage with edit boxes for each field. Finally, Cfg 
offers the functions
| char * getval(const char * name);
| char * getval(int ix);
that allow the real application to query Cfg for the values of the fields.

The fields variable is an array, where each element is a field, the last
field containing zero's to signal end-of-array. Each field consists of four
values: the field name (a string), the default field value (also a string),
the maximum field length (integer), and a description (string).

The default fields defines just the credentials for an access point.
| NvmField CfgFieldsDefault[] = {
|   {"ssid"    , "MySSID"     , 32, "The ssid of the wifi network this device should connect to."    },
|   {"password", "MyPassword" , 32, "The password of the wifi network this device should connect to."},
|   {0         , 0            ,  0, 0},  
| };
Note that cfg.getval("password") would give the same result as cfg.getval(1),
since this fields definition has field "password" at index 1.

Note that Cfg only supports string fields. This is because the Nvm and text 
boxes on the web page are strings. This means that when there is a field like
| {"interval", "300"          , 10, "The time (in seconds) between webserver requests for time."},
one way to extract the integer is
| int interval = String(cfg.getval("interval")).toInt();

Also note that there is no input checking by Cfg, except that when the user 
enters too long a string, this is truncated to the length specified in the 
fields array.

From the fields, also the web page is generated. There are two "mark up" features.
A field with a length of 0 generates a header (from name) and a sub-header (from extra).
When the the extra field ends in a space, the web page has a white-line below the item.


REVISION HISTORY
 v1.6.0  20200308  Added undo next to reset
 v1.5.0  20200307  Added headings and tooltips to the web page
 v1.4.0  20170521  Added tot github; some small updates to comments
 v1.3.0  20170521  Fix: mac address in ssid, new websrv, more LOGUSR for websrv
 v1.2.0  20170504  Added CfgMsg example; small DBG fixes, new CRLL handling
 v1.1.0  20170427  Fix: urldecode on webvalues. New: default buttons on webpage. More efficient string handling.
 v1.0.0  20170424  Initial version
*/
 

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Nvm.h>


#define CFG_SERIALLVL_NON  0 // Cfg will not print to Serial
#define CFG_SERIALLVL_USR  1 // Cfg will only print user messages
#define CFG_SERIALLVL_DBG  2 // Cfg will print debug/trace messages too


extern NvmField CfgFieldsDefault[];


class Cfg
{
  public:
    Cfg(const char *appname, NvmField*fields=CfgFieldsDefault, int seriallvl=CFG_SERIALLVL_USR, int ledpin=D4);
    ~Cfg(void);
    void check(int cfgwait=100, int butpin=D3);
    bool cfgmode(void);
    void setup(void);
    void loop(void);
    char * getval(const char * name);
    char * getval(int ix);
  private:
    const char*      _appname;   // Application name, used on serial prints, ssid, webpage
    NvmField*        _fields;    // The fields that need to be configured
    int              _seriallvl; // Level of feedback over serial port (debug, trace)
    int              _ledpin;    // The id of the led that is used for feedback
    int              _cfg;       // Persistent recording whether user selected configuration mode
    int              _loop;      // Number of loop calls
    bool             _restart;   // Request to restart
    ESP8266WebServer*_websrv;    // Webserver for configuration mode
    DNSServer*       _dnssrv;    // DNS server
    Nvm*             _nvm_;      // Named strings in eeprom (use via _nvm() )
    char**           _vals;      // The values from the nvm (local cache), so that a safe char* can be returned from getval()
    void _handle_config(void);
    void _handle_save(void);
    void _handle_restart(void);
    void _handle_404(void);
    Nvm* _nvm(void);
};

#endif

