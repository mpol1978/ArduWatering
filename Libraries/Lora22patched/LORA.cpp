/*
  Copyright (c) 2015 Daniele Denaro.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

/******************************************************************************/
/* Class LORA. 
*  This class semplifies class SX1278 in case of LoRa protocol using.
*  Just use begin function and sendMess or receiveMessMode to communicate.
*  Few simple functions:
*  - begin(key) : initialize for LoRa protocol,  with cryptography
*  - defDevRange(code) : define net addresses structure (local addresses range)
*  - defNetAddress(address) : define net address 
*  - sendNetMess(local dest, local sender, mess, length) : message sending
*  - receiveMessMode() :  set shield in continuous receving mode and then you have 
*                         to poll message coming with receiveNetMess(...) in a loop.
*  - receiveNetMess(local dest, local sender, buffer, max length) : receive mess
*                          
*  Basic functions (no cripto, no address; just for transmission testing):
*  - begin() : initialize shield for LoRa protocol (default power: 10dBm).
*    If shield is not here it returns false.
*  - sendMess() : send message (as null terminated string or byte array) 
*  - receiveMessMode() and dataRead() to receive message.
*    receiveMessMode() set shield in continuous receving mode and then you have 
*    to poll message coming with dataRead() in a loop. 
*    
*  
*    
* Author: Daniele Denaro 2015
*/

#include <LORA.h>


/******* With AES256 cryptography and sender/destination addresses ***********/

/********* Initializing ********/

/* Start shield in LoRa mode and prepare 32 bytes key for AES256 crypto*/
bool LORA::begin(unsigned int keyval)
{
  if (!begin()) return false;
  SX.createKey(keyval);
  return true;
}

/* Change shield in LoRa mode, if it was already started in different mode
   with AES256 key creation */
void LORA::setModeLora(unsigned int keyval)
{
  setModeLora();
  SX.createKey(keyval);
}

/* Define net structure. This function must be called before defNetAddress
*  Code has to be from 2 to 14 and is the exponent of power of 2.
*  Device address address range will be from 1 and 2^code -1. 0 address is 
*  reserved for broadcasting.
*  Ex.:
*  code 2 -> device addresses allowed 1-3 net max address 16683
*  code 3 -> device addresses allowed 1-7 net max address 8191
*  code 4 -> device addresses allowed 1-15 net max address 4095
*  code 5 -> device addresses allowed 1-31 net max address 2047
*  code 6 -> device addresses allowed 1-63 net max address 1023
*  code 7 -> device addresses allowed 1-127 net max address 511
*  code 8 -> device addresses allowed 1-254 net max address 254
*  :
*  code 13 -> device addresses allowed 1-8191 net max address 7
*  Function returns the resulting maximun net address allowed.
*  Or -1 if code error.
*/
int LORA::defDevRange(unsigned char code)
{
  if (code>14) return -1;
  r2p=code;         // mask : (2^r2p -1)
  mask=(1<<r2p)-1;  //ex: r2p=7 -> mask=0x007F (00000000 01111111) range:1-127
  netmask=~mask;  //ex: 11111111 10000000
  maxnetadd=netmask>>r2p;  //ex: maxnet=0000001 11111111
  return maxnetadd;
}

/* Define net address impliedly used by routines for sending and receive.
*  This function must be called after device range definiction (defDevRange(...))
*/
bool LORA::defNetAddress(unsigned int add)
{
   if (add>maxnetadd) return false;
   netAddress=add<<r2p;
}


/* DEPRECATED: use defDevRange and defNetAddress */
/* Set a network address if you like to create a network with net address and
*  a range of device addresses. In this case device adresses range must be a
*  2 power value (8,16,32,64,128,256 ecc.). Besides net address value must be 
*  lower than 0xFFFF/device_range (I.E. 0x1FFF,0xFFF,0x7FF,0x3FF,0x1FF,0xFF ecc) 
*  This function MUST be called before calling sender and receiver functions that
*  use sub-net adressess!
*/
bool LORA::setNetAddress(unsigned int netAdd, unsigned int devRange)
{
    r2p=log(devRange)/log(2);
    unsigned int devR=1<<r2p;
    if (devR<devRange) {r2p++;}
    devRange=1<<r2p;
    mask=devRange-1;
    long net=netAdd << r2p;
    if (net>0xffff) return false;
    netAddress=net;
    return true;
}

