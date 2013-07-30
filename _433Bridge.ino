#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>

#define BLINKDELAY 5
#define SYSTEMCODE 31
#define TRANSMITTER P1_4
#define RECEIVER P1_3

ElroTransmitter elroTransmitter(TRANSMITTER);
BlokkerTransmitter blokkerTransmitter(TRANSMITTER);
ActionTransmitter actionTransmitter(TRANSMITTER);

//display help
String inputString = "h";  
boolean stringComplete = true;


void p(String input){
  Serial.println(input);
}



void setup()
{
  Serial.begin(9600);
  Serial.println("started");
  inputString.reserve(20);
  // put your setup code here, to run once:M
   pinMode(RECEIVER, INPUT);
   
   pinMode(TRANSMITTER, OUTPUT);
   digitalWrite(TRANSMITTER, LOW);
   
   
   RemoteReceiver::init(RECEIVER, 3, translateCode);
   pinMode(RED_LED, OUTPUT);
   digitalWrite(RED_LED, LOW);
 
   blink();  
 }

int count =0;
volatile bool FLAG = false;
void loop()
{
  ProcessSerial();
  if(FLAG){
    blink(2);
    FLAG=false;
   }
  
  if(stringComplete){
    blink(1);
    RemoteReceiver::disable();
    boolean onOf = false;
    char address = 0;
    char device = 0;
    char group = 0;
    
    switch(inputString[0]){
      case 'h':
        p("usage:");
        p("action:  a|0..31 (char)sys|A-E (charCap)dev|0-1(on/of)");
        p("blokker: b|1-8 (char)|0-1 onof(char)");
        p("Elro:    e|0..31(char)|A-D CAP||0-1(on/of");
        p("KAKU:    k|A-P (char)add|1..4(char)group|1..4 (char)dev) |0-1 onof|");
        p("KAKU:    k|A-P (char)add|1..4 (char)dev) |0-1 onof|");
        p("Test:    t\\r");
        p("");
        p("eg: a31B1 -> put 31 as char 31 (one byte)");    
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
    RemoteReceiver::enable();
    stringComplete = false;
  
  }
}

void ProcessSerial() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read(); 
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}

  
void translateCode(unsigned long receivedCode, unsigned int period){
   interrupts();
   FLAG=true;
   RemoteReceiver::disable();
 /* 
  Serial.print("Code: ");
  Serial.print(receivedCode, HEX);
  Serial.flush();
  Serial.print(", period duration: ");
  Serial.print(period);
  Serial.println("us.");
  */
 // brute(receivedCode);
 // RemoteTransmitter::sendCode(P1_4, receivedCode, period, 3);
  for (char dev='A'; dev<='D'; dev++)
  {
    for(char b=0; b <= 1; b++)
    {
     unsigned long test = elroTransmitter.getTelegram(SYSTEMCODE, dev, b);     
     if (RemoteTransmitter::isSameCode(test, receivedCode))
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
   
 RemoteReceiver::enable();
}

//BruteForce the possible command:
void brute(long input){
  for (char address=B00000; address<=B11111;address++){
    for (int dev=1; dev<=4;dev++){
      for (int b=0; b<=1;b++){
        long elro = elroTransmitter.getTelegram(dev,address, b);
   
        if(RemoteTransmitter::isSameCode(elro, input))
        {
          Serial.println("found elro");
        }
   
      }
    }
  }
  
    
  Serial.println("no match found");
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
