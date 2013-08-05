#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>
//#include <VirtualWire.h>

#include <SpaceLen.h>


#define BLINKDELAY 5
#define SYSTEMCODE 31
#define TRANSMITTER P1_5
#define RECEIVER P1_4
 
const char *HELP = "action:  a|0..31 (char)sys|A-E (charCap)dev|0-1(on/of)\n blokker: b|1-8 (char)|0-1 onof(char)\n Elro:    e|0..31(char)|A-D CAP||0-1(on/of)\n KAKU:    k|A-P (char)add|1..4(char)group|1..4 (char)dev) |0-1 onof|\n Test: t\\r\n  eg: a31B1 -> put 31 as char 31 (one byte)\0";

ElroTransmitter elroTransmitter(TRANSMITTER);
BlokkerTransmitter blokkerTransmitter(TRANSMITTER);
ActionTransmitter actionTransmitter(TRANSMITTER);

//display help
String inputString = "";  
boolean stringComplete = false;



void p(String input){
  Serial.println(input);
 }

//SpaceLen stuff

volatile uint8_t data[4]= {0,0,0,0};
volatile boolean SENSORFLAG = false;

//int count =0;
volatile long lastCode = 0;
volatile long lastPeriod = 0;
volatile bool REMOTEFLAG=false;



void setup()
{
  Serial.begin(9600);
  Serial.println("started");
  inputString.reserve(20);
  
  //Serial.println(*HELP);
  
  // put your setup code here, to run once:M
   pinMode(RECEIVER, INPUT);
   
   pinMode(TRANSMITTER, OUTPUT);
   digitalWrite(TRANSMITTER, LOW);
   
   
   RemoteReceiver::init(RECEIVER, 3, translateCode);
   pinMode(RED_LED, OUTPUT);
   digitalWrite(RED_LED, LOW);
 
   blink(); 
   
   //Attach interrupts
  SpaceLen::init(RECEIVER, handleSensorData);
  attachInterrupt(RECEIVER, interruptWrapper, RISING); 
  
  //Virtual Wire Settings
  // vw_set_ptt_inverted(true); // Required for DR3100
  /*
   vw_set_ptt_pin(P2_1);
   vw_set_rx_pin(RECEIVER);
   vw_set_tx_pin(TRANSMITTER);
   vw_setup(500);	 // Bits per sec
   */
 }

void loop()
{
  processSerial();

  if(SENSORFLAG){
    renderSensorData();
    blink();
    SENSORFLAG=false;
  }
  
  if(stringComplete){
   processStringComplete();
   stringComplete = false;
   blink();
  }
  
  if (REMOTEFLAG){
   for (char dev='A'; dev<='D'; dev++)
    {
      for(char b=0; b <= 1; b++)
      {
       unsigned long test = elroTransmitter.getTelegram(SYSTEMCODE, dev, b);     
       if (RemoteTransmitter::isSameCode(test, lastCode))
       {
         Serial.print("r");
         Serial.print(dev);
         if(b == 0)
           Serial.println("0");
         else
           Serial.println("1");
         Serial.flush();
       } 
     }  
    }
    blink();
    REMOTEFLAG=false;
   }
      
    
}

void renderSensorData(){
 if ( (data[3]) == 0xF0 ) {							
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
      Serial.println(data[0],DEC);
      Serial.flush(); 
 }
}

void interruptWrapper(){
  //HACK to make print work proper
  if(stringComplete || SENSORFLAG || REMOTEFLAG)
    {
      return;
    }
  //make rising/falling work alas "CHANGE"
  uint8_t bit = digitalPinToBitMask(RECEIVER);
  P1IES ^= bit;

  SpaceLen::interruptHandler();
  RemoteReceiver::interruptHandler();
}

void handleSensorData(byte *datain) {
  SENSORFLAG = true;
  for (int x=0; x<4;x++){
    data[x] = datain[x];
  }

}

void processSerial() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read(); 
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}

void processStringComplete()
{
  interrupts();
   boolean onOf = false;
   char address = 0;
   char device = 0;
    
    
    switch(inputString[0]){
      case 'h':
        Serial.println("HELP NOT IMPLEMEMENTED YET");
       // Serial.print(HELP);
        Serial.flush();
       
        break;
       case 'i':
        Serial.println("i hit");
       break;
       
       case 'a':
         address = inputString[1];
         device = inputString[2];
         onOf = inputString[3] == 1? true : false;
         actionTransmitter.sendSignal(address, device, onOf);
         break; 
       
       case 'b':
         address = inputString[1];
         onOf = inputString[2] == 1? true : false;
         blokkerTransmitter.sendSignal(address, onOf);
         break;
         
       case 'e':
         address = inputString[1];
         device = inputString[2];
         onOf = inputString[3] == 1? true : false;
         elroTransmitter.sendSignal(address, device, onOf);
         break;  
       
       case 'k':
         p("not implemented yet :(");
         break;
       
       case 't':
         p("test toggle elro 31, B, on/of");
         p("test toggle bokker dev 1");
         elroTransmitter.sendSignal(31, 'B', true);
         blokkerTransmitter.sendSignal(1,true);
         delay(2000);
         elroTransmitter.sendSignal(31, 'B', false);
         blokkerTransmitter.sendSignal(1,false);
         break;
         
       default:
          blink(4);
          p("unknown command");
          break;
    }
    p("ok");
     //Clear state of the input and re-enable receiver
    inputString = ""; 
    stringComplete = false;
}
  
void translateCode(unsigned long receivedCode, unsigned int period){
    REMOTEFLAG=true;
    lastCode = receivedCode;
    lastPeriod = period; 
   }


void blink(int times){
  for (int i=0; i< times; i++)
  blink();
}
void blink(){
 digitalWrite(RED_LED, HIGH);
  delay(BLINKDELAY);
 digitalWrite(RED_LED,LOW); 
 delay(BLINKDELAY);
}
