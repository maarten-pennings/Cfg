/*
  cfgtest.ino - Illustrates how to more easily test a larger web page (and demo's web page markup features)
  Created by Maarten Pennings 2020 Marcg 8
*/
#include <Cfg.h>


// This configuration is an example (used in my WordClock project) that uses several markup features.
//  - Some items (e.g. "Access points") have a length of 0.
//    This renders them (the "name" field) as a header on the web page.
//    The "extra" field is used as sub header.
//  - Some items (e.g. "Password.1") have an "extra" field ending in a space.
//    This adds a white line after them. Note how this formats the SSID/Password pairs.
//    It is good practice to have a white line before a header.
//    That is why "Password.3", "Round", "Color.4", and "Animation" all end with a white line.
//  - Some items (e.g. "Refresh") have embedded html.
//    Since 'extra' is only used on the web page, that is ok.
//    Do note "Timezone"; it has an html link, but that is dubious since host seeing the generated
//    web page is connected to the ESP8266 and thus not to internet, and thus the link can not be opened.
NvmField CfgFields[] = {
  {"Access points"   , ""                           ,  0, "The clock uses internet to get time. Supply credentials for one or more WiFi access points (APs). " },
  {"Ssid.1"          , "SSID for AP1"               , 32, "The ssid of the first wifi network the WordClock could connect to (mandatory)." },
  {"Password.1"      , "Password for AP1"           , 32, "The password of the first wifi network the WordClock could connect to (mandatory). "},
  {"Ssid.2"          , "SSID for AP2"               , 32, "The ssid of the second wifi network (optional, may be blank)." },
  {"Password.2"      , "Password for AP2"           , 32, "The password of the second wifi network (optional, may be blank). "},
  {"Ssid.3"          , "SSID for AP3"               , 32, "The ssid of the third wifi network (optional, may be blank)." },
  {"Password.3"      , "Password for AP3"           , 32, "The password of the third wifi network (optional, may be blank). "},
  
  {"Time management" , ""                           ,  0, "Time is obtained from so-called NTP servers. They provide UTC time, so also the time-zone must be entered. " },
  {"NTP.server.1"    , "pool.ntp.org"               , 32, "The hostname of the first NTP server (mandatory)." },
  {"NTP.server.2"    , "europe.pool.ntp.org"        , 32, "The hostname of a second NTP server (optional, may be blank)." },
  {"NTP.server.3"    , "north-america.pool.ntp.org" , 32, "The hostname of a third NTP server (optional, may be blank). " },
  {"Timezone"        , "CET-1CEST,M3.5.0,M10.5.0/3" , 48, "The timezone string (including daylight saving), see <A href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>details</A>. " },
  {"Round"           , "150"                        ,  4, "The number of seconds to add to the actual time (typically 150, to round 2.5 min up to 5 min intervals). " },
  
  {"Color palette"   , ""                           ,  0, "The colors for the letter categories. Use RRGGBB, hex digits for red, green and blue. E.g. FFFF00 is bright yellow, 000011 dim blue. " },
  {"Color.1"         , "110000"                     ,  6, "The first color; by default used for hour category(een-twaalf)." },
  {"Color.2"         , "001100"                     ,  6, "The second color; by default used for minute category (vijf, tien, kwart, half)." },
  {"Color.3"         , "000011"                     ,  6, "The third color; by default used for help category (voor, over)." },
  {"Color.4"         , "222222"                     ,  6, "The fourth color; for some animations. " },
  
  {"Display"         , ""                           ,  0, "Which colors are used when. " },
  {"Refresh"         , "one"                        ,  8, "When is the display refreshed: <b>one</b> (every minute - useless for <b>fix</b>/<b>none</b>), <b>five</b> (every 5 minutes)." },
  {"Mapping"         , "cycle"                      ,  8, "Mapping: <b>fix</b> (fixed to default), <b>cycle</b> (colors cycle over categories), <b>random</b> (random between Color.1 and Color.2)." },  
  {"Animation"       , "hor"                        ,  8, "The animation: <b>none</b>, <b>wipe</b> (horizontal wipe, using Color.4), <b>dots</b> (letter by letter off then on). " },  
  {0                 , 0                            ,  0, 0},  
};

Cfg cfg("CfgTest", CfgFields );

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWordClock Cfg test\n\n");

  // Normally there would be   
  //    cfg.check();
  //    if( cfg.cfgmode() ) { cfg.setup(); }
  // but this is an example of how to quickly test the web page, so we always start it.

  cfg.setup();
}

void loop() {
  // This is an example of how to quickly test the web page, so we always run it.
  cfg.loop();
}
