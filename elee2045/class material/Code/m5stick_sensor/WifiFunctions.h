#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include <M5Unified.h>
#include <WiFi.h>

#define MAX_WIFI_NETWORKS 5

struct WifiCredential {
  char ssid[50];
  char username[50];
  char password[50];
};

bool connectToWifi(WifiCredential* credentials, int count){
  WiFi.mode(WIFI_STA);
  for(int i = 0; i < count; i++){
    M5.Display.clear();
    M5.Display.setCursor(0,10);
    M5.Display.printf(" Trying %s\n", credentials[i].ssid);
    WiFi.disconnect();
    
    if(strlen(credentials[i].username) == 0){
      WiFi.begin(credentials[i].ssid, credentials[i].password);
    }else{
      WiFi.begin(credentials[i].ssid, WPA2_AUTH_PEAP, credentials[i].username, credentials[i].username, credentials[i].password);
    }
    
    for(int k = 0; k < 10; k++){
      M5.Display.print(".");
      M5.delay(1000);
      if(WiFi.status() == WL_CONNECTED) break;
    }
    
    if(WiFi.status() == WL_CONNECTED){
      M5.Display.println(" Connected");
      M5.Display.println(WiFi.localIP());
      return true;
    }
  }
  return false;
}

#endif