/******** Sending **********/
/* Send buffer mess adding a word as addressee (destAdd) and a word as sending 
*  address. 
*  A random byte is added just to make message unique.
*  Sending address, random byte and message are encoded with AES256 cryptography
*  using predefined 32 key. Length of encoded segment is a multiple of 16 bytes
*  blocks.
*  Destination word (two bytes) is not encoded.
*/
int LORA::sendMess(unsigned int destAdd, unsigned int sendAdd, byte *mess, int lmess)
{
  int lenEnc=lmess+2+1;                  //len of buffer segment to encode 
  int nbk=int(ceil((float)lenEnc/16));
  lenEnc=nbk*16;
  int lenBuff=lenEnc+2;                  //len of total buffer to send
  byte *buff=(byte *)calloc(lenBuff,1);  //buffer to send
  buff[0]=highByte(destAdd);buff[1]=lowByte(destAdd);  //dest address plain
  byte *buffEnc=&buff[2];                //buffer segment to encode
  
  marker=random(256);
  buffEnc[0]=marker;                     //just to make message univocal
  buffEnc[1]=highByte(sendAdd);buffEnc[2]=lowByte(sendAdd); //sender address
  byte *buffMess=&buffEnc[3];            //message segment
  memcpy(buffMess,mess,lmess);           //fill with message
  if (SX.encryptBuff(buffEnc,nbk)==NULL) Serial.println("Errore!!!");
  int ret=sendMess(buff,lenBuff);
  free(buff);
  return ret;
}

/* As previous function but using predefined network address and device address
*  range. So, devAdd is the subaddress of device inside the network dominion and
*  sendSubAdd is the subaddress of sender.
*/
int LORA::sendNetMess(unsigned int devSubAdd, unsigned int sendSubAdd, byte *mess, int lmess)
{
  unsigned int destAdd=netAddress|devSubAdd;
  unsigned int sendAdd=netAddress|sendSubAdd;
  return sendMess(destAdd,sendAdd,mess,lmess);
}
/*********** Receiving **********/

/* Set shield in continuous receiving mode. Use dataRead function to 
   to verify if data is arrived */
void LORA::receiveMessMode()
{
  SX.setState(STDBY);
  SX.setState(FSRX);
  SX.clearAllLoraFlag();
  SX.setState(RXCONT);
}

/* Receive incoming message into the buffer buff
*  If no message is incoming, return 0.
*  If message is incoming, then compare addressee (that is plain) with destAdd. 
*  If addressee is not correct return 0. (I.E is not a message for the receiver)
*  But if addressee is 0, it means that is a broadcat message and no control is 
*  made and message is accepted.  
*  Then decode message and compare sender with sendAdd. If sender is not correct
*  return -1. But if sendAdd parameter is 0 messages from anybody are accepted.
*  Finally, return the length of the sole message (without addresses).
*  The message pointer can be obtained by getMess() function.
*  The sender address can be obtained by getSender() function. 
* 
*/
int LORA::receiveMess(unsigned int destAdd, unsigned int sendAdd, byte *buff, byte maxlen )
{
  int len=0;
  if ((len=dataRead(buff,maxlen))==0) return 0;

  if (destAdd!=0)
  {
   if (buff[0]!=highByte(destAdd)) return 0;
   if (buff[1]!=lowByte(destAdd)) return 0;
  }
  decodeMess(buff,len);
  if (sendAdd!=0) {if (senderAddress!=sendAdd) return -1;}
  return len-5;
}

void LORA::decodeMess(byte *buff,int len)
{
  int lenEnc=len-2;
  byte *buffEnc=&buff[2];
  int nbk=int(ceil((float)lenEnc/16));
  
  SX.decryptBuff(buffEnc,nbk);
  
  marker=buffEnc[0];
  senderAddress=word(buffEnc[1],buffEnc[2]);
  receivedMessage=&buffEnc[3];
  receivedMessLen=lenEnc-3;  
}

/* As previous function but using predefined network address and device address
*  range. So, devAdd is the subaddress of device inside the network dominion and
*  sendSubAdd is the subaddress of sender.
*/
int LORA::receiveNetMess(unsigned int devSubAdd, unsigned int sendSubAdd, byte *buff, byte maxlen )
{
  int len=0;
  if ((len=dataRead(buff,maxlen))==0) return 0;
  unsigned int add=word(buff[0],buff[1]);
  if ((add&netmask) != netAddress) return 0;
  unsigned int dest=add&mask;
  if (dest!=0) {if (dest!=devSubAdd) return 0;} 
  decodeMess(buff,len);
  unsigned int senderNet=senderAddress & netmask;
  if (senderNet!=netAddress) return -2; 
  subNetSenderAddress=senderAddress & mask;
  if (sendSubAdd!=0) {if (subNetSenderAddress!=sendSubAdd) return -1;}
  return receivedMessLen;
}

/* Get sender address of last message received */
unsigned int LORA::getSender() {return senderAddress;}
/* Get sender sub-address in case of network address system */
unsigned int LORA::getNetSender() {return subNetSenderAddress;}
/* Get the clean message buffer (without any other prefix) of last message 
   received */ 
byte* LORA::getMessage(){return receivedMessage;} 

byte LORA::getMarker(){return marker;}

/********* Utility **********/

/* For convenience. Equivalent to SX.setPower 
*  cod:  1 (7dBm=5mW), 2 (10dBm=10mW), 3 (13dBm=20mW), 4 (17dBm=50mW), 
*        5 (20dBm=100mW)   
*/ 
  void LORA::setPower(int cod){SX.setPower(cod);}
  
/* For convenience. Equivalent to SX.setFreq 
*  The parameter freq is in MHz Ex.: 433.93 (default 434.0)
*/ 
  void LORA::setFrequency(float freq){SX.setFreq(freq);}  
  
