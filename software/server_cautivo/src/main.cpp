//------------------------------------------------------------------------
//                                LIBRERIAS
//------------------------------------------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <Ticker.h>
#include <ESPAsyncWiFiManager.h> 

//#define USE_LittleFS            
#ifdef USE_LittleFS
	#include <FS.h>
	#define SPIFFS LittleFS
	#include <LittleFS.h> 
#else
  #include <FS.h>
#endif
//------------------------------------------------------------------------
//                                CONSTANTES
//------------------------------------------------------------------------
#define LED_WIFI        0         //Led de estado de conexion WIFI (D3)
#define TRIGGER_PIN     4         //Acceso al portal cautivo (D2)

#define SERVER_ADDRESS  1         //Direccion del equipo Servidor
#define REMOTE_ADDRESS  2         //Direccion del equipo Remoto
                                  //CLK (D5), MISO (D6), MOSI (D7)
#define RFM95_CS        15        //Pin de CS del modulo SPI    (D8)
#define RFM95_RST       16        //Pin de reset modulo Lora   (D0)
#define RFM95_INT       5         //Pin de interrupcion para Rx Lora (D1)
#define LED_LORA        2         //Indicador de actividad de Tx/Rx (D4)
#define POWER_LORA      20        //Potencia de transmision (entre 3 y 23)
#define RETRIES_LORA    3         //Cantidad de reintentos (de 3 a 6) 
#define TIMEOUT_LORA    4         //Timeout entre reintentos (de 4 a 8)
#define RF95_FREQ       915.0     //Frecuencia del tranceptor Lora
#define CTE_TO_NORMAL   50        //CTE para ajustar el timeout
#define CTE_TO_SLOW     500      //CTE para ajustar el timeout en slow
#define SCAN_WIFI       1         //Escaneo de redes WiFi en el inicio
//------------------------------------------------------------------------
//                                 VARIABLES
//------------------------------------------------------------------------
char ver_soft[] = "Conmutador de Retorno v1.0";
char bienvenida[] = "---- Equipo Servidor ----";

bool reboot = false;                //Flag de reinicio del servidor web
bool slow = false;                  //Flag de modulacion lenta
bool switch_modulation = false;     //P/ realizar el cambio de modulacion
bool ws_connect = false;            //actualiza estado al conectar un cliente WS

StaticJsonDocument<300> doc;        //para serializar y deserializar json

String  comando = "";               //comando recibido a ejecutar
uint8_t idport;                     //id de puerto
String  texto;                      //descripcion del puerto
bool    estado;                     //estado del puerto variable auxiliar
uint8_t valor;                      //valor enviado en los comandos
bool    ack = 0;                    //envio de ACK por ws
int16_t rssi;
uint8_t txpower = POWER_LORA;       //potencia de tansmision lora [5-20]
uint8_t retries = RETRIES_LORA;     //cantidad de reintentos [3-6]
uint8_t timeout = TIMEOUT_LORA;     //timeout entre reintentos [4-8]
uint16_t aux;

bool    gpio[4] = {0,0,0,0};        //matriz de estado de los puertos
char    packet[12] = "";            //string de Tx Lora
char    buf_rx_lora [15];           //string de Rx Lora
uint8_t len = sizeof(buf_rx_lora);  //tamano del string

Ticker ticker;                //Objeto para control del led de estado WiFi
//------------------------------------------------------------------------
//                          FUNCIONES - CODIGO ANEXO
//------------------------------------------------------------------------
//Callback para control del LED de estado de WiFi.
void tick(void) {  
  bool state = digitalRead(LED_WIFI);    
  digitalWrite(LED_WIFI, !state);             
}

//------------------------------------------------------------------------
//Devuelve el texto descriptivo del puerto segun el id.
String text_pin(uint8_t id) {
    String desc;

    switch(id) {
    case 0:
      desc = "P1";
      break;
    case 1:
      desc = "P3";
      break;
    case 2:
      desc = "P4";
      break;
    case 3:
      desc = "P6";
      break;
    default:
      break;          
  }
  return desc;
}

