#include <Stepper.h>
#include <Servo.h>
#define SPEAKER 2

//-------------------- Stepped initia ----------------
const int stepsPerRevolution = 300;  // change this to fit the number of steps per revolution
                 
Stepper myStepper(stepsPerRevolution, 12,13);     



// give the motor control pins names:
const int pwmA = 3;
const int pwmB = 11;
const int brakeA = 9;
const int brakeB = 8;
const int dirA = 12;
const int dirB = 13;
  int x = 0;

//----------------------- servo initia ----------------

Servo myservo;
int pos = 0;

//-------------------- Speaker initia ----------------
int BassTab[]={1802, 1802, 1802, 2211, 1516, 1802 ,2211, 1516, 1802 ,2211, 1516, 1802 , 
               1200, 1200, 1200, 900,  1516, 1802, 2211, 1516, 1802 ,2211, 1516, 1802};


void setup() {
  
//------------------stepped setup----------------------  
  Serial.begin(9600);
  // set the PWM and brake pins so that the direction pins  // can be used to control the motor:
  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(brakeA, OUTPUT);
  pinMode(brakeB, OUTPUT);
  digitalWrite(pwmA, HIGH);
  digitalWrite(pwmB, HIGH);
  digitalWrite(brakeA, LOW);
  digitalWrite(brakeB, LOW);
  
  // initialize the serial port:
  Serial.begin(9600);
  
  // set the motor speed (for multiple steps only):
  myStepper.setSpeed(40);




//---------------------servo setup-------------------------
  pinMode(SPEAKER,OUTPUT);
  digitalWrite(SPEAKER,LOW);
  
}


void loop() {


  
//---------------------sound loop------------------------- 

for(int note_index=0;note_index< 12 ;note_index++)
    {
      if(note_index == 4 || note_index == 7 || note_index == 10){

          sound(note_index);
          delay(50);
       }
      else if(note_index == 3 || note_index == 6 || note_index == 9){

          sound(note_index);
          delay(250);
       }
     else {
      sound(note_index);
      delay(500);
      }
    }
//---------------------servo setup-------------------------
 
    myservo.attach(9);   
    myservo.write(180);              
    delay(2000);   
    myservo.detach(); 
     
//------------------stepped loop---------------------- 
  myStepper.step(stepsPerRevolution);
  delay(3000);

  myStepper.step(-stepsPerRevolution);

  //---------------motion during light on--------------

  myservo.attach(9);   
  myservo.write(180);              
  delay(3000);      // 30 sec motion
  myservo.detach();  
  
  delay(400);  //4 min delay

  myservo.attach(9);   
  myservo.write(180);              
  delay(3000);   // 30 sec motion
  myservo.detach(); 

  delay(400);  // 4 min delay
  
    //-------------------lights off---------------
}

void sound(uint8_t note_index)
{
  for(int i=0;i<100;i++)   
  {
    digitalWrite(SPEAKER,HIGH);
    delayMicroseconds(BassTab[note_index]);
    digitalWrite(SPEAKER,LOW);
    delayMicroseconds(BassTab[note_index]);
  }
}

