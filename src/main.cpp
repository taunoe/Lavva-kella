/**
 *  
 *  Circuit Sculpture "Lavva kellä"
 *  File: main.cpp
 *  Board: Wemos D1 mini lite
 * 
 *  Description:  
 *
 *  Copyright 2021 Tauno Erik
 *  https://taunoerik.art
 *
 *  Started: 25.01.2021
 *  Edited:  01.02.2021
 * 
 *  TODO:
 *  - music
 *  - web interface
 *  - 
 **/

/******************************************************************** 
 * 7-segment LED:
 *  |--A--|
 *  F     B
 *  |--G--|
 *  E     C
 *  |--D--|   
 *          dp
 *
 *  GF+AB
 *    #
 *  ED+Csp
 *
 *
 *  0 - High! (Common anode!)
 *  1 - Low
 ********************************************************************/

// NTP - Network Time Protocol
// UTC - Coordinated Universal Time.
// UTC does not vary, it is the same world wide
// UDP - User Datagram Protocol. Port 123
// https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/

/* Includes */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "wifi-secrets.h"  // Wifi ssid passwords
#include <NTPClient.h>  // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>

/* Enable debug Serial.print */
#define DEBUGno
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

/******************************************************************* 
 * Configurations
 *******************************************************************/

// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t WIFI_TIMEOUT = 5000;  // ms

// For UTC +2.00 : 2 * 60 * 60 : 7200
const uint32_t UTC_OFFSET = 7200;  // seconds //long

// NTP Terms of Service: https://www.pool.ntp.org/tos.html
const uint32_t UPDATE_INTERVAL = 60000*59*25;  // 1min = 60000ms

const int BUZZER_PIN = D1;  // Active piezo buzzer

// Shift register pins
const int DATA_PIN  = D7;  // D0 <- does not work!
const int LATCH_PIN = D5;
const int CLOCK_PIN = D6;

// Number of individual 7-segment displays
const int NUMBER_OF_7SEGS = 6;

// Alphabet size: numbers + letters
const int ALPHABET_SIZE = 17;

// All 6 numbers: 'AA:BB:CC.
uint8_t dataframe[NUMBER_OF_7SEGS] = {
  0b11111111, 0b11111111,  // 'AA. [0], [1]
  0b11111111, 0b11111111,  // 'BB. [2], [3]
  0b11111111, 0b11111111,  // 'CC. [4], [5]
};

// Clock
const int SECOND = 1000;
uint32_t prev_millis {};
boolean is_second = false;
boolean is_clock_dots = true;  // 00:00:00

int h {};  // hours
int m {};  // minutes
int s {};  // seconds


/************************************
 * 
 ************************************/

ESP8266WiFiMulti wifiMulti;

WiFiUDP ntpUDP;
// Define NTP Client to get time
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
NTPClient NTP_time(ntpUDP, "europe.pool.ntp.org", UTC_OFFSET, UPDATE_INTERVAL);


/******************************************************************* 
 * Functions
 *******************************************************************/

/*
 * Function to serial print compile date.
 */
void print_info() {
  Serial.println("\"Lavva kellä\"");
  Serial.print("Compiled: ");
  Serial.print(__TIME__);
  Serial.print(" ");
  Serial.println(__DATE__);
  Serial.println("Made by Tauno Erik.");
}

/*
 * Maintain WiFi connection
 */
void check_wifi() {
  if (wifiMulti.run(WIFI_TIMEOUT) == WL_CONNECTED) {
    // DEBUG_PRINT("\nWiFi connected: ");
    // DEBUG_PRINT(WiFi.SSID());
    // DEBUG_PRINT(" ");
    // DEBUG_PRINTLN(WiFi.localIP());
  } else {
    delay(100);
    Serial.println("\n--> WiFi not connected!");
  }
}

namespace sound {
  /*
   */
  void beep() {
    // uint32_t current_millis = millis();
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(40);
    digitalWrite(BUZZER_PIN, LOW);
  }
}
/********************************************************************/

