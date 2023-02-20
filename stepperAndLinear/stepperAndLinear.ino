
#include <Arduino.h>
#include "BasicStepperDriver.h"

// Define stepper motors properties
#define MOTOR_STEPS 200
#define RPM 40
#define RPM_L 450
#define SLEEP_L 15
#define SLEEP 2
#define MICROSTEPS_L 1
#define MICROSTEPS 1
#define DIR_L 13
#define STEP_L 14
#define DIR 0
#define STEP 1

//Defining the teensy pins for control using Bonsai
#define ALUM 11
#define ATT 20
#define NON 19
#define LFWD 17
#define LBCK 22
#define CATCH_FWD 18


//Define the teensy pins to report back to Bonsai
#define L_ST 3
#define L_CATCH 5
#define R_ALUM 8
#define R_MUT 7
#define R_NON 6
#define R_OBJ 9
#define R_reset 22

// Create interrupt pin
const byte interruptPin = 12;

// intilaize motor objects
BasicStepperDriver linnear(MOTOR_STEPS, DIR_L, STEP_L,SLEEP_L);
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP,SLEEP);

// initilaize variables for computation of positions
int whiskPos; // create variable for the linear movement to whisker position
int stepperAngle = 0; // create variable for the object stepper angle
int catch_dis;
int myrand1; // first rand number generator for rotary stepper 
int myrand2; // second rand number generator for rotary stepper 
int myrandmove = myrand1+myrand2 ; // sum movement of myrand1+myrand2
int randangle; // rand option between 2 stimulus.
int randopt; // switch case for stimulus.
// define the position of the objects on the wheel
int al_1 = 0 * 33;
int al_2 = 3 * 33;
int at_1 = 2 * 33;
int at_2 = 4 * 33;  
int no_1 = 1 * 33;
int no_2 = 5 * 33;



void setup() {
    

    //////////////////////
    linnear.begin(RPM_L, MICROSTEPS_L);
    linnear.setEnableActiveState(LOW);
    stepper.begin(RPM, MICROSTEPS);
    stepper.setEnableActiveState(LOW);
    stepper.enable();
    Serial.begin(115200);
    pinMode(interruptPin, INPUT_PULLDOWN);
    pinMode(ALUM,INPUT_PULLDOWN);
    pinMode(ATT,INPUT_PULLDOWN);
    pinMode(NON,INPUT_PULLDOWN);
    
    pinMode(3,OUTPUT);
    randomSeed(analogRead(0));
   // while (! Serial); // Wait until Serial is ready - Leonardo
   
   attachInterrupt(digitalPinToInterrupt(interruptPin), advancemotor, RISING);
   Serial.println("l - aluminum foil m - muted n - non");
//   
  linnear.enable();
   linnear.rotate(-36000);
   
   linnear.disable();
   catch_dis = 10 * 360;   

}