//------------------------------------------------------------------------
#include "config.h"              //Datos de la red y coneccion
#include "ESP8266_Utils.hpp"     //Func. para manejo de WiFi en STA y AP
#include "WebSockets.hpp"        //Func. para comunicacion WebSockets
#include "Server.hpp"            //Func. para el manejo del servidor web
#include "ESP8266_Utils_AWS.hpp" //Inicializacion y eventos por WS
#include "RF95_Utils.hpp"        //Func. para el manejo del modulo RFM95
//------------------------------------------------------------------------
//Imprime en consola los datos de la recepcion LoRa y niveles de señal.
void printRcv(char *buf, uint8_t from_adr) {
  Serial.print("De: 0x");
  Serial.print(from_adr, HEX);
  Serial.print("-> ");
  Serial.println((char*)buf_rx_lora);
  Serial.print("RSSI: ");
  Serial.print(rf95.lastRssi(), DEC);
  Serial.print(", SNR: ");
  Serial.println(rf95.lastSNR(), DEC);      
}

//------------------------------------------------------------------------
//Secuencia para el borrado de credenciales WiFi al activar TRIGGER_PIN.
void delete_wifi(void) {  
  delay(60);                                //Retardo para evitar rebotes
  if(digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println("Botón presionado."); 
    delay(2000);                         //Espera de 4 seg para el borrado
    delay(2000);      
    if(digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Botón retenido.");
      Serial.println("Borrando credenciales...");      
      Serial.println("Reiniciando...");              
      WiFi.mode(WIFI_AP_STA);         //para borrar debe estar en modo STA
      WiFi.persistent(true);
      WiFi.disconnect(true);
      WiFi.persistent(false);        
      delay(1000);
      ESP.restart();         
    }      
  }
}

//------------------------------------------------------------------------
//                                SETUP
//------------------------------------------------------------------------
void setup(void) {
    
  pinMode(TRIGGER_PIN, INPUT_PULLUP);           //Configuracion de pines     
  pinMode(LED_LORA, OUTPUT);
  digitalWrite(LED_LORA, HIGH);           
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(LED_WIFI, OUTPUT);
  //Intenta conectarse en modo STA con las credenciales actuales  
  ticker.attach(0.6, tick);                 //Destello cada 0.6s 
  
  //while (!Serial);              //Esperar a que la consola este lista
  Serial.begin(115200);
  SPIFFS.begin();
  delay(20);
  
  Serial.println("\n");
  Serial.println(ESP.getFullVersion());     //Imprime version completa  
  Serial.println("");
  Serial.println(ver_soft);                 //Imprime version de la app
  Serial.println(bienvenida);

  if(SCAN_WIFI)
    scanWifi();                     //Escaneo de redes y visualizacion

  ConnectWiFi_STA(false);                   //conexion en modo Station
  //ConnectWiFi_AP();
  
  InitServer();                             //Iniciar servidor Web
  InitWebSockets();                         //Iniciar WebSocket
  InitRF95();                               //Inicializacion modulo LoRa
}

