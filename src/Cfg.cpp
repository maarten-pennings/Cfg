/*
  Cfg.cpp - Library for configuring an ESP8266 app
  Created by Maarten Pennings 2017 April 16
*/


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Cfg.h"


#define CFG_LOGPREFIX   "Cfg" // The start of all serial prints of Cfg
#define CFG_FLASH_SETUP   50  // The time between ledpin flashes in setup() - user to press button
#define CFG_FLASH_LOOP   999  // The time between ledpin flashes in loop() - user to connect with web browser


// Shorthand for prints to the serial port, taking _seriallvl into account
#define LOGUSRX(...) do if( _seriallvl>=CFG_SERIALLVL_USR ) Serial.printf(__VA_ARGS__); while(0)
#define LOGDBGX(...) do if( _seriallvl>=CFG_SERIALLVL_DBG ) Serial.printf(__VA_ARGS__); while(0)
#define LOGUSR(...)  LOGUSRX(CFG_LOGPREFIX ": " __VA_ARGS__)
#define LOGDBG(...)  LOGDBGX(CFG_LOGPREFIX ": " "DBG: " __VA_ARGS__)


// One default definition of fields (containing just an ssid and password of a wifi network)
NvmField CfgFieldsDefault[] = {
  {"ssid"    , "MySSID"     , 32, "The ssid of the wifi network this device should connect to."    },
  {"password", "MyPassword" , 32, "The password of the wifi network this device should connect to."},
  {0         , 0            ,  0, 0},
};


Cfg::Cfg(const char*appname, NvmField*fields, int seriallvl, int ledpin) {
  _appname = appname;
  _fields = fields;
  _seriallvl = seriallvl;
  _ledpin = ledpin;
  _cfg = false;
  _loop = 0;
  _restart = false;
  _websrv = 0;
  _dnssrv = 0;
  _nvm_ = 0; // creation is delayed (the Nvm constructor prints errors to Serial)
  _vals = 0; // creation is with creation of _nvm_
}


Nvm* Cfg::_nvm(void) {
  // Getter which constructs on first use
  if( _nvm_==0 ) {
    _nvm_ = new Nvm(_fields);
    int count = _nvm_->count();
    _vals = new char*[count];
    for( int ix=0; ix<count; ix++ ) {
      _vals[ix]= new char[NVM_MAX_LENZ];
      _nvm_->get(ix,_vals[ix]);
    }
    // _nvm_->dump();
  }
  return _nvm_;
}


Cfg::~Cfg(void) {
  if( _vals   !=0 ) {
    for( int i=0; i<_nvm_->count(); i++ ) delete[] _vals[i];
    delete[] _vals;
  }
  if( _nvm_  !=0 ) delete _nvm_;
  if( _dnssrv!=0 ) delete _dnssrv;
  if( _websrv!=0 ) delete _websrv;
}


void Cfg::check(int cfgwait, int butpin) {
  // Welcome
  LOGUSR("Press button on pin %d to enter configuration mode\n", butpin );
  // Configure pins
  if( _ledpin>=0 ) pinMode(_ledpin, OUTPUT);
  pinMode(butpin, INPUT);
  // Capture old values (we don't know if HIGH or LOW is default)
  int oldbut = digitalRead(butpin);
  // Now wait to see if user presses the button
  LOGDBG("Waiting for button ");
  int waited = 0;
  while( waited<cfgwait && !_cfg ) {
    if( _ledpin>=0 ) digitalWrite(_ledpin, (HIGH+LOW)-digitalRead(_ledpin) );
    waited++;
    if( digitalRead(butpin)!=oldbut ) _cfg= true;
    LOGDBGX(".");
    delay(CFG_FLASH_SETUP);
    waited++;
  }
  LOGDBGX("\n");
  // Feedback to user
  LOGDBG("Configuration mode: %s\n", _cfg ? "requested" : "no request");
}


bool Cfg::cfgmode(void) {
  return _cfg;
}


char * Cfg::getval(const char * name) {
  return getval( _nvm()->find(name) );
}


