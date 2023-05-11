//------------------------------------------------------------------------
//Instancia del Objeto server.
AsyncWebServer server(80);
//------------------------------------------------------------------------
//Inicializacion del servidor web.
void InitServer(void) { 

	server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html")
	.setAuthentication(http_username, http_password);
	
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->send(400, "text/plain", "Not found");
	});

	server.on("/p", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hola mundo!!!");
  	});
 
	server.begin();
	Serial.println("HTTP server started");
}