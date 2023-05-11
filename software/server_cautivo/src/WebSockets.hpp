//------------------------------------------------------------------------
//Instancia del objeto WebSocket asincrono.
AsyncWebSocket ws("/ws");
//Instancia del objeto WebSocket EventSource.
//AsyncEventSource events("/events");
//------------------------------------------------------------------------
//Esta funcion es llamada una vez que se han recibido datos por WebSocket.
void ProcessRequest(AsyncWebSocketClient *client, String request) {

  DeserializationError error = deserializeJson(doc, request);
  if (error) { return; }

  Serial.print("/ws<- ");    //muestro en consola el meje recibido por WS
  Serial.println(request);
  
  comando = (String)doc["command"];             //cargo los datos de JSON 
  idport  = doc["id"];
  texto   = (String)doc["text"];
  estado  = doc["status"]; 
  valor   = doc["value"];
}

//------------------------------------------------------------------------
//Esta funcion envia el comando updateGPIO hacia el cliente WebSocket
//para actualizar el estado de un puerto.
void updateGPIO_WS(uint8_t id, String pin, bool value) {
  String response;
  StaticJsonDocument<300> doc;
  
  doc["command"] = "updateGPIO";
  doc["id"] = id;
  doc["text"] = pin;
  doc["status"] = value;
  doc["ack"] = 1;
  doc["rssi"] = rssi;
  serializeJson(doc, response);

  Serial.print("/ws-> ");
  Serial.println(response);  
  ws.textAll(response);

  Serial.print(pin);
  Serial.println(value ? String(" ON") : String(" OFF"));
  Serial.println("");
}

//------------------------------------------------------------------------
//Esta funcion devuelve como respuesta un comando setGPIO al cliente
//WebSocket para actualizar el estado de un puerto.
void setGPIO_WS(uint8_t id, String text, bool state, bool ack) {
  String response;                          //envio de ack al cliente ws
  StaticJsonDocument<300> doc;

  doc["command"] = "setGPIO";
  doc["id"] = id;
  doc["text"] = text;
  doc["status"] = state;
  doc["ack"] = ack;
  if (ack ==1)
    doc["rssi"] = rssi;
  else
    doc["rssi"] = "?";
  serializeJson(doc, response);

  Serial.print("/ws-> ");
  Serial.println(response);
  Serial.println("");  
  ws.textAll(response);
}

//------------------------------------------------------------------------
//Esta funcion solo envia el comando wifi_info con nivel RSSI del modulo
//Wifi si full=false, y si full=true envia todos los datos de la conexion
//al cliente WebSocket incluyendo ademas los parametros de configuracion
//del modulo LoRa.
void wifi_info_WS(bool full=false) {
  String response;                          //envio de ack al cliente ws
  StaticJsonDocument<300> doc;

  doc["command"] = "wifi_info";
  //doc["id"] = 0;
  doc["signal"] = WiFi.RSSI();

  if(full) {
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP();
    doc["channel"] = WiFi.channel();
    doc["mac"] = WiFi.BSSIDstr();
    doc["txpower"] = txpower;
    doc["retries"] = retries;
    doc["timeout"] = timeout;
    doc["slow"] = slow;
  }

  serializeJson(doc, response);

  Serial.print("/ws-> ");
  Serial.println(response);
  Serial.println("");  
  ws.textAll(response);
}

//------------------------------------------------------------------------
void modulation_WS(void) {
String response;                              //envio de ack al cliente ws
  StaticJsonDocument<200> doc;

  doc["command"] = "doAction";
  doc["id"] = idport;
  doc["status"] = estado;

  serializeJson(doc, response);

  Serial.print("/ws-> ");
  Serial.println(response);
  Serial.println("");  
  ws.textAll(response);
}

//------------------------------------------------------------------------
//Esta funcion procesa el comando "doAction" enviado por el cliente WS.
void doAction(uint8_t actionId, String text) {
  Serial.print("Do action: ");
  Serial.print(text);
  Serial.print(" - ");
  Serial.println(estado);
  //-----------------------
  switch(actionId){
    case 0:
      if(estado)
        slow = 1;
      else
        slow = 0;
      switch_modulation = 1;
      break;
    case 1:
      reboot = 1;
      break;
    default:               
      break;
  }  
}