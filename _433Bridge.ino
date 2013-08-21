#include <QueueList.h>

//#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>
#include <LedProtocol.h>
#include <AlarmReceiver.h>

#define MAX_MSG_SIZE 6
#define NRBYTES 4
#define DELIMITER ";;;"
#define STARTPACKET "|||"

#include <SpaceLen.h>


#define BLINKDELAY 5
#define SYSTEMCODE 31
#define TRANSMITTER P1_5
#define RECEIVER P1_4

#define HELP1  "action:  a|0..31 (char)"
#define HELP2  "sys|A-E (charCap)dev|0-1(on/of)"
#define HELP3  "blokker: b|1-8 (char)|0-1 onof(char)"
#define HELP4  "lro:    e|0..31(char)|A-D CAP||0-1(on/of)"
#define HELP5  "KAKU:    k|A-P (char)add|1..4(char)group|1..4 (char)dev) |0-1 onof|"
#define HELP6  "LEDPRO: l|(0-255)|(brightnes 0-255)"
#define HELP7  "Test: t\\r\n  eg: a31B1 -> put 31 as char 31 (one byte)"

ElroTransmitter elroTransmitter(TRANSMITTER);
BlokkerTransmitter blokkerTransmitter(TRANSMITTER);
ActionTransmitter actionTransmitter(TRANSMITTER);

//SpaceLen stuff

volatile uint8_t data[4]= {
  0,0,0,0};
volatile boolean SENSORFLAG = false;

//int count =0;
volatile long lastCode = 0;
volatile long lastPeriod = 0;
volatile bool REMOTEFLAG=false;
volatile bool SENDING= false;

volatile uint8_t pcInPointer = 0;

struct RECORD
{
  byte cmd[4];
};

QueueList <RECORD> queue;
RECORD current;

void setup()
{
  Serial.begin(9600);
  Serial.println("started");
  Serial.println(HELP1);
  Serial.flush();
  Serial.println(HELP2);
  Serial.flush();
  Serial.println(HELP3);
  Serial.flush();
  Serial.println(HELP4);
  Serial.flush();
  Serial.println(HELP5);
  Serial.flush();
  Serial.println(HELP6);
  Serial.flush();
  Serial.println(HELP7);
  Serial.println(DELIMITER);
  delay(2000);
  //inputString.reserve(10);

  pinMode(RECEIVER, INPUT);

  pinMode(TRANSMITTER, OUTPUT);
  digitalWrite(TRANSMITTER, LOW);


  //   RemoteReceiver::init(RECEIVER, 3, translateCode);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);

  blink(); 

  //Attach interrupts
  SpaceLen::init(RECEIVER, handleSensorData);
  attachInterrupt(RECEIVER, interruptWrapper, RISING); 
  AlarmReceiver::start();
  // AlarmReceiver::interrupt();
  // set the printer of the queue.
  queue.setPrinter (Serial);
}

void loop()
{
  processSerial();

  if(SENSORFLAG){
    renderSensorData();
    blink();
    SENSORFLAG=false;
  }

  if(!queue.isEmpty()){
    SENDING=true;
    processStringComplete();

    blink();
    for(int x=0;x,x<20;x++){
      delay(10);
      processSerial();
    }
    SENDING=false;
  }

  if(AlarmReceiver::dataReady()){
    REMOTEFLAG = true;
    Serial.print(STARTPACKET);
    Serial.print("a");
    Serial.print(AlarmReceiver::getData(), HEX);
    Serial.println(DELIMITER);
    Serial.flush();
    for(int x=0;x,x<40;x++){
      delay(10);
      processSerial();
    }
    blink();
    REMOTEFLAG=false;
  }

}

void renderSensorData(){
  if ( (data[3]) == 0xF0 ) {
    Serial.print(STARTPACKET);							
    int channel=((data[1]&0x30)>>4)+1;
    byte high=((data[1]&0x08)?0xf0:0)+(data[1]&0x0f);
    byte low=data[2];
    int temp10= high*256+low;
    Serial.print("c:");
    Serial.print(channel);

    Serial.flush();
    Serial.print(";t");
    //Serial.print(temp10/10);
    //Serial.print(".");
    Serial.print(temp10);
    //Serial.print(abs((temp10)%10));
    Serial.flush();

    Serial.print(";b");
    Serial.print(data[0],DEC);
    Serial.println(DELIMITER);
    Serial.flush(); 

    for(int x=0;x<100;x++){
      delay(10);
      processSerial();
    }
  }
}

