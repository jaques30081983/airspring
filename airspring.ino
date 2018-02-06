/*********************************************************************

bigwood air spring

written by Jakob Laemmle

*********************************************************************/

#include <Wire.h>
#include <SPI.h>
//Oled Display
#include <U8glib.h>
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);    // I2C / TWI



// constants won't change. They're used here to
// set pin numbers:
const int buttonUpPin = 2;     // the number of the pushbutton pin
const int buttonDownPin = 3;     // the number of the pushbutton pin
const int buttonTogglePin = 4;     // the number of the switch pin
const int switchHandBrakePin = 5;     // the number of the handbrake pin

const int relayUp = 6;     // the number of the relay for leveling up pin
const int relayDown = 7;     // the number of the relay for leveling down pin
const int relaySignal = 8;     // the number of the relay for the signal light in the dashboard pin
const int relaySpare = 9;     // the number of the spare relay pin

const int sensorPin = A0;    // select the input pin for the leveling sensor

const int drivingLevel = 730;    // driving level of the air spring (730 / 480)
const int maxLowLevel = 550; //max down level of the air spring (550 / 360)
const int maxHighLevel = 860; // max up level of the air spring (860 / 560)

unsigned long maxTimeDown = 55000; // Max time for leveling down from driving level 55sec
unsigned long maxTimeUp = 50000; // Max time for leveling up from driving level 50sec

unsigned long maxTimeDownFromHigh = 60000; // Max time for leveling down from high level 60sec
unsigned long maxTimeUpFromLow = 37000; // Max time for leveling up from low level to driving level 37sec

// variables will change:
int buttonUpState = 0;         // variable for reading the pushbutton status
int buttonDownState = 0;         // variable for reading the pushbutton status
int buttonToggleState = 0;         // variable for reading the switch status
int switchHandBrakeState = 0;         // variable for reading the handbrake status

int sensorValue = 0;  // variable to store the value coming from the leveling sensor
int sensorValueOverTime =0;
int sensorValueSmooth = drivingLevel;

unsigned long firstTime = 0;
unsigned long levelTime = 0;
int xtime = 0;

int levelStartPoint = 0;
int levelDirection = 0;
int levelUpState = 0;
int levelDownState = 0;

String text1 = "";
String text2 = "";
String text3 = "";

void setup()   {
  // init ###################
  Serial.begin(9600);

  // Initialise the Arduino data pins for OUTPUT
  pinMode(relayUp, OUTPUT);
  pinMode(relayDown, OUTPUT);
  pinMode(relaySignal, OUTPUT);
  pinMode(relaySpare, OUTPUT);

  // switch Relays off
  digitalWrite(relayUp,HIGH);
  digitalWrite(relayDown,HIGH);
  digitalWrite(relaySpare,HIGH);

  //Oled init
  u8g.setFont(u8g_font_unifont);
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.

  delay(2000);
  // switch the signal later off for indication in the dashboard
  digitalWrite(relaySignal,HIGH);

}

void draw(void) {
  // draw level and status ###################
  // graphic commands to redraw the complete screen should be placed here
  u8g.setFont(u8g_font_8x13B);
  u8g.setColorIndex(1);

  // levelmeter
  u8g.drawFrame(2,3,20,59); // Level Frame
  
  u8g.drawFrame(0,19,24,10); //Good level frame
  u8g.drawBox(4,(56-((sensorValue-maxLowLevel)*0.18)),16,5);

  // text
  u8g.setPrintPos(30, 20);
  u8g.print(text1);

  u8g.setPrintPos(30, 40);
  u8g.print(text2);

  u8g.setPrintPos(30, 60);
  u8g.print(text3);
}