char * Cfg::getval(int ix) {
  char * s=0;
  if( 0<=ix && ix<_nvm()->count() ) s=_vals[ix];
  return s;
}


static String mac(int len=WL_MAC_ADDR_LENGTH, bool soft=false) {
  uint8_t macbuf[WL_MAC_ADDR_LENGTH];
  char    hexbuf[3*WL_MAC_ADDR_LENGTH+1];
  char * p= (char*)&hexbuf;
  if( soft ) WiFi.softAPmacAddress(macbuf); else WiFi.macAddress(macbuf);
  for(int i=WL_MAC_ADDR_LENGTH-len; i<WL_MAC_ADDR_LENGTH; i++ ) {
    uint8 d1=(macbuf[i]>>4)&0x0f;
    uint8 d0=(macbuf[i]>>0)&0x0f;
    *p++= d1>=10 ? d1+'A'-10 : d1+'0';
    *p++= d0>=10 ? d0+'A'-10 : d0+'0';
    //*p++= ':';
    //if( i==2 ) *p++= ':';
  }
  //*(p-1)= '\0'; // overwrite last ':'
  *p= '\0';
  return String((char*)&hexbuf);
}


void Cfg::setup(void) {
  LOGUSR("Entering configuration mode\n");
  // Compose SSID
  String name = String(_appname) + "-" + mac(3);
  // Start Access Point
  IPAddress ip(10, 10, 10, 10);
  WiFi.hostname(name.c_str()); // or wifi_station_set_hostname(name.c_str()); // needs extern "C" { #include "user_interface.h" }
  WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(name.c_str());
  // Start server
  _websrv = new ESP8266WebServer(80);
  _websrv->on("/"        , [this](){this->_handle_config(); } );
  _websrv->on("/save"    , [this](){this->_handle_save(); } );
  _websrv->on("/restart" , [this](){this->_handle_restart(); } );
  _websrv->onNotFound(     [this](){this->_handle_404(); } );
  _websrv->begin();
  LOGUSR("Join WiFi '%s' (open)\n",name.c_str());
  // Start dns on standard port (53)
  _dnssrv = new DNSServer();
  _dnssrv->start(53, "*", ip);
  LOGUSR("Then browse to any page (e.g. '%s')\n",ip.toString().c_str());
}


void Cfg::loop(void) {
  // Feedback 'waiting'
  _loop++;
  if( _ledpin>=0 ) digitalWrite(_ledpin, (HIGH+LOW)-digitalRead(_ledpin) );
  // Is there a restart request
  if( _restart ) {
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    LOGUSR("Restart will now be invoked...\n");
    delay(1000);
    ESP.restart();
    return;
  }
  // Give DNS and web server cycles
  _dnssrv->processNextRequest();
  _websrv->handleClient();
  // Wait
  delay(CFG_FLASH_LOOP);
}


