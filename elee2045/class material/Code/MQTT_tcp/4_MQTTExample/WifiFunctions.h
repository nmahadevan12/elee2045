bool connectToWifi(const char * ssid, const char * username, const char * password){
  WiFi.mode(WIFI_STA);
  M5.Display.print(" Connecting to "); M5.Display.println(ssid);
  WiFi.disconnect();
  if(strlen(username)==0){
    WiFi.begin(ssid,password);
  }else{
    WiFi.begin(ssid,WPA2_AUTH_PEAP,"",username,password);
  }
  M5.Display.clear();
  M5.Display.setCursor(0,10);
  M5.Display.printf(" Connecting to %s\n", ssid);
  for(int k=0;k<10;k++){
    M5.Display.print(".");
    M5.delay(1000);
    if(WiFi.status() == WL_CONNECTED){
      break;
    }
  }
  if(WiFi.status() == WL_CONNECTED){
    M5.Display.println(" Connected");
    M5.Display.println(WiFi.localIP());
    return true;
  }
  return false;
}