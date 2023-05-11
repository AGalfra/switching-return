//------------------------------------------------------------------------
//                                LIBRERIAS
//------------------------------------------------------------------------
#include <Arduino.h>

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
//------------------------------------------------------------------------
//                                CONSTANTES
//------------------------------------------------------------------------
#define SERVER_ADDRESS  1         // Direccion del equipo Servidor
#define REMOTE_ADDRESS  2         // Direccion del equipo Remoto

#define BOOT1           PB2       // para condicion de arranque 

#define RFM95_CS        PA4       // Pin de CS del modulo SPI
#define RFM95_RST       PA3       // Pin de reset modulo Lora
#define RFM95_INT       PA2       // Pin de interrupcion para Rx Lora
#define LED_LORA        PC14      // Pin par indicar actividad de Tx/Rx
//#define LED_LORA        PC13    // Pin par indicar actividad de Tx/Rx
#define POWER_LORA      20        // Potencia de transmision (de 5 a 23)
#define RETRIES_LORA    3         // Cantidad de reintentos (de 3 a 6)
#define TIMEOUT_LORA    4         // Timeout entre reintentos (4 a 8)
#define RF95_FREQ       915.0     // Frecuencia del tranceptor Lora
#define CTE_TO_NORMAL   50        // CTE para ajustar el timeout
#define CTE_TO_SLOW     500      // CTE para ajustar el timeout en slow
#define TIEMPO          80000     // 80 segundos timeout para millis()
                                  // Definicion de pines de salida
#define PORT0           PB9       // para P1
#define PORT1           PB8       // para P2/P3
#define PORT2           PB7       // para P4
#define PORT3           PB6       // para P5/P6

#define LOGICA_NEGATIVA 1         // Logica neg para control de puertos
//------------------------------------------------------------------------
//                                 VARIABLES
//------------------------------------------------------------------------
char ver_soft[]   = "\nConmutador de Retorno v1.0";
char bienvenida[] = "----- Equipo Remoto -----\n";
char buf[RH_RF95_MAX_MESSAGE_LEN];

uint8_t  idport;                      //Id de puerto
bool     estado;                      //Estado del puerto variable aux
bool     gpio[4] = {0,0,0,0};         //Matriz de estado de los puertos
uint8_t  ports[4] = {PORT0, PORT1,    
                     PORT2, PORT3};   //Matriz de puertos
char     packet[14] = "updateGPIO.";  //Array de datos Lora 
uint8_t  i;                           //Contador
bool     rebooting = 1;               //Indica que el uC esta iniciando
bool     slow = false;                //Flag de modulación lenta
bool     refresh = false;
uint8_t  power = POWER_LORA;          //Potencia de tx LoRa 
uint8_t  retries = RETRIES_LORA;      //Cant. de reintentos p/ sendtoWait
uint16_t timeout = TIMEOUT_LORA;      //Timeout entre reintentos
uint16_t aux;
uint32_t previousMillis;
//------------------------------------------------------------------------
//                          FUNCIONES - CODIGO ANEXO
//------------------------------------------------------------------------
#include "RF95_Utils.hpp"      //Funciones para el manejo del modulo RFM95

//------------------------------------------------------------------------
//Imprime en consola de los datos recibidos por el modulo Lora junto con 
//los niveles de recepcion.
void printRcv(char *buf, uint8_t from_adr) {
  Serial.println(""); 
  Serial.print("De: 0x");
  Serial.print(from_adr, HEX);
  Serial.print("-> ");
  Serial.println((char*)buf);
  //RH_RF95::printBuffer("Trama completa: ", buf, len);
  Serial.print("RSSI: ");
  Serial.print(rf95.lastRssi(), DEC);
  Serial.print(", SNR: ");
  Serial.println(rf95.lastSNR(), DEC);  
}

//------------------------------------------------------------------------
//Envia estado actualizado de todos los puertos y los imprime en consola.
void updateGPIO(void) {                   
  for (i=0;i<4;i++) {    
    if(LOGICA_NEGATIVA)
      gpio[i] = !(digitalRead(ports[i]));   
    else
      gpio[i] = digitalRead(ports[i]);   

    itoa(i, packet+11, 10);
    itoa(gpio[i], packet+12, 10);
    packet[13] = 0;
    Serial.println(packet); 
    delay(10);
    send_lora();
    Serial.println("");     
  }  
}

