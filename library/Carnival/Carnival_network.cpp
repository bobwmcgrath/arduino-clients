/*
  Carnival_network.cpp - Carnival library
  Copyright 2016 Neil Verplank.  All right reserved.
*/

// include main library descriptions here as needed.
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "WConstants.h"
#endif

#include      <ESP8266WiFi.h>
#include      <Carnival_PW.h>
#include      <Carnival_network.h>
#include      <Carnival_debug.h>
#include      <Carnival_leds.h>


extern "C" {
#include "gpio.h"
}
extern "C" {
#include "user_interface.h"
}

#define       REDLED        0        // what pin?
#define       BLUELED       2        // what pin?
const boolean ON            = LOW;     // LED ON
const boolean OFF           = HIGH;    // LED OFF


WiFiClient  client;
int         status          = WL_IDLE_STATUS;
String      WHO             = "";
boolean     DEBUG           = 1;

extern Carnival_debug debug;
extern Carnival_leds leds;

Carnival_network::Carnival_network()
{
}



/*================= WIFI FUNCTIONS =====================*/


void Carnival_network::start(String who, bool dbug) {
    WHO    = who;
    DEBUG  = dbug;
}

void Carnival_network::connectWifi(){

  debug.MsgPart("Attempting to connect to SSID: ");
  debug.Msg(ssid);

  if ( WiFi.status() != WL_CONNECTED) {

// This may be required for lower power consumption.  unconfirmed.
        WiFi.mode(WIFI_STA);
        status = WiFi.begin(ssid, pass);  // connect to network
        // flash and wait 10 seconds for connection:
        leds.flashLED(BLUELED, 10, 166, false); // flash slowly (non-blocking) to show attempt
    }
    if (DEBUG) {
       if (status==WL_CONNECTED) { printWifiStatus(); }
    }
}



int Carnival_network::reconnect(bool output) {
    // wait for a new client:
    // Attempt a connection with base unit
    if (output) { debug.Msg("Confirming connection..."); }

    int con=0;
    // check "connected() less frequently
    if (!client || (output && !client.connected())) {
        con = client.connect(HOST,PORT);
        if (con) {
          leds.setLED(BLUELED,1);
          // re-announce who we are to the server
          // clean out the input buffer:
          client.flush();
          client.println(WHO); // Tell server which game
          if (output) {
            debug.MsgPart(WHO);
            debug.Msg(": ONLINE");
          }
       } else {
         debug.Msg("couldn't connect to host:port");
       }
    } else { con =1; }
    if (!con) {
      
        debug.Msg("Socket connection failed"); 
        leds.setLED(BLUELED,0);
        leds.flashLED(REDLED, 5, 25, false);   // flash 5 times to indicate failure
        delay(1000);            // wait a little before trying again
        return 0;
        
    } else {
        return 1;
    }
}


void Carnival_network::printWifiStatus() {

     if (!DEBUG) { return; }
     
     debug.MsgPart("\nWireless Connected to SSID: ");
     debug.Msg(WiFi.SSID());

     IPAddress ip = WiFi.localIP();
     Serial.print("IP Address: ");
     Serial.println(ip);
    
     long rssi = WiFi.RSSI();
     debug.MsgPart("signal strength (RSSI):");
     debug.MsgPart(rssi);
     debug.Msg(" dBm");
    
}


// send a well-formatted message to the server
// looks like "WHOMAI:some message:optional data"

void Carnival_network::callServer(String message){
  
    String out = WHO;
    out += ":";
    out += message;
    out += ":";
    client.flush();  // clear buffer
    client.println(out);
    leds.blinkBlue(3, 10, 1); // show communication
    
}

void Carnival_network::callServer(int message, int optdata){
  
    String out = WHO;
    out += ":";
    out += message;
    out += ":";
    out += optdata;

    client.flush();  // clear buffer
    client.println(out);
    leds.blinkBlue(3, 10, 1); // show communication
    
}


void callback() {
    debug.Msg("Woke up from sleep");
}

void Carnival_network::sleepNow(int wakeButton) {
  pinMode(wakeButton, INPUT_PULLUP);
  debug.Msg("going to light sleep...");
  leds.setLED(BLUELED,0);
  delay(100);
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
  gpio_pin_wakeup_enable(GPIO_ID_PIN(wakeButton), GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for LOW pulses on pin. 
  wifi_fpm_open();
  wifi_fpm_set_wakeup_cb(callback); //wakeup callback
  wifi_fpm_do_sleep(0xFFFFFFF); 
  delay(100); 
  connectWifi();
  reconnect(1);
}

