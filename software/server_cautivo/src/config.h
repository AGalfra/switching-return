//-------------------------------------
// Para Portal cautivo
const char* cautive_ssid = "Portal_Cautivo_ESP";
const char* password_ssid = "12345678";
//-------------------------------------
// Para modo Station
const char* ssid_sta     = "AG-Link-2.4";
const char* password_sta = "AleG@c60@5656";
const char* hostname_sta = "ESP8266";
//-------------------------------------
// Para modo Access Point
const char* ssid_ap     = "Controlador-Retorno";
const char* password_ap = "12345678";
//-------------------------------------
const char* http_username = "admin";
const char* http_password = "admin";
//-------------------------------------
IPAddress ip(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
//-------------------------------------
//WebSockets.hpp
void ProcessRequest(AsyncWebSocketClient *client, String request);
void updateGPIO_WS(uint8_t id, String pin, bool value);
void setGPIO_WS(uint8_t id, String text, bool state, bool ack);
void wifi_info_WS(bool full); 
void doAction(uint8_t actionId, String text);
//Server.hpp
void InitServer(void);
//R95_Utils.hpp
void InitRF95(void);
void setModulation(void);
void send_lora(void);
//ESP8266_Utils_AWS.hpp
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void InitWebSockets(void);
//ESP8266_Utils.hpp
void ConnectWiFi_STA(bool useStaticIP);
void print_connect_wifi(void);
void ConnectWiFi_AP(bool useStaticIP);
void configModeCallback(AsyncWiFiManager *myWiFiManager);
void scanWifi(void);

void tick(void);
String text_pin(uint8_t id);
void printRcv(char *buf, uint8_t from_adr);
void delete_wifi(void);
//-------------------------------------