//------------------------------------------------------------------------
//                                SETUP
//------------------------------------------------------------------------
void setup() {

  pinMode(BOOT1, INPUT); 
  pinMode(LED_LORA, OUTPUT);            //configuracion de pines
  digitalWrite(LED_LORA, HIGH);         
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  if(digitalRead(BOOT1) == 1)
    while (!Serial);              //espera hasta que la consola este lista
  
  Serial.begin(115200);
  delay(1000);  
  Serial.println(ver_soft);  
  delay(10);
  Serial.println(bienvenida);

  for (i=0;i<4;i++) {           //Configuracion e inicializacion de pines
    pinMode(ports[i], OUTPUT);   
    if(LOGICA_NEGATIVA)       
      digitalWrite(ports[i], 0);    
    else
      digitalWrite(ports[i], 1);
  }  
  Serial.println("ON todos los puertos");
  
  InitRF95();                           //inicializacion del modulo RFM95
  delay(50);
}

//------------------------------------------------------------------------
//                                BUCLE PRINCIPAL
//------------------------------------------------------------------------
void loop() {

  if(rebooting){
    updateGPIO();         //Al reiniciar enviar estado de todos los puertos
    rebooting = 0;
  }
  if(((millis() - previousMillis) >= TIEMPO) && (slow == 1)) {
    if(refresh == 0) {              //si no se recibio el refresh <>...
      slow = 0;
      setModulation();            //volver a modulacion normal(por defecto)
    }
    refresh = 0;
    previousMillis = millis();
  }

  //-----------------------------  
  if (manager.available()) {      //Verificar si hay un mje LoRa disponible
    uint8_t len = sizeof(buf);
    uint8_t from, indice; 
    String rx;

    //recibo la trama y envio el acuse de recibo automaticamente
    if (manager.recvfromAck((uint8_t *)buf, &len, &from)) {   

      digitalWrite(LED_LORA, LOW);
      delay(50);                  
      printRcv(buf, from);               //imprimir en consola datos de rx
      digitalWrite(LED_LORA, HIGH);
            
      rx = String(buf);
      //----------------------------- Comando setGPIO
      if (rx.indexOf("setGPIO.") != -1) { 
        indice = rx.indexOf("setGPIO.");
        idport = rx.substring(indice+8, indice+9).toInt();
        estado = rx.substring(indice+9, indice+10).toInt();
        
        if(LOGICA_NEGATIVA)
          digitalWrite(ports[idport], (!estado)); 
        else
          digitalWrite(ports[idport], (estado));
      }      
      //----------------------------- Comando getGPIO
      else if (rx.indexOf("getGPIO") != -1) {
        Serial.println("");
        delay(50);    
        updateGPIO();            //envio updateGPIO de todos los puertos
      }
      //----------------------------- Comando txpower
      else if (rx.indexOf("txpower.") != -1) { 
        indice = rx.indexOf("txpower.");
        if(rx.substring(indice+9, indice+10) == 0)
          power = rx.substring(indice+8, indice+9).toInt();
        else {
          power = 10 * rx.substring(indice+8, indice+9).toInt();
          power += rx.substring(indice+9, indice+10).toInt();
        }
        rf95.setTxPower(power, false);
        Serial.print("setTxPower-> ");
        Serial.println(power, DEC);        
      }
      //----------------------------- Comando retries
      else if (rx.indexOf("retries.") != -1) { 
        indice = rx.indexOf("retries.");
        retries = rx.substring(indice+8, indice+9).toInt();   
        manager.setRetries(retries);
        Serial.print("setRetries-> ");
        Serial.println(retries, DEC);        
      }
      //----------------------------- Comando timeout
      else if (rx.indexOf("timeout.") != -1) {  
        indice = rx.indexOf("timeout.");
        timeout = rx.substring(indice+8, indice+9).toInt();           
        
        if(slow)
          aux = timeout * CTE_TO_SLOW;
        else
          aux = timeout * CTE_TO_NORMAL;
        manager.setTimeout(aux);   
        Serial.print("setTimeout-> ");
        Serial.println(aux, DEC);        
      }
      //----------------------------- Comando setMOD
      else if (rx.indexOf("setMOD.") != -1) {
        indice = rx.indexOf("setMOD.");
        slow = rx.substring(indice+7, indice+8).toInt();      
        setModulation();
        previousMillis = millis();
      }
      //----------------------------- Comando refresh
      else if (rx.indexOf("<>") != -1) {
        refresh = 1;        
      }
    }
  } 
}