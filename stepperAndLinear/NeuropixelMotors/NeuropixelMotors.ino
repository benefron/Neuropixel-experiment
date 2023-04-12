
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



//Define the teensy pins to report back to Bonsai
#define L_ST 3
#define L_CATCH 5
#define R_ALUM 8
#define R_MUT 7
#define R_NON 6
#define R_OBJ 9
#define R_reset 22
#define Obj4 18

// Create interrupt pin
const byte interruptPin = 12;

// intilaize motor objects
BasicStepperDriver linnear(MOTOR_STEPS, DIR_L, STEP_L,SLEEP_L);
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP,SLEEP);

// initilaize variables for computation of positions
int whiskPos; // create variable for the linear movement to whisker position
int stepperAngle = 0; // create variable for the object stepper angle
// define the position of the objects on the wheel
int al_1 = 3 * 33;
int at_1 = 2 * 33;
int no_1 = 1 * 33;
int obj_4 = 0 * 33;



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

}

void loop() {

 

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
        else
        {
          ch = Serial.read();
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
      digitalWrite(R_reset,LOW);
      
    
      
       

           
        if (ch == 'l' || digitalRead(ALUM) == HIGH)
        {
           


          stepper.move(-stepperAngle);
          delay(75);
          stepper.move(al_1*MICROSTEPS);       
          stepperAngle = al_1*MICROSTEPS;
          Serial.println("aluminum");
          
        }
    
    
        if (ch == 'm'|| ch == 's')
        {

          stepper.move(-stepperAngle);
          delay(100);
          stepper.move(at_1*MICROSTEPS);   
          stepperAngle = at_1*MICROSTEPS;
          Serial.println("aluminum silenced");
          delay(100);

          
        }
    
    
    
        if (ch == 'n' || ch == 'e')
        {
            
         
          stepper.move(-stepperAngle);
          delay(100);
          stepper.move(no_1*MICROSTEPS);   
          stepperAngle = no_1*MICROSTEPS;
          Serial.println("non");

          
        }
        if (ch == 'o')
        {
            
         
          stepper.move(-stepperAngle);
          delay(100);
          stepper.move(obj_4*MICROSTEPS);   
          stepperAngle = obj_4*MICROSTEPS;
          Serial.println("obj4");
          delay(100);
          

        }
        if (digitalRead(LFWD) == HIGH || ch == 'f')
		{
			linnear.enable();
      delay(50);
			linnear.rotate(whiskPos);
      delay(50);
			linnear.disable();
      Serial.println("at Whiskers");
      
			
		}

    if (ch == 'b')
		{
			linnear.enable();
      Serial.println("Moving back");
      
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
  Serial.println("reset");
  delay(20);
	digitalWrite(R_reset,HIGH);
	delay(20);
  
  
  
  
}