/* For convenience. Like SX.setState(state).
*  If yes==true radio module goes in lowest power state.
*  If yes==false radio module goes in standard standby state; transmitting and
*  receiving starts from this state.
*/ 
  void LORA::setSleepState(boolean yes)
  {
    if (yes) SX.setState(SLEEP);
    else SX.setState(STDBY);
  }


/***************** Basic function (no crypto) **************************/

/* Start shield in LoRa mode without crypto*/
bool LORA::begin()
{
  if (!SX.begin()) return false;
  delay(1);
  setModeLora();
  return true;
}

/* Change shield in LoRa mode, if it was already started in different mode */
void LORA::setModeLora()
{
  SX.setState(SLEEP);
  SX.startModeLORA();
  SX.SPIwrite(0x06,0x6C);SX.SPIwrite(0x07,0x80); //default freq. 
  setConfig(11,7,4);   //low speed, max redundancy 
  SX.setLoraLowDataRateOptimize(true); //optimize for this low speed 
  SX.SPIwrite(0x0A,0x08); //ramp 50uS
  SX.setState(STDBY);
}

/* Send message (packet) mlen long (or null terminated string).
   Return 0 if ok (sent) or -1 if problem (not sent) */
int LORA::sendMess(char mess[])
{int mlen=strlen(mess); return sendMess((byte*)mess,mlen);}


int LORA::sendMess(byte mess[],byte mlen)
{
  SX.setState(STDBY);
  SX.setState(FSTX);
  delayMicroseconds(100);  
  SX.setLoraDataToSend(mess,mlen);
  SX.clearLoraFlag(TxDone);
  SX.setState(TX);
  delayMicroseconds(100);  
  int i;
  int n=((mlen+SX.getLoraPreambleLen())*20)/SX.getSRate()+40;
//  Serial.println(n);
  for(i=0;i<n;i++) {if (SX.getLoraFlag(TxDone)) return 0; else delay(100);}
  SX.setState(STDBY);
  return -1;
}

/*** continuous receiving mode ***/

/* Data arrived ? If yes, data are copied into mess buffer and function 
   returns number of bytes and mess is a null terminated string.
   If not, function returns 0 (mess = 0 length string)*/
int LORA::dataRead(char mess[],byte maxlen)
{int nc=dataRead((byte*)mess,maxlen);mess[nc]=0;return nc;}

/* Buff is a byte array and is not null terminated */
int LORA::dataRead(byte buff[],byte blen)
{
  if (!SX.getLoraFlag(RxDone)) return 0;
  SX.clearAllLoraFlag();
  if (SX.getLoraFlag(PayloadCrcError)) {SX.discardLoraRx();return -2;}
  return SX.readLoraData(buff,blen);
}

/*** single packet receiving mode ***/

/* CAD monitor for sec seconds and receive message if coming. It returns 0 or
   message length (or -1 if header invalid or -2 il CRV invalid)*/
int LORA::waitForMess(char mess[],byte mlen, float sec)
{int nc=waitForMess((byte*)mess,mlen,sec);mess[nc]=0;return nc;}

int LORA::waitForMess(byte buff[],byte blen, float sec)
{
  if (!CADmonitor(sec)) return 0;
  SX.setLoraRxByteTout(300);
  SX.clearAllLoraFlag();
  SX.setState(FSRX);
  SX.setState(RXSING);
  int fend=-1;
  int i;
  for (i=0;i<100;i++)
  {if ((fend=SX.getLoraRxEndFlag())==0) delay(100); else break;}
  SX.setState(STDBY);
  if (fend<=0) return -1; 
  if (SX.getLoraFlag(PayloadCrcError)) {SX.discardLoraRx();return -2;}
  return SX.readLoraData(buff,blen);
}

/* Monitor channel for sec seconds. Return true if preamble is detected */
bool LORA::CADmonitor(float sec)
{
  SX.setState(STDBY);
  SX.clearAllLoraFlag();
  bool f=false;
  unsigned int n=int(sec*1000);
  unsigned int i;
  for (i=0;i<n;i++) 
   {
     SX.setState(CAD);while (!SX.getLoraFlag(CadDone)) delay(1);
     if (SX.getLoraFlag(CadDetected)) {f=true;break;} 
   }
  return f; 
}

/* Set timeout (in milliseconds) for each listen period. (Def.: 100)*/ 
void LORA::setRxTimeout(int tmillis)
{SX.setLoraRxTimeout(tmillis);}

int LORA::getRxTimeout()
{return SX.getLoraRxTimeout();}

/*Configuration 
  Spreading Factor sprf: 6 to 12 (def.: 7) 
  Bandwidth bw: 0 to 9 (def.: 7 (125kHz))
  Coding Rate cr: 1 to 4 (def.: 1)
*/   
void LORA::setConfig(byte sprf, byte bw, byte cr)
{
  SX.setLoraSprFactor(sprf);
  SX.setLoraBw(bw);
  SX.setLoraCr(cr);
}

/* Set on/off automatic payload CRC computation/detection  (def.: off)*/
void LORA::setPayloadCRC(byte yesno)
{
  SX. setLoraCrc(yesno);
}

