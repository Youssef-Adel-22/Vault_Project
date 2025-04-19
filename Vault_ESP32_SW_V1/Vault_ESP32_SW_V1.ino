#include "esp_system.h"
#include <ESP32Servo.h>
#include <string.h>
#define LOCK_SERVO_PIN 13
#define LED 2
Servo lockServo;
const uint KeypadRow_pin[3]= {27,14,12};
const uint KeypadCol_pin[4]= {32,33,25,26};
uint8_t Keypad_numbers[3][4]= 
{
  {'R','0','1','2'},
  {'\0','3','4','5'},
  {'6','7','8','9'}
};
char PressedKey = '\0';
char localPassword[4] ={0};
char enteredPassword[5] ={0};
byte passIndex =0;
byte attemptsIndex =10;
boolean passwordAlive= false;
byte i;
typedef enum
{
  STATE_IDEL,
  STATE_REQUSET,
  STATE_VERIVCATION,
  STATE_PAUSED,
  STATE_ERROR,
} stateMachine_t;
stateMachine_t stateMachine =STATE_IDEL;

void setup() {
  lockServo.attach(LOCK_SERVO_PIN);
  lockServo.write(45);
  Serial.begin(9600);

//Select pin mode for colums as input pulldown
pinMode(KeypadCol_pin[0], INPUT_PULLDOWN);
pinMode(KeypadCol_pin[1], INPUT_PULLDOWN);
pinMode(KeypadCol_pin[2], INPUT_PULLDOWN);
pinMode(KeypadCol_pin[3], INPUT_PULLDOWN);
//Select pin mode for rows as Output
pinMode(KeypadRow_pin[0], OUTPUT);
pinMode(KeypadRow_pin[1], OUTPUT);
pinMode(KeypadRow_pin[2], OUTPUT);

pinMode(LED, OUTPUT);

}

void loop() {
  switch(stateMachine)
  {
    case STATE_IDEL:
    //Serial.println("Current State: STATE_IDEL");
      if((PressedKey == 'R') &&(passwordAlive == true) && (attemptsIndex >0))
      {
        Serial.print("Access REQUSE Again!\nRemaining Attempts:");
        attemptsIndex --;
        Serial.println(attemptsIndex);
        stateMachine = STATE_REQUSET;
      }
      else if((passwordAlive == true) && (attemptsIndex >0))
      {
        Serial.print("Remaining Attempts:");
        Serial.println(attemptsIndex);
        stateMachine = STATE_REQUSET;
      }
      else if((readKeypad() == 'R') && ((attemptsIndex == 0)||passwordAlive == true))
      {
        Serial.println("Generate a new password...");
        generate_secure_4digit_password();
        Serial.println(localPassword);
        passwordAlive = true;
        attemptsIndex =10;
        stateMachine = STATE_REQUSET;
      }
      else if((readKeypad() == 'R') && ((attemptsIndex ==0)||passwordAlive == false))
      {
        Serial.println("Access REQUSET");
        generate_secure_4digit_password();
        Serial.println(localPassword);
        passwordAlive = true;
        attemptsIndex =10;
        stateMachine = STATE_REQUSET;
      }
    break;

    case STATE_REQUSET:
    Serial.println("Current State: STATE_REQUSET");
     passIndex =0;
      while(passIndex < 4)
      {
        PressedKey = readKeypad();
        if((PressedKey != '\0') && (PressedKey != 'R'))
        {
          enteredPassword[passIndex]=PressedKey;
          Serial.print(PressedKey);
          passIndex++;
        }
        else if(PressedKey == 'R')
          break;
         PressedKey = '\0';
      }
      Serial.println();
      (PressedKey == 'R')?stateMachine =  STATE_IDEL: stateMachine = STATE_VERIVCATION;
    break;

    case STATE_VERIVCATION:
      Serial.println("Current State: STATE_VERIVCATION");
      Serial.println(enteredPassword);
      Serial.println(localPassword);
      if (strcmp(enteredPassword, localPassword))
        stateMachine = STATE_ERROR;
      else
        stateMachine = STATE_PAUSED;
    break;
    case STATE_PAUSED:
      Serial.println("Current State: STATE_PAUSED");
      digitalWrite(LED, 1);
      lockServo.write(0);
      delay(1000);
      lockServo.write(45);
      Serial.println("PASSED ✔✔✔");
      digitalWrite(LED, 0);
      passwordAlive = false;
      stateMachine = STATE_IDEL;
    break;
    case STATE_ERROR:
    Serial.println("Current State: STATE_ERROR");
    Serial.println("Wrong Password XXX");
    for( i=0; i<10; i++)
    {
      digitalWrite(LED, 1);
      delay(50);
      digitalWrite(LED,0);
      delay(50);
    }
    (passwordAlive == true) ? (attemptsIndex--): 0;
    stateMachine = STATE_IDEL;
    break;
  }

}


char readKeypad() {
  for (byte r = 0; r < 3; r++) {
    // Activate current row by setting it high
    digitalWrite(KeypadRow_pin[r], 1);
    
    // Check each column
    for (byte c = 0; c < 4; c++) {
      if (digitalRead(KeypadCol_pin[c]) == 1) {
        // Wait for key release (debounce)
        delay(200);
        while(digitalRead(KeypadCol_pin[c]) == 1);
        delay(200);
        digitalWrite(KeypadRow_pin[r], 0); // Deactivate row
        return Keypad_numbers[r][c];
      }
    }

    // Deactivate row
    digitalWrite(KeypadRow_pin[r], 0);
  }
  return '\0'; // No key pressed
}



void generate_secure_4digit_password() {
    uint32_t random_number;
    for (int j = 0; j < 4; j++) {
        // Get 8 random bits (0-255) from hardware RNG
        random_number = esp_random();
        // Use o modulo 10 to get digit (0-9)
        random_number %= 10;
        // Build the 4-digit number
        localPassword[j] = random_number+48;
    }
}