static String head(const char * name) {
  String title=String("    <title>")+name+"</title>\r\n";
  String meta =String("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n");

  String style= R"====(
    <style>
      body{background:#8cc700; font-family:Arial,Helvetica,sans-serif; }
      div.head{background:#00a3c7; margin:10px; padding:10px; border:solid black 1px;}
      div.sub{background:#0fad00; margin:10px; padding:10px; border:solid black 1px;}
      input{background:#8cc700; padding:3px; border:solid black 1px;}
      .but{background:#00a3c7; padding:5px; text-decoration:none; color:black; font-size:small; border:solid black 1px; border-radius:8px;}
      th{text-align:left;}
      small{font-style:italic;}
    </style>
    
)===="; // http://www.tigercolor.com/color-lab/color-theory/color-harmonies.htm

  return String("<!DOCTYPE html>\r\n<html>\r\n  <head>\r\n")+title+meta+style+"  </head>\r\n";
}


static String body1(const char * name, const char * task) {
  return String("  <body>\r\n\r\n    <div class='head'><b>")+name+"</b><br/><small>"+task+"</small></div>\r\n";
}


static String body2(void) {
  return R"====(
    <div class='sub' style='text-align:right'>
      <a class='but' href='/restart' title='Restart without save'>Restart</a>
      <a class='but' href='/' title='Reload configuration without save'>Configure</a>
    </div>
    
  </body>
</html>
)===="; 
}


void Cfg::_handle_config(void) {
  LOGUSR("web: '%s' (config)\n",_websrv->uri().c_str() );  
  String body= String("\r\n    <div class='sub'>\r\n      <form action='save'><table>\r\n" );
 
  NvmField * field = _fields;
  while( field->name!=0 ) {
    if( field->len==0 ) {
      body+= String("\r\n")+
        "        <tr> <th colspan='3'>"+field->name+"&nbsp;</th> </tr>\r\n" 
        "        <tr> <td colspan='3'><small>"+field->extra+"</small></th> </tr>\r\n"; 
    } else {
      char val[NVM_MAX_LENZ];
      _nvm()->get(field->name,val);
      body += String("") +
        "        <tr>\r\n"
        "          <td>"+field->name+"&nbsp;</td>\r\n"
        "          <td style='width:90%;'><input type='text' name='"+field->name+"' id='"+field->name+"' maxlength='"+field->len+"' value='"+val+"' style='width:100%;'></td>\r\n"
        "          <td><b onclick='document.getElementById(\""+field->name+"\").value=\""+val+"\"' title='Reset to current'>&nbsp;&nbsp;&#x21B6;</b></td>\r\n"
        "          <td><b onclick='document.getElementById(\""+field->name+"\").value=\""+field->dft+"\"' title='Reset to default'>&#x2913;</b></td>\r\n"
        "        </tr>\r\n" 
        "        <tr> <td></td> <td><small>"+field->extra+"</small></td> </tr>\r\n";
    }
    if( field->extra[strlen(field->extra)-1]==' ' )
      body += "        <tr> <td>&nbsp;</td> </tr>\r\n";      
    field++;
  }
  body += "        <tr> <td><input class='but' type='submit' value='Save' title='Save and restart'></td> </tr>\r\n      </table><form>\r\n    </div>\r\n";

  _websrv->send(200,"text/html",head(_appname)+body1(_appname,"Edit configuration")+body+body2());
}


void Cfg::_handle_save(void) {
  LOGUSR("web: '%s'\n",_websrv->uri().c_str() ); 
  LOGDBG("web: %d args\n",_websrv->args() );                            

  String list="";
  for( int i=0; i<_websrv->args(); i++) {
    String name = _websrv->argName(i);
    String val = _websrv->arg(i);
    LOGDBG("web: arg[%d/'%s'] = '%s'\n",i,name.c_str(),val.c_str() );  
    int ix= _nvm()->find(name.c_str());
    if( ix==-1 ) {
      LOGUSR("Ignored: '%s' = '%s'\n",name.c_str(),val.c_str());       
    } else {
      _nvm()->put( ix, val.c_str());
      memcpy(_vals[ix],val.c_str(),NVM_MAX_LENZ);
      LOGUSR("Saved: '%s' = '%s'\n", name.c_str(), val.c_str() );
      if( list!="") list+=", ";
      list+="<i>"+name+"</i>"; 
    }
  }  
  
  if( list=="" ) list="Nothing to save."; else list="Saving "+list;
  String body= "    <div class='sub'>"+list+".<br/><br/>Will restart shortly.</div>\r\n";
  _websrv->send(200,"text/html",head(_appname)+body1(_appname,"Saving configuration")+body+body2());
  _restart=true;
}


void Cfg::_handle_restart(void) {
  LOGUSR("web: '%s'\n",_websrv->uri().c_str() );  
  String body= "    <div class='sub'>Will restart shortly.</div>\r\n";
  _websrv->send(200,"text/html",head(_appname)+body1(_appname,"Restarting")+body+body2());
  _restart=true;
}


void Cfg::_handle_404(void) {
  LOGUSR("web: '%s' not found\n",_websrv->uri().c_str() );  
  String body= "    <div class='sub'>Page not found.</div>\r\n";
  _websrv->send(404,"text/html",head(_appname)+body1(_appname,"Error")+body+body2());
}