void loop() {

 
    // generate random numbers to rotate motor
 myrand1  = random(20,180) * random(-1,2); 
 myrand2  = random(20,180) * random(-1,2);
 myrandmove = myrand1+myrand2;
 // genrate random option from 2 identical stimulus.
      randangle = random(1,3);
      int tempAngle;  

// determine the position of the linear motor at ehiskers
  int mm = 0;
  char jj;
  int rot;
 char ch;
  int bt;
  int temp_catch;
  

     if (Serial.available())
      {   
        jj = Serial.peek();
        if(isDigit(jj) || jj == '-'){
      
          linnear.enable();
          mm = Serial.parseInt();
          Serial.println(mm);
          
          bt = mm * 360;
          linnear.rotate(bt);
          whiskPos += bt;
          Serial.println(whiskPos);
          linnear.disable();
        }
        else if(jj=='r')
        {
          String rr = Serial.readString();
          rr.remove(0,1);
          Serial.print("Rotating stepper  ");
          Serial.println(rr);
          rot = rr.toInt();
          stepper.move(rot);
          stepperAngle = 0;
        }
        else if(jj == 'c') // determine catch trial distance
        {
          String catchD = Serial.readString();
          catchD.remove(0,1); 
          Serial.print("catch distance");
          Serial.println(catchD);
          catch_dis = catchD.toInt()*360;
        }
        else
        {
          ch = Serial.read(); 
          Serial.println("print ch");
          Serial.println(ch);
        }
      
      }
    

      
//    Send all teensy controlled pins to LOW so you make sure they are ready to give commands  
      digitalWrite(2,LOW);
      digitalWrite(ALUM,LOW);
      digitalWrite(ATT,LOW);
      digitalWrite(NON,LOW);
      digitalWrite(interruptPin,LOW);
      digitalWrite(R_ALUM,LOW);
      digitalWrite(R_MUT,LOW);
      digitalWrite(R_NON,LOW);
      digitalWrite(R_OBJ,LOW);
	    digitalWrite(L_ST,LOW);
	    digitalWrite(LFWD,LOW);
      digitalWrite(L_CATCH,LOW);
      digitalWrite(R_reset,LOW);
      
    
      
       

           
        if (ch == 'l' || digitalRead(ALUM) == HIGH)
        {
           
          switch (randangle) {
          case 1:
          tempAngle = al_1;
          break;
          case 2:
          tempAngle = al_2;
          break;
          }

          stepper.move(-stepperAngle);
          delay(75);
          stepper.move(myrand1*MICROSTEPS);
          delay(75);
          stepper.move(myrand2*MICROSTEPS);
          delay(75);
          stepper.move((-myrandmove*MICROSTEPS)+(tempAngle*MICROSTEPS));       
          stepperAngle = tempAngle*MICROSTEPS;
          Serial.println("aluminum");
          

          delay(100);
          digitalWrite(R_OBJ,HIGH);
          delay(100);
          digitalWrite(R_ALUM,HIGH);
          delay(100);
          digitalWrite(ALUM,LOW);
        }
    
    
        if (ch == 'm'|| digitalRead(ATT) == HIGH)
        {
          
          switch (randangle) {
          case 2 :
          tempAngle = at_1;
          break;
          case 1 :
          tempAngle = at_2;
          break;
          }
          stepper.move(-stepperAngle);
          delay(75);
          stepper.move(myrand1*MICROSTEPS);
          delay(75);
          stepper.move(myrand2*MICROSTEPS);
          delay(75);
          stepper.move((-myrandmove*MICROSTEPS)+(tempAngle*MICROSTEPS));   
          stepperAngle = tempAngle*MICROSTEPS;
          Serial.println("aluminum silenced");
          delay(100);
          digitalWrite(R_OBJ,HIGH);
          delay(100);
          digitalWrite(R_MUT,HIGH);
          delay(100);
          digitalWrite(ATT,LOW);
         
          
        }
    
    
    
        if (ch == 'n' || digitalRead(NON) == HIGH)
        {
            
          switch (randangle) {
          case 1:
          tempAngle = no_1;
          break;
          case 2:
          tempAngle = no_2;
          break;
          }
         
          stepper.move(-stepperAngle);
          delay(75);
          stepper.move(myrand1*MICROSTEPS);
          delay(75);
          stepper.move(myrand2*MICROSTEPS);
          delay(75);
          stepper.move((-myrandmove*MICROSTEPS)+(tempAngle*MICROSTEPS));   
          stepperAngle = tempAngle;
          Serial.println("non");
          delay(100);
          digitalWrite(R_OBJ,HIGH);
          delay(100);
          digitalWrite(R_NON,HIGH);
          delay(100);
          digitalWrite(NON,LOW);
        
         
          
          
        }
        if (digitalRead(LFWD) == HIGH || ch == 'f')
		{
			linnear.enable();
      delay(50);
			linnear.rotate(whiskPos);
		//	whiskPos = -whiskPos;
      delay(50);
			linnear.disable();
      Serial.println("motor moved");
      delay(100);
			digitalWrite(L_ST,HIGH);
			delay(100);
			
		}
     if (digitalRead(CATCH_FWD) == HIGH || ch == '.')
		{
			linnear.enable();
      temp_catch = whiskPos - catch_dis;
			linnear.rotate(temp_catch);
     // catch_dis = - catch_dis;
		//	whiskPos = -whiskPos;
			linnear.disable();
      delay(100);
			digitalWrite(L_CATCH,HIGH);
			delay(100);
			
		}

    if (ch == 'b')
		{
			linnear.enable();
      
			linnear.rotate(-36000);
      
			linnear.disable();
     // delay(20);
			//digitalWrite(R_reset,HIGH);
			//delay(20);
			
		}
      
}
      




void advancemotor() {
  
  linnear.startBrake();
  linnear.rotate(180);
  Serial.println("stopped");
  delay(20);
	digitalWrite(R_reset,HIGH);
	delay(20);
  
  
  
  
}
