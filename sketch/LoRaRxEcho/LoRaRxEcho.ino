/* Test LoRa . Use together with LoRaTxEcho transmitter sketch to test echo */
/* Receiver: this sketch listen for message coming and echoes it back.
*  It uses continuous receiving. When message arrive stop receiving mode and sends echo.
*  Then restore continuous receiving mode.
* 
*  On serial console are displaied also misured RSSI and SNR values.
*/

#include "LORA.h"
#include <SPI.h>

#define reclen 64       // buffer length
#define format "|Rssi: %d RssiPk: %d SnrPk %d|  "
#define pinf  7          // pin for signaling led
#define ACKMSG "RX ACK msg"

LORA LR;                // Class LORA instance
boolean SHIELD = true;
char recbuff[reclen];   // receiving buffer
char sendbuff[reclen];  // transmitting buffer
char data[64];          // buffer for RSSI and SNR values
const int SF = 10;             // Spreading factor value
const int BW = 7;             // Bandwidth value
const int PWR = 5;             //transmit power set to 100 mW (20 dBm)

void setup() 
{
  // mpol mod 24-08-16
  digitalWrite(pinf,0);
  pinMode(pinf,OUTPUT);
  // end mod
  Serial.begin(9600);
  if (!LR.begin()) 
  {
    Serial.println("No LoRa shield detected! It can't perform echo!");
    SHIELD = false;
    return;
  } 
  Serial.println("LoRa echo receiver.");   
  SX.setPower(PWR);
  LR.setConfig(SF,BW,1); // set different coding rate 1 (def: 9,6,4)
  showConfig();
  Serial.println("Waiting for message...");
  LR.receiveMessMode();   // set shield in continuous receiving mode
}

void loop() 
{
  if (!SHIELD)
  {
    delay(200);
    return;
  }
  
  if (getMess())
  {
    sendEcho();
    LR.receiveMessMode();
  } // if mess. send echo and return back to receiving mode

  // mpol mod 24-08-16
  digitalWrite(pinf,LOW);
  // end mod
}

boolean getMess()
{
  if ( LR.dataRead(recbuff,reclen) <= 0)
  {
    delay(10);
    return false;
  }
  
  Serial.print("< ");
  Serial.println(recbuff);
  snprintf(data,63,format,SX.getLoraRssi(),SX.lastLoraPacketRssi(),SX.lastLoraPacketSnr());
  Serial.println(data);
  // mpol mod 24-08-16
  digitalWrite(pinf,HIGH);
  delay(200);
  // end mod
  return true;
}

void sendEcho()
{
  SX.setState(STDBY);              // stop receiving mode
  delay(300);
  // mpol mod 22-07-17
  //strlcpy(sendbuff,recbuff,reclen);
  strlcpy(sendbuff,ACKMSG,32);
  Serial.print("> ");
  // mpol mod 22-07-17
  //Serial.println(sendbuff);
  if ( LR.sendMess(sendbuff) < 0)
    Serial.println("Sending error!");
  else
  {
    Serial.println("Received msg from TX, sent acknowledge");
  }
}

void showConfig()
{
  Serial.print("Frequence: ");
  Serial.println(SX.readFreq()); 
  Serial.print("Transmit power (mW): ");
  Serial.println(SX.getPower(2)); 
  Serial.print("Preamble bytes: ");
  Serial.print(SX.getLoraPreambleLen());
  Serial.println("+4");  
  snprintf(data,63,"SpFactor: %d BandW: %d Cr: %d",SX.getLoraSprFactor(),SX.getLoraBw(),SX.getLoraCr());
  Serial.println(data); 
  Serial.print("Rate (byte/sec): ");
  Serial.println(SX.getSRate());
}

