void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) { 
	if(type == WS_EVT_CONNECT) {
    IPAddress ip_r = client->remoteIP();
		Serial.printf("ws[%s][%u] connected from: %d.%d.%d.%d\n", server->url(), client->id(), ip_r[0], ip_r[1], ip_r[2], ip_r[3]);
		client->printf("Hello Client %u", client->id());
		client->ping();		
    //----------------------------------------
    // Actualizacion de estados al conectar WS
    //----------------------------------------    
    ws_connect = true;    
    //----------------------------------------
	}
	else if(type == WS_EVT_DISCONNECT) {
		Serial.printf("ws[%s][%u] disconnect!\n", server->url(), client->id());
	}
	else if(type == WS_EVT_ERROR) {
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
	}
	else if(type == WS_EVT_PONG) {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    Serial.println("");
	}
	else if(type == WS_EVT_DATA) {
		AwsFrameInfo * info = (AwsFrameInfo*)arg;
		String msg = "";
		if(info->final && info->index == 0 && info->len == len){
			if(info->opcode == WS_TEXT){
				for(size_t i=0; i < info->len; i++) {
					msg += (char) data[i];
				}
			} else {
				char buff[4];
				for(size_t i=0; i < info->len; i++) {
					sprintf(buff, "%02x ", (uint8_t) data[i]);
					msg += buff ;
				}
			}

			if(info->opcode == WS_TEXT)
			ProcessRequest(client, msg);
			
		} else {
			//message is comprised of multiple frames or the frame is split into multiple packets
			if(info->opcode == WS_TEXT){
				for(size_t i=0; i < len; i++) {
					msg += (char) data[i];
				}
			} else {
				char buff[4];
				for(size_t i=0; i < len; i++) {
					sprintf(buff, "%02x ", (uint8_t) data[i]);
					msg += buff ;
				}
			}
			Serial.printf("%s\n",msg.c_str());

			if((info->index + len) == info->len){
				if(info->final){
					if(info->message_opcode == WS_TEXT)
					ProcessRequest(client, msg);
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void InitWebSockets(void) {
	ws.onEvent(onWsEvent);                      //attach AsyncWebSocket
	server.addHandler(&ws);    
	//server.addHandler(&events);				//attach AsyncEventSource
	Serial.println("WebSocket server started");
	Serial.println("");
}