namespace numbers {
  uint8_t tens[ALPHABET_SIZE][2] = {
    {0b10001111, 0b00011111},  // 0_
    {0b11101111, 0b11011111},  // 1_
    {0b11001111, 0b00101111},  // 2_
    {0b11001111, 0b10001111},  // 3_
    {0b10101111, 0b11001111},  // 4_
    {0b10011111, 0b10001111},  // 5_
    {0b10011111, 0b00001111},  // 6_
    {0b11001111, 0b11011111},  // 7_
    {0b10001111, 0b00001111},  // 8_
    {0b10001111, 0b10001111},  // 9_
    {0b10001111, 0b01001111},  // A_
    {0b10111111, 0b00001111},  // b_
    {0b10011111, 0b00111111},  // C_
    {0b11101111, 0b00001111},  // d_
    {0b10011111, 0b00101111},  // E_
    {0b10011111, 0b00101111},  // F_
    {0b01111111, 0b11111111},  // .__ dot
  };

  uint8_t ones[ALPHABET_SIZE][2] = {
    {0b11111000, 0b11110001},  // _0
    {0b11111110, 0b11111101},  // _1
    {0b11110100, 0b11110011},  // _2
    {0b11110100, 0b11111001},  // _3
    {0b11110010, 0b11111101},  // _4
    {0b11110001, 0b11111001},  // _5
    {0b11110001, 0b11110001},  // _6
    {0b11111100, 0b11111101},  // _7
    {0b11110000, 0b11110001},  // _8
    {0b11110000, 0b11111001},  // _9
    {0b11110000, 0b11110101},  // _A
    {0b11111011, 0b11110001},  // _b
    {0b11111001, 0b11110011},  // _C
    {0b11110110, 0b11110001},  // _d
    {0b11110001, 0b11110011},  // _E
    {0b11110001, 0b11110111},  // _F
    {0b11111111, 0b11111110},  // __. dot
  };
}  // namespace numbers *********************************************/


namespace update {
  /*
  * Function to update display.
  * Input *pointer to dataframe. All 6 numbers: '00:00:00.
  * 
  * MSBFIRST - Most Significant Bit First
  * LSBFIRST - Least Significant Bit First
  */
  void display(uint8_t *dataframe) {
    digitalWrite(LATCH_PIN, LOW);

    for (uint8_t i{}; i < NUMBER_OF_7SEGS; i++) {
      // DEBUG_PRINT(" dataframe[");
      // DEBUG_PRINT(i);
      // DEBUG_PRINT("] = ");
      // DEBUG_PRINTLN(dataframe[i]);
      shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, dataframe[i]);
    }

