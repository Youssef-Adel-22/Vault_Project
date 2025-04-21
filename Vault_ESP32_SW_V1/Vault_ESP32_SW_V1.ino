#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "esp_system.h"
#include <ESP32Servo.h>
#include <string.h>
#define LOCK_SERVO_PIN 13
#define LED 2
#define WIFI_SSID "Yusuf_Harby"         // enter your wifi name
#define WIFI_PASSWORD "Harby_Alaa2024"  // enter your wifi password
/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define SENDER_EMAIL "autopress2025@gmail.com"  // sender email id
#define SENDER_PASSWORD "oyjgqxbksmzrgisp"
/* Recipient's email*/
#define RECIPIENT_EMAIL "youssefharby111@gmail.com"  // receiver email id
/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;
Servo lockServo;
const uint KeypadRow_pin[3] = { 27, 14, 12 };
const uint KeypadCol_pin[4] = { 32, 33, 25, 26 };
uint8_t Keypad_numbers[3][4] = {
  { 'R', '0', '1', '2' },
  { '\0', '3', '4', '5' },
  { '6', '7', '8', '9' }
};
char PressedKey = '\0';
char localPassword[4] = { 0 };
char enteredPassword[5] = { 0 };
byte passIndex = 0;
byte attemptsIndex = 10;
boolean passwordAlive = false;
byte i;
typedef enum {
  STATE_IDEL,
  STATE_REQUSET,
  STATE_VERIVCATION,
  STATE_PAUSED,
  STATE_ERROR,
} stateMachine_t;
stateMachine_t stateMachine = STATE_IDEL;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
void setup() {
  lockServo.attach(LOCK_SERVO_PIN, 1050, 1950);  // Slightly narrower than standard range);
  lockServo.write(60);
  Serial.begin(115200);
  Serial.print("\nConnecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  SMTP_Config();
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
  switch (stateMachine) {
    case STATE_IDEL:
      //Serial.println("Current State: STATE_IDEL");
      if ((PressedKey == 'R') && (passwordAlive == true) && (attemptsIndex > 0)) {          //If user entered R while entring OTP
        Serial.print("Access REQUSE Again!\nRemaining Attempts:");
        attemptsIndex--; //Decrement the attemts trails
        Serial.println(attemptsIndex);//print the remaining attemts 
        stateMachine = STATE_REQUSET; //Return to Enter password state
      } else if ((passwordAlive == true) && (attemptsIndex > 0)) {                          // If user entered a wrong OTP
        Serial.print("Remaining Attempts:");
        Serial.println(attemptsIndex);
        stateMachine = STATE_REQUSET;
      } else if ((readKeypad() == 'R') && ((attemptsIndex == 0) || passwordAlive == true)) {//If user entered R to request OTP after 10 attempts
        Serial.println("Generate a new password...");
        generate_secure_4digit_password();
        Serial.println(localPassword);
        send_OTP();
        passwordAlive = true;
        attemptsIndex = 10;
        stateMachine = STATE_REQUSET;
      } else if ((readKeypad() == 'R') && ((attemptsIndex == 0) || passwordAlive == false)) {//If user entered R to request OTP at idel
        Serial.println("Access REQUSET");
        generate_secure_4digit_password();
        Serial.println(localPassword);
        send_OTP();
        passwordAlive = true;
        attemptsIndex = 10;
        stateMachine = STATE_REQUSET;
      }
      break;

    case STATE_REQUSET:
      Serial.println("Current State: STATE_REQUSET");
      passIndex = 0;
      while (passIndex < 4) {
        PressedKey = readKeypad();
        if ((PressedKey != '\0') && (PressedKey != 'R')) {
          enteredPassword[passIndex] = PressedKey;
          Serial.print(PressedKey);
          passIndex++;
        } else if (PressedKey == 'R')
          break;
        PressedKey = '\0';
      }
      Serial.println();
      (PressedKey == 'R') ? stateMachine = STATE_IDEL : stateMachine = STATE_VERIVCATION;
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
      lockServo.write(5);
      delay(1000);
      lockServo.write(60);
      Serial.println("PASSED ✔✔✔");
      digitalWrite(LED, 0);
      passwordAlive = false;
      stateMachine = STATE_IDEL;
      break;
    case STATE_ERROR:
      Serial.println("Current State: STATE_ERROR");
      Serial.println("Wrong Password XXX");
      for (i = 0; i < 10; i++) {
        digitalWrite(LED, 1);
        delay(50);
        digitalWrite(LED, 0);
        delay(50);
      }
      (passwordAlive == true) ? (attemptsIndex--) : 0;
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
        while (digitalRead(KeypadCol_pin[c]) == 1)
          ;
        delay(200);
        digitalWrite(KeypadRow_pin[r], 0);  // Deactivate row
        return Keypad_numbers[r][c];
      }
    }

    // Deactivate row
    digitalWrite(KeypadRow_pin[r], 0);
  }
  return '\0';  // No key pressed
}



void generate_secure_4digit_password() {
  uint32_t random_number;
  for (int j = 0; j < 4; j++) {
    // Get 8 random bits (0-255) from hardware RNG
    random_number = esp_random();
    // Use o modulo 10 to get digit (0-9)
    random_number %= 10;
    // Build the 4-digit number
    localPassword[j] = random_number + 48;
  }
}

boolean send_OTP() {
  /* Declare the Session_Config for user defined session credentials */
  ESP_Mail_Session config;
  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = SENDER_EMAIL;
  config.login.password = SENDER_PASSWORD;
  config.login.user_domain = "";
  /*
  Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 2;  // Egypt is GMT+2 (no DST adjustment)
  config.time.day_light_offset = 0;
  /* Connect to the server */
  if (!smtp.connect(&config)) {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return false;
  }
  if (!smtp.isLoggedIn()) {
    Serial.println("\nNot yet logged in.");
  } else {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }
  /* Declare the message class */
  SMTP_Message message;
  /* Set the message headers */
  message.sender.name = F("AUTOPRESS");
  message.sender.email = SENDER_EMAIL;
  message.subject = F("AUTOPRESS vault OTP ") + (String)localPassword;
  message.addRecipient(F("AUTOPRESS_Client"), RECIPIENT_EMAIL);
  //Send raw text message
  String textMsg = "AUTOPRESS vault OTP: ";
  textMsg += (String)localPassword;
  textMsg += "\n\nThank you for using AUTOPRESS services!\n\n\nBest Regards, Autopress team";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  return true;
}
void SMTP_Config() {
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);
  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(0);
  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);
}



/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());
  /* Print the sending result */
  if (status.success()) {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}