//------------------------------------------------------------------------
//                                BUCLE PRINCIPAL
//------------------------------------------------------------------------
void loop(void) {
  //verificar si hay dato disponible desde el equipo remoto
  if (manager.available()) { 
    String buf;
    uint8_t indice, from; 
    
    if (manager.recvfromAck((uint8_t *)buf_rx_lora, &len, &from)) {    
      //envia el ACK automaticamente y recibe los datos
      printRcv(buf_rx_lora, from);                    //mostrar en consola
      
      buf = String(buf_rx_lora);
      if (buf.indexOf("updateGPIO.") != -1) {
        indice = buf.indexOf("updateGPIO.");
        idport = buf.substring(indice+11, indice+12).toInt();
        estado = buf.substring(indice+12, indice+13).toInt();

        gpio[idport] = estado;
        updateGPIO_WS(idport, text_pin(idport), estado);
      }
      
    }
  }
  
  //----------------------------- Mensajes recibidos por WebSocket
  if(comando == "setGPIO") {
    comando += '.';
    comando.toCharArray(packet, sizeof(packet));
    itoa(idport, packet+8, 10);
    itoa(estado, packet+9, 10);
    packet[10] = 0;
    Serial.println(packet);  
    
    send_lora();                            //envio al equipo remoto
    setGPIO_WS(idport, texto, estado, ack); //actualizo estado en cliente WS
    comando = "";                  //borrar el comando, luego de ejecutar
  }

  //-----------------------------
  else if(comando == "doAction") {
    doAction(idport, texto);
    comando = "";                 
  }

  //-----------------------------
  else if(comando == "txpower") {
    comando += '.';
    comando.toCharArray(packet, sizeof(packet));
    itoa(valor, packet+8, 10);
    packet[10] = 0;
    Serial.println(packet);  
    
    txpower = valor;
    rf95.setTxPower(valor, false);   //Configuro potencia de Tx
    send_lora();                     //lo reenvio al equipo remoto
    Serial.println(""); 
    comando = "";                         
  } 
  
  //-----------------------------
  else if(comando == "retries") {
    comando += '.';
    comando.toCharArray(packet, sizeof(packet));
    itoa(valor, packet+8, 10);
    packet[9] = 0;
    Serial.println(packet);  
    
    retries = valor;
    manager.setRetries(valor);       //Configuro cantidad de reintentos
    send_lora();                     //lo reenvio al equipo remoto
    Serial.println("");  
    comando = "";        
  } 

  //-----------------------------
  else if(comando == "timeout") {
    comando += '.';
    comando.toCharArray(packet, sizeof(packet));
    itoa(valor, packet+8, 10);
    packet[9] = 0;
    Serial.println(packet);  
    
    timeout = valor;
    if(slow)
      aux = timeout * CTE_TO_SLOW;
    else
      aux = timeout * CTE_TO_NORMAL;

    manager.setTimeout(aux);          //Configuro timeout entre reintentos
    Serial.print("setTimeout-> ");
    Serial.println(aux, DEC);    
    send_lora();                      
    Serial.println("");  
    comando = "";                          
  } 

  //-----------------------------
  if(ws_connect) {                    //Si se ha conectado un cliente WS..
    wifi_info_WS(true);               //estado de Wifi full y config LoRa  
    String getpins = "getGPIO";
    getpins.toCharArray(packet, sizeof(packet));
    Serial.println(packet); 
    send_lora();                      //Envio de comando getGPIO
    Serial.println(""); 
    ws_connect = false;             //borrar la bandera, luego de ejecutar
  }  

  //----------------------------- Reinicio del ESP8266
  if(reboot){ 
    Serial.println("Rebooting...");   
    delay(50);
    ESP.restart();                        
  }

  //-----------------------------
  if(switch_modulation){ 
    String cmd = "setMOD.";
    cmd.toCharArray(packet, sizeof(packet));
    itoa(slow, packet+7, 10);
    packet[8] = 0;
    Serial.println(packet); 
    send_lora();                          //Envio de comando setMOD
    if(ack)
      setModulation();                    //Seteo esquema de modulacion
    modulation_WS();
    switch_modulation = 0;    
  }

  //----------------------------- Forzar el borrado de credenciales
  if(digitalRead(TRIGGER_PIN) == LOW) {      
    delete_wifi();
  }

  if(millis()%60000 == 0) {             //Cada 1 min envio estado de wifi
    wifi_info_WS();  
    //si slow=1 enviar <> para mantener la configuracion
    if(slow) {                            
      String cmd = "<>";
      cmd.toCharArray(packet, sizeof(packet));      
      packet[2] = 0;
      Serial.println(packet); 
      send_lora();                        //Envio de comando <>
      Serial.println(""); 
    }
  }
  //-----------------------------
  ws.cleanupClients();                //Eliminacion de clientes ws antiguos
  //-----------------------------
} 