void interruptWrapper(){
  //HACK to make print work proper
  if(SENSORFLAG || REMOTEFLAG || SENDING)
  {
    return;
  }
  //make rising/falling work alas "CHANGE"
  uint8_t bit = digitalPinToBitMask(RECEIVER);
  P1IES ^= bit;

  SpaceLen::interruptHandler();
  //x RemoteReceiver::interruptHandler();
  AlarmReceiver::interrupt();
}

void handleSensorData(byte *datain) {
  SENSORFLAG = true;
  for (int x=0; x<4;x++){
    data[x] = datain[x];
  }

}

void processSerial() {
  while (Serial.available() > 0) {
    byte inChar = Serial.read(); 
    if (inChar == '\n' || pcInPointer == MAX_MSG_SIZE) {
      current.cmd;
      queue.push(current);
      pcInPointer = 0;
      
    }
    else{
      current.cmd[pcInPointer] = inChar;
      pcInPointer++;
    }
  }
}

void processStringComplete()
{
  interrupts();
  boolean onOf = false;
  byte address = 0;
  byte device = 0;
  byte b[NRBYTES];// {0x00,0x00,0x00,0x00}; 
  RECORD t;
  
  //byte record[NRBYTES];
  RECORD record = queue.pop();
  switch(record.cmd[0]){
  case 'h':
    Serial.println("RESET TO VIEW HELP!");
    // Serial.print(HELP);
    Serial.flush();
    break;
  case 'i':
    Serial.println("i hit");
    break;

  case 'a':
    address = record.cmd[1];
    device = record.cmd[2];
    onOf = record.cmd[3] == 1? true : false;
    actionTransmitter.sendSignal(address, device, onOf);
    break; 

  case 'b':
    address = record.cmd[1];
    onOf = record.cmd[2] == 1? true : false;
    blokkerTransmitter.sendSignal(address, onOf);
    break;

  case 'e':
    address = record.cmd[1];
    device = record.cmd[2];
    onOf = record.cmd[3] == 1? true : false;
    elroTransmitter.sendSignal(address, device, onOf);
    break;  

  case 'k':
    Serial.print("not implemented yet :(");
    break;

  case 'T':
    Serial.println("TurnOffledstrip 0x01f");
    t.cmd[0] = 'l';
    t.cmd[1] =  0x01;//addr
    t.cmd[2] =  0x00;//r
    t.cmd[3] =  0x00;//g
    t.cmd[4] = 0x00;//b    

    queue.push(t);
    break;

  case 't':
    Serial.println("test toggle elro 31, B, on/of");
    Serial.println("test toggle bokker dev 1");
    Serial.println("ledriver add 0x01 0xff0000");
    t.cmd[0] = 'l';
    t.cmd[1] =  0x01;//addr
    t.cmd[2] =  0xFF;//r
    t.cmd[3] =  0x00;//g
    t.cmd[4] = 0x00;//b    

     delay(100);
      elroTransmitter.sendSignal(31, 'B', true);
      blokkerTransmitter.sendSignal(1,true);
      delay(2000);
    
    //  LedProtocol::sendPackage(TRANSMITTER, b, NRBYTES);
    //  delay(100);
      elroTransmitter.sendSignal(31, 'B', false);
      blokkerTransmitter.sendSignal(1,false);
    queue.push(t);
    break;

    case('l'):
    //Serial.print("setting led ");
    b[0] = record.cmd[1]; //add
    b[1] = record.cmd[2]; //r
    b[2] = record.cmd[3]; //g
    b[3] = record.cmd[4]; //b
    LedProtocol::sendPackage(TRANSMITTER, b, NRBYTES);
    delay(50);
    LedProtocol::sendPackage(TRANSMITTER, b, NRBYTES);
    delay(50);
    LedProtocol::sendPackage(TRANSMITTER, b, NRBYTES);
    break;

    case('L'):
    Serial.println("FixLed code. send to 0x01");
    b[0] = 0x01; //add
    b[1] = 0xFF; //r
    b[2] = 0x00; //g
    b[3] = 0x00; //b
    LedProtocol::sendPackage(TRANSMITTER, b, NRBYTES);
    break;

  default:
    blink(1);
    Serial.print("unknown command: " + record.cmd[0]);
    Serial.println(DELIMITER);
    break;
  }
  //Serial.print("ok");
  // Serial.println(DELIMITER);
}
/*
void translateCode(unsigned long receivedCode, unsigned int period){
 REMOTEFLAG=true;
 lastCode = receivedCode;
 lastPeriod = period; 
 }
 */

void blink(int times){
  for (int i=0; i< times; i++)
    blink();
}
void blink(){
  digitalWrite(RED_LED, HIGH);
  delay(BLINKDELAY);
  processSerial();
  digitalWrite(RED_LED,LOW); 
  delay(BLINKDELAY);
}


