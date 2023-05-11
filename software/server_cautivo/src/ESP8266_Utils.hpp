//------------------------------------------------------------------------
//Conexion del ESP en modo Station, intenta conectar con las credenciales
//actuales, si no conecta dentro del tiempo establecido por timeout lanza
//WiFiManager con autoConnect()
//y sin importar el resultado de la conexion se reinicia.
void ConnectWiFi_STA(bool useStaticIP = false) {
  uint8_t timeout = 100;           // 100 para 5 segundos (100*50ms)

  Serial.println("");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.begin();  
  WiFi.persistent(true); 
  if(useStaticIP) WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(50);  
    Serial.print('.'); 
    timeout--;
    if(timeout == 0) 
      break;
  } 

  if(WiFi.status() == WL_CONNECTED) {    //Conectado correctamente a WIFI
    
    if (WiFi.getMode() != WIFI_STA) {
      Serial.println("Reasignando modo WIFI_STA");
      WiFi.mode(WIFI_STA);  
    }
    print_connect_wifi();               //Imprimir datos de la conexion
    ticker.detach();  
    digitalWrite(LED_WIFI, LOW);        //Led encendido fijo, conexion ok
  }
  else {                         //Error en conexion... lanzo WiFiManager
    AsyncWebServer server_cuativo(80);
    DNSServer dns_cautivo;  
    AsyncWiFiManager wifiManager(&server_cuativo, &dns_cautivo);
    //establece callback cuando falla la conexion WiFi
    //y entra en el modo configuracion
    wifiManager.setAPCallback(configModeCallback);
    // salir despues de la configuracion en lugar de conectarse
    //wifiManager.setBreakAfterConfig(true);
    if (!wifiManager.autoConnect(cautive_ssid)) {
      Serial.println("No se pudo conectar, se reiniciará para reintentar");
      delay(2000);
      ESP.restart();
    }
    //si llego aqui se ha conectado al wifi
    print_connect_wifi();             //Imprimir datos de la conexion
    //WiFiMode_t -> WIFI_OFF=0, WIFI_STA=1, WIFI_AP=2, WIFI_AP_STA=3
    Serial.println(WiFi.getMode());   
    delay(500);
    ESP.restart();
  }
}

//------------------------------------------------------------------------
//Muestra en consola los datos de la conexion actual.
void print_connect_wifi(void) {
//Conectado correctamente a la red en modo STA
  Serial.println("");                       
  Serial.println("Conexión... OK");
  Serial.print("Iniciado STA:\t");  
  Serial.printf("[CH %02d] [%s] %ddBm %s\n",
                  WiFi.channel(),
                  WiFi.BSSIDstr().c_str(),
                  WiFi.RSSI(),
                  WiFi.SSID().c_str());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

//------------------------------------------------------------------------
//Conexion del ESP en modo Access Point.
void ConnectWiFi_AP(bool useStaticIP = false) { 
   Serial.println("");
   WiFi.mode(WIFI_AP);
   while(!WiFi.softAP(ssid_ap, password_ap)) {
     Serial.println(".");
     delay(150);
   }
   if(useStaticIP) WiFi.softAPConfig(ip, gateway, subnet);

   Serial.println("");
   Serial.print("Iniciado AP:\t");
   Serial.println(ssid_ap);
   Serial.print("IP address:\t");
   Serial.println(WiFi.softAPIP());
}

//------------------------------------------------------------------------
//Esta funcion llamada cuando WiFiManager entra en modo de configuracion.
void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  Serial.println("WiFiManager -> config mode");
  Serial.println(WiFi.softAPIP());
  //se muestra SSID utilizado en modo AP
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //se ha ingresado al modo configuracion de WiFi, destello rapido del led
  
  ticker.attach(0.2, tick);      //Destello cada 0.2s - Modo Configuracion
}

//------------------------------------------------------------------------
//Realiza un escaneo de las redes Wifi disponibles, y lista los resultados
//en la consola.
void scanWifi(void) {
  String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t* bssid;
  int32_t channel;
  bool hidden;
  int scanResult;
  
  Serial.println(F("\nIniciando escaneo WiFi..."));
  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (scanResult == 0) {
    Serial.println(F("No se encontraron redes"));
  }
  else if (scanResult > 0) {
    Serial.printf(PSTR("%d redes encontradas:\n"), scanResult);
  
    for (int8_t i = 0; i < scanResult; i++) {

      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);
      Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %s\n"),
                        i,
                        channel,
                        bssid[0], bssid[1], bssid[2],
                        bssid[3], bssid[4], bssid[5],
                        rssi,
                        (encryptionType == ENC_TYPE_NONE) ? ' ' : '*',
                        hidden ? 'H' : 'V',
                        ssid.c_str());
      delay(0);
    }
  }
  else {
    Serial.printf(PSTR("Error en scanWiFi %d"), scanResult);
  }      
}