void loop() {
  // picture loop ###################
  u8g.firstPage();
  do {
  draw();
  } while( u8g.nextPage() );

  // read the state of the pushbutton value:
  buttonUpState = digitalRead(buttonUpPin);
  buttonDownState = digitalRead(buttonDownPin);
  buttonToggleState = digitalRead(buttonTogglePin);
  switchHandBrakeState = digitalRead(switchHandBrakePin);

  // set state of up down buttons
  if(buttonUpState == HIGH and buttonToggleState == LOW){
    levelUpState = levelUpState + 1;

  }else if(buttonDownState == HIGH and buttonToggleState == LOW){
    levelDownState = levelDownState + 1;

  }

  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  Serial.print(sensorValue);
  Serial.print("\n");

  //Normal Operation
  if(buttonToggleState == LOW and switchHandBrakeState == HIGH and levelUpState == 0 and levelDownState == 0){
  //Auto leveling ###################
  text1 = "Auto";
  text3 = sensorValue;

  int minRange = drivingLevel - 15;
  int maxRange = drivingLevel + 15;

  // set time for later
  if(firstTime == 0){
    firstTime = millis();
  }else{
    // smooth sensor value over time
    if((millis() - firstTime) < 10000){
      sensorValueOverTime = sensorValueOverTime + sensorValue;
      xtime = xtime +1;
    }else{
      sensorValueSmooth = sensorValueOverTime / xtime;
      Serial.print("S");
      Serial.print(sensorValueSmooth);
      Serial.print(" ");
      Serial.print(xtime);
      Serial.print("\n");

      firstTime = millis();
      xtime = 0;
      sensorValueOverTime = 0;
    }
  }

  //Operate Auto leveling
  if(sensorValueSmooth > maxRange or sensorValueSmooth < minRange){
    if (sensorValue > maxRange){
      //Auto Down
      digitalWrite(relayDown,LOW);
      digitalWrite(relayUp,HIGH);
      sensorValueSmooth = sensorValue;
      text2 = "DOWN AL";
      levelDirection = 0;

      //Set start level time
      if(levelTime == 0){
        levelTime = millis();
        levelStartPoint = sensorValue;
      }

    }else if(sensorValue < minRange){
      //Auto Up
      digitalWrite(relayUp,LOW);
      digitalWrite(relayDown,HIGH);
      sensorValueSmooth = sensorValue;
      text2 = "UP AL";
      levelDirection = 1;

      //Set start level time
      if(levelTime == 0){
        levelTime = millis();
      }

    }else{
      //Level OK
      delay(2000);
      digitalWrite(relayUp,HIGH);
      digitalWrite(relayDown,HIGH);
      sensorValueSmooth = drivingLevel;
      levelTime = 0;

    }

  // Check if its running over time for security
    if(levelStartPoint < drivingLevel){
      // Under driving level
    if((millis()-levelTime) > maxTimeUpFromLow){
      text1 = "ALERT!";
      text2 = "STOP?";
      digitalWrite(relaySignal,LOW);
    }else{
      digitalWrite(relaySignal,HIGH);
    }
    }else if(levelStartPoint > drivingLevel) {
    // Over driving level
      if((millis()-levelTime) > maxTimeDownFromHigh){
      text1 = "ALERT!";
      text2 = "STOP?";
      digitalWrite(relaySignal,LOW);
      }else{
        digitalWrite(relaySignal,HIGH);
      }
    }


  }else{
    text2 = "GOOD AL";
  }

  }else if (buttonToggleState == LOW and switchHandBrakeState == HIGH and levelUpState >= 1){
  //Auto Level Up ###################
  text1 = "Auto Up";
  text3 = sensorValue;

  if(levelStartPoint == 0){
    levelStartPoint = sensorValue;
  }

  if(levelStartPoint < drivingLevel){
  // Under driving level
    if(levelUpState == 1){
    //Auto up to driving level

      if(sensorValue < drivingLevel){
        digitalWrite(relayUp,LOW);
        digitalWrite(relayDown,HIGH);

        text2 = "UP to DL";

      }else{
        //Level OK
        delay(2000);
        digitalWrite(relayUp,HIGH);
        digitalWrite(relayDown,HIGH);
        text2 = "DL OK";
        levelUpState = 0;
      }

    }else if (levelUpState == 2){
    //Auto up to top level
      if(sensorValue < maxHighLevel){
        digitalWrite(relayUp,LOW);
        digitalWrite(relayDown,HIGH);

        text2 = "UP to TL";

      }else{
        //Level OK
        delay(1500);
        digitalWrite(relayUp,HIGH);
        digitalWrite(relayDown,HIGH);
        text2 = "TL OK";

      }

    }else if (levelUpState == 3){
    //Stop
     digitalWrite(relayUp,HIGH);
     digitalWrite(relayDown,HIGH);

     text2 = "STOP";

    }else if (levelUpState == 4){
        //Stop
         digitalWrite(relayUp,HIGH);
         digitalWrite(relayDown,HIGH);

         text1 = "Auto";
         text2 = "wait...";
         levelUpState = 0;
         levelStartPoint = 0;
    }

  }else if (levelStartPoint > drivingLevel){
  // Over driving level
    if(levelUpState == 1){
      //Auto up to top level
        if(sensorValue < maxHighLevel){
          digitalWrite(relayUp,LOW);
          digitalWrite(relayDown,HIGH);

          text2 = "UP to TL";

        }else{
          //Level OK
          delay(1500);
          digitalWrite(relayUp,HIGH);
          digitalWrite(relayDown,HIGH);
          text2 = "TL OK";

        }

      }else if (levelUpState == 2){
      //Stop
       digitalWrite(relayUp,HIGH);
       digitalWrite(relayDown,HIGH);

       text2 = "STOP";

      }else if (levelUpState == 3){
          //Stop
           digitalWrite(relayUp,HIGH);
           digitalWrite(relayDown,HIGH);

           text1 = "Auto";
           text2 = "wait...";
           levelUpState = 0;
           levelStartPoint = 0;
      }

  }


  }else if (buttonToggleState == LOW and switchHandBrakeState == HIGH and levelDownState >= 1){
  //Auto Level Down ###################
    text1 = "Auto Down";
    text3 = sensorValue;


    if(sensorValue > drivingLevel){
    // Over driving level
      if(levelDownState == 1){
      //Auto down to driving level

        if(sensorValue > drivingLevel){
          digitalWrite(relayUp,HIGH);
          digitalWrite(relayDown,LOW);

          text2 = "Down to DL";

        }else{
          //Level OK
          delay(2000);
          digitalWrite(relayUp,HIGH);
          digitalWrite(relayDown,HIGH);
          text2 = "DL OK";
          levelDownState = 0;
        }

      }else if (levelDownState == 2){
      //Auto down to low level
        if(sensorValue < maxHighLevel){
          digitalWrite(relayUp,HIGH);
          digitalWrite(relayDown,LOW);

          text2 = "Down to LL";

        }else{
          //Level OK
          delay(2000);
          digitalWrite(relayUp,HIGH);
          digitalWrite(relayDown,HIGH);
          text2 = "LL OK";

        }

      }else if (levelDownState == 3){
      //Stop
       digitalWrite(relayUp,HIGH);
       digitalWrite(relayDown,HIGH);

       text2 = "STOP";

      }else if (levelDownState == 4){
          //Stop
           digitalWrite(relayUp,HIGH);
           digitalWrite(relayDown,HIGH);

           text1 = "Auto";
           text2 = "wait...";
           levelDownState = 0;
      }

    }else if (sensorValue < drivingLevel){
    // Under driving level
      if(levelDownState == 1){
        //Auto down to low level
          if(sensorValue > maxLowLevel){
            digitalWrite(relayUp,HIGH);
            digitalWrite(relayDown,LOW);

            text2 = "Down to LL";

          }else{
            //Level OK
            delay(2000);
            digitalWrite(relayUp,HIGH);
            digitalWrite(relayDown,HIGH);
            text2 = "LL OK";

          }

        }else if (levelDownState == 2){
        //Stop
         digitalWrite(relayUp,HIGH);
         digitalWrite(relayDown,HIGH);

         text2 = "STOP";

        }else if (levelDownState == 3){
            //Stop
             digitalWrite(relayUp,HIGH);
             digitalWrite(relayDown,HIGH);

             text1 = "Auto";
             text2 = "wait...";
             levelDownState = 0;
        }

    }


  }else if(buttonToggleState == HIGH and switchHandBrakeState == HIGH){
  //Manually ###################
    sensorValue = sensorValue +110;
    text1 = "Manually";
    text3 = sensorValue;

    if(buttonUpState == HIGH){
      //Level Up
      digitalWrite(relayUp,LOW);
      text2 = "UP";
    }else if (buttonDownState == HIGH){
      //Level Down
      digitalWrite(relayDown,LOW);

      text2 = "DOWN";
    }else{
      digitalWrite(relayUp,HIGH);
      digitalWrite(relayDown,HIGH);
      text2 = "STOP";
    }


    // Alert if under or over levels
    if(sensorValue > maxHighLevel){
    digitalWrite(relaySignal,LOW);
    text2 = "Over TL!";
    }else{
    digitalWrite(relaySignal,HIGH);
    }

    if(sensorValue < maxLowLevel){
    digitalWrite(relaySignal,LOW);
    text2 = "Under LL!";
    }else{
    digitalWrite(relaySignal,HIGH);
    }

  }else{
    //Handbrake On
    text1 = "Hand";
    text2 = "brake";
    text3 = "ON!";
    digitalWrite(relayUp,HIGH);
    digitalWrite(relayDown,HIGH);


  }


  delay(200);        // delay in between reads for stability
}
