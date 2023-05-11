//-------------------------------------------------------------------------
//Instancia del Driver de radio (RH_RF95)
RH_RF95 rf95(RFM95_CS, RFM95_INT);
//Instancia del Manager, clase para gestionar mensajes de longitud
//variable, direccionados, fiables, retransmitidos y reconocidos.
RHReliableDatagram manager(rf95, REMOTE_ADDRESS);
//-------------------------------------------------------------------------
//Inicializacion del modulo Lora
void InitRF95() {

  //digitalWrite(LED_LORA, LOW);         // apagar led lora  
  digitalWrite(RFM95_RST, LOW);          // Reset manual Lora
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  
  if(!manager.init()) {
    Serial.println("INIT FAILED");
    delay(100);
  }
  else
    Serial.println("LoRa radio init OK!");

  //Los valores predeterminados despues de init son:
  //434.0MHz, 13dBm, Bw=125kHz, Cr=4/5, Sf=128chips/symbol, CRC on

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency FAILED");
    delay(100);
  }  
  else {
    Serial.print("Frecuencia: ");
    Serial.print(RF95_FREQ);
    Serial.println(" MHZ\n");
  }
  //La potencia del transmisor predeterminada es 13dBm, usando PA_BOOST.
  //Si esta usando modulos RFM95/96/97/98 que utilizan el pin PA_BOOST,
  //entonces puede configurar las potencias del transmisor de 2 a 20 dBm
  rf95.setTxPower(POWER_LORA, false);

  //Opcionalmente, puede solicitar que el modulo espere que la deteccion 
  //de actividad del canal no muestre actividad en el canal antes de
  //transmitir con el tiempo de espera de CAD en un valor distinto de cero:
//  driver.setCADTimeout(10000);

}

//-------------------------------------------------------------------------
//Cambia la configuracion de modulacion: normal o lenta (largo alcance)
// y ajusta el timout a un valor adecuado
void setModulation(void) {
  
  if (slow) {                                         
    if (!rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512)) { //Modul. lenta  //Bw125Cr45Sf2048
      Serial.println("\nsetModemConfig FAILED");
      delay(100);
    }  
    else {
      Serial.println("\nModulación: Bw31.25 Cr4/8 Sf512");
      //Serial.println("\nModulación: Bw125 Cr4/5 Sf2048");            
      manager.setTimeout(timeout * CTE_TO_SLOW);
      Serial.print("setTimeout-> ");
       Serial.println(timeout * CTE_TO_SLOW); 
    }    
  }
  else {
    if (!rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128)) { //Modul. normal
        Serial.println("\nsetModemConfig FAILED");
        delay(100);
      }  
      else {
        Serial.println("\nModulación: Bw125 Cr4/5 Sf128");      
        manager.setTimeout(timeout * CTE_TO_NORMAL);
        Serial.print("setTimeout-> ");
        Serial.println(timeout * CTE_TO_NORMAL);      
      }      
  }  
}

//-------------------------------------------------------------------------
//Envio packet al destino 'SERVER_ADDRESS' utilizando la clase 
//RHReliableDatagram el dato es almacenado previamente en packet.
void send_lora(void) {

  digitalWrite(LED_LORA, LOW);
  delay(50);
  if(manager.sendtoWait((uint8_t *)packet,sizeof(packet),SERVER_ADDRESS)) {  
    // Si es true se recibio el ack correctamente desde el destinatario
    Serial.println("sendtoWait OK");
    Serial.print("RSSI: ");
    Serial.print(rf95.lastRssi(), DEC);
    Serial.print(", SNR: ");
    Serial.println(rf95.lastSNR(), DEC);
  }
  else {    
    Serial.println("Falla en sendtoWait");
  }
  digitalWrite(LED_LORA, HIGH);

}
//--------------------------------------------------------------------------