    digitalWrite(LATCH_PIN, HIGH);
  }

  /*
   * Function to find "tens" from given digit.
   */
  uint8_t find_tens(uint8_t number) {
    return (number / 10) % 10;
  }

  /*
   * Function to find "ones" from given digit.
   */
  uint8_t find_ones(uint8_t number) {
    return number % 10;
  }

  /*
   * Function to update hours in dataframe
   * dataframe[0] and [1]
   * Input number 0-99
   */
  void hours(uint8_t number) {
    // If we have overflow
    if (number > 99) {
      number = 99;
    }

    // Find tens and ones
    uint8_t s_tens = find_tens(number);
    uint8_t s_ones = find_ones(number);

    // Tens and ones combined:
    uint8_t merged_digit[2] = {0};

    // Merge tens and ones binarys with logical disjunction (bitwise OR)
    merged_digit[0] = ~(numbers::tens[s_tens][0]) | ~(numbers::ones[s_ones][0]);
    merged_digit[1] = ~(numbers::tens[s_tens][1]) | ~(numbers::ones[s_ones][1]);

    if (is_clock_dots) {
      // Set bit high.
      // merged_digit[0] = merged_digit[0] | (1<<7);  // '00
      merged_digit[1] = merged_digit[1] | 1;  // 00.
    } else {
      // Set bit low.
      merged_digit[1] = merged_digit[1] & ~(1);  // 00.
    }

    // Change dataframe
    dataframe[0] = ~(merged_digit[0]);
    dataframe[1] = ~(merged_digit[1]);
  }

  /*
   * Function to update minuts in dataframe
   * dataframe[2] and [3]
   * Input number 0-99
   */
  void minutes(uint8_t number) {
    DEBUG_PRINT("\n update::minutes ");
    DEBUG_PRINT(number);
    // If we have overflow
    if (number > 99) {
      number = 99;
    }

    // Find tens and ones
    uint8_t m_tens = find_tens(number);
    uint8_t m_ones = find_ones(number);
    DEBUG_PRINT(" tens: ");
    DEBUG_PRINT(m_tens);
    DEBUG_PRINT(" ones: ");
    DEBUG_PRINT(m_ones);

    // Tens and ones combined:
    uint8_t merged_digit[2] = {0};

    DEBUG_PRINT("\n merged_digit[0]=");
    DEBUG_PRINT(merged_digit[0]);
    DEBUG_PRINT(" merged_digit[1]=");
    DEBUG_PRINT(merged_digit[1]);

    // Merge tens and ones binarys with logical disjunction (bitwise OR)
    merged_digit[0] = ~(numbers::tens[m_tens][0]) | ~(numbers::ones[m_ones][0]);
    merged_digit[1] = ~(numbers::tens[m_tens][1]) | ~(numbers::ones[m_ones][1]);

    DEBUG_PRINT("\n merged_digit[0]=");
    DEBUG_PRINT(merged_digit[0]);
    DEBUG_PRINT(" merged_digit[1]=");
    DEBUG_PRINT(merged_digit[1]);

    if (is_clock_dots) {
      // Set bits high.
      merged_digit[0] = merged_digit[0] | (1<<7);  // '00
      merged_digit[1] = merged_digit[1] | 1;       // 00.
    } else {
      // Set bits low.
      merged_digit[0] = merged_digit[0] & ~(1<<7);  // '00
      merged_digit[1] = merged_digit[1] & ~(1);  // 00.
    }

    // Change dataframe
    dataframe[2] = ~(merged_digit[0]);
    dataframe[3] = ~(merged_digit[1]);

    DEBUG_PRINT("\n dataframe[2]=");
    DEBUG_PRINT(dataframe[2]);
    DEBUG_PRINT(" dataframe[3]=");
    DEBUG_PRINT(dataframe[3]);
  }

  /*
   * Function to update seconds in dataframe
   * dataframe[4] and [5]
   * Input number 0-99
   */
  void seconds(uint8_t number) {
    // If we have overflow
    if (number > 99) {
      number = 99;
    }

    // Find tens and ones
    uint8_t s_tens = find_tens(number);
    uint8_t s_ones = find_ones(number);

    // Tens and ones combined: 0b000000, 0b000000
    uint8_t merged_digit[2] = {0};

    // Merge tens and ones binarys with logical disjunction (bitwise OR)
    merged_digit[0] = ~(numbers::tens[s_tens][0]) | ~(numbers::ones[s_ones][0]);
    merged_digit[1] = ~(numbers::tens[s_tens][1]) | ~(numbers::ones[s_ones][1]);

    // Display clock dots?
    if (is_clock_dots) {
      merged_digit[0] = merged_digit[0] | (1<<7);  // '00
    } else {
      merged_digit[0] = merged_digit[0] & ~(1<<7);  // '00
    }

    // Change dataframe
    dataframe[4] = ~(merged_digit[0]);
    dataframe[5] = ~(merged_digit[1]);
  }

}  // namespace update **********************************************/

namespace my_clock {
  /*
   * Function to run a clock
   */
  void run() {
    uint32_t current_millis = millis();

    if (current_millis - prev_millis >= SECOND) {
      is_second = true;
      prev_millis = current_millis;
    }

    if (is_second) {
      s++;

      // Seconds
      if (s >= 60) {
        m++;
        s = 0;
      }
      // Minutes
      if (m >= 60) {
        h++;
        sound::beep();
        m = 0;
      }
      // Hours
      if (h >= 24) {
        h = 0;
      }

      Serial.print("h");
      Serial.print(h);
      Serial.print("m");
      Serial.print(m);
      Serial.print("s");
      Serial.println(s);

      update::hours(h);
      update::minutes(m);
      update::seconds(s);

      is_second = false;
    }
  }

}  // namespace my_clock ********************************************/

namespace special {

  void dots_on() {
    dataframe[1] = ~((~dataframe[1]) | 1);
  }

}  // namespace special *********************************************/


void setup() {
  Serial.begin(115200);

  // Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  // Don't save WiFi configuration in flash - optional
  WiFi.persistent(false);

  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);

  // Register multi WiFi networks
  wifiMulti.addAP(Secrets::ssd1, Secrets::pass1);
  wifiMulti.addAP(Secrets::ssd2, Secrets::pass2);
  wifiMulti.addAP(Secrets::ssd3, Secrets::pass3);
  wifiMulti.addAP(Secrets::ssd4, Secrets::pass4);

  NTP_time.begin();

  print_info();
  
  sound::beep();
}

void loop() {
  check_wifi();

  NTP_time.update();
  // Serial.println(NTP_time.getFormattedTime());
  h = NTP_time.getHours();
  m = NTP_time.getMinutes();
  s = NTP_time.getSeconds();

  my_clock::run();

  update::display(dataframe);
}
