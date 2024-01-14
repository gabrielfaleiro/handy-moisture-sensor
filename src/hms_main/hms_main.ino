#include <Arduino.h>
#include "decod_7_seg_disp.h"
#include "esp8266_pinout.h"
#include "eeprom_app.h"
#include "button_input.h"
#include "timing.h"

/* /////////////////////////////////////////////////////////////////
    Defines and enums
*/ /////////////////////////////////////////////////////////////////

#define blinkingPeriod 300

enum States {reading, condiguration_min, configuration_max};

/* /////////////////////////////////////////////////////////////////
    Global variables
*/ /////////////////////////////////////////////////////////////////

Decod7SegDisp tens_digit(D5, D7, D6, D0);
Decod7SegDisp units_digit(D4, D2, D1, D3);
EepromApp     permanent_mem;
ButtonInput   button_config(D8, HIGH);
uint16_t      min_humidity = 0;
uint16_t      max_humidity = 0;

/* /////////////////////////////////////////////////////////////////
    Display functions
*/ /////////////////////////////////////////////////////////////////

void displayNumber(uint num){
  tens_digit.displayDigit(num/10);
  units_digit.displayDigit(num%10);
}

void blinkingDisplayNumber(uint num){
  static uint8_t blink_state = 0;
  static unsigned long prev_counter = 0;
  unsigned long elapsed_time = 0;

  elapsed_time = elapsedTime(prev_counter, millis());

  if (elapsed_time >= blinkingPeriod){
    prev_counter = millis();
    blink_state = ! blink_state;
  }

  if(blink_state) displayNumber(num);
  else{
    // 0x0f (15) turns off the display
    tens_digit.displayDigit(0x0f);
    units_digit.displayDigit(0x0f);
  }
}

/* /////////////////////////////////////////////////////////////////
    Setup
*/ /////////////////////////////////////////////////////////////////

void setup() {
  pinMode(A0, INPUT);

  min_humidity = permanent_mem.getMinHumidity();
  if(min_humidity == 0xffff){ // If the memory is not initialized
    min_humidity = 800; // Sensible default value
    permanent_mem.setMinHumidity(min_humidity);
  }
  max_humidity = permanent_mem.getMaxHumidity();
  if(max_humidity == 0xffff){ // If the memory is not initialized
    max_humidity = 300; // Sensible default value
    permanent_mem.setMaxHumidity(max_humidity);
  }
}

/* /////////////////////////////////////////////////////////////////
    Loop
*/ /////////////////////////////////////////////////////////////////

void loop() {
  static States st            = reading;
  uint moisture_sensor_value  = 0;
  uint8_t mapped_sensor_value = 0;
  uint8_t but_conf_push_type  = 0;

  moisture_sensor_value = analogRead(A0);
  but_conf_push_type = button_config.getPushType();

  switch(st){
    case reading:
      mapped_sensor_value = map(moisture_sensor_value, min_humidity, max_humidity, 0, 99);
      displayNumber(mapped_sensor_value);

      // Transitions
      if(but_conf_push_type == 3){
        st = condiguration_min;
      }
    break;
    case condiguration_min:
      blinkingDisplayNumber(0);
      
      // Transitions
      if(but_conf_push_type == 1){
        // Save min humidity
        permanent_mem.setMinHumidity(moisture_sensor_value);
      }
      if(but_conf_push_type == 2){
        st = configuration_max;
      }
    break;
    case configuration_max:
      blinkingDisplayNumber(99);

      // Transitions
      if(but_conf_push_type == 1){
        // Save max humidity
        permanent_mem.setMaxHumidity(moisture_sensor_value);
      }
      if(but_conf_push_type == 2){
        st = reading;
      }

    break;
    default:
      // Uncontrolled state: Reset manually
      tens_digit.displayDigit(0x0e); // Shows an e standing for error
      units_digit.displayDigit(1); // error code
    break;
  }

  delay(50);
}
