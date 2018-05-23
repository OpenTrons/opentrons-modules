/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

#include <Arduino.h>
#include <avr/wdt.h>
#include "lights.h"
#include "peltiers.h"
#include "thermistor.h"
#include "gcode.h"

Lights lights = Lights();
Peltiers peltiers = Peltiers();
Thermistor thermistor = Thermistor();
Gcode gcode = Gcode();

#define pin_tone_out 11
#define pin_fan_pwm 9

#define TEMPERATURE_MAX 99
#define TEMPERATURE_MIN -9

#define TEMPERATURE_FEELS_COLD 10
#define TEMPERATURE_ROOM 25
#define TEMPERATURE_FEELS_HOT 60

#define BOOTLOADER_ON_WDTO TRUE

bool START_BOOTLOADER = false;
unsigned long start_bootloader_timestamp = 0;
const int start_bootloader_timeout = 3000;  // 3 seconds

#if BOOTLOADER_ON_WDTO == TRUE
#define WD_TIMEOUT 1000  
#endif

int TARGET_TEMPERATURE = TEMPERATURE_ROOM;
bool IS_TARGETING = false;

String device_serial = "TD001180622A01";
String device_model = "001";
String device_version = "edge-1a2b3c4";

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void set_target_temperature(int target_temp){
  if (target_temp < TEMPERATURE_MIN) {
    target_temp = TEMPERATURE_MIN;
    gcode.print_warning(
      "Target temperature too low, setting to TEMPERATURE_MIN degrees");
  }
  if (target_temp > TEMPERATURE_MAX) {
    target_temp = TEMPERATURE_MAX;
    gcode.print_warning(
      "Target temperature too high, setting to TEMPERATURE_MAX degrees");
  }
  peltiers.disable_peltiers();
  disable_target();
  IS_TARGETING = true;
  TARGET_TEMPERATURE = target_temp;
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void disable_target(){
  IS_TARGETING = false;
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void delay_minutes(int minutes){
  for (int i=0;i<minutes;i++){
    for (int s=0; s<60; s++){
      delay(1000);  // 1 minute
    }
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void set_fan_percentage(float percentage){
  percentage = constrain(percentage, 0.0, 1.0);
  analogWrite(pin_fan_pwm, int(percentage * 255.0));
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void cold(float amount) {
  peltiers.set_cold_percentage(amount);
}

void hot(float amount) {
  peltiers.set_hot_percentage(amount);
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void stabilize_to_target_temp(int current_temp, bool set_fan=false){
  peltiers.update_peltier_cycle();
  if (IS_TARGETING) {
    // if we've arrived, just be calm, but don't turn off
    if (current_temp == TARGET_TEMPERATURE) {
      if (TARGET_TEMPERATURE > TEMPERATURE_ROOM) {
        hot(0.5);
        if (set_fan) set_fan_percentage(0.5);
      }
      else {
        cold(1.0);
        if (set_fan) set_fan_percentage(1.0);
      }
    }
    else if (TARGET_TEMPERATURE - current_temp > 0.0) {
      if (TARGET_TEMPERATURE > TEMPERATURE_ROOM) {
        hot(1.0);
        if (set_fan) set_fan_percentage(0.5);
      }
      else {
        hot(0.2);
        if (set_fan) set_fan_percentage(1.0);
      }
    }
    // COOL DOWN
    else {
      if (TARGET_TEMPERATURE < TEMPERATURE_ROOM) {
        cold(1.0);
        if (set_fan) set_fan_percentage(1.0);
      }
      else {
        cold(0.2);
        if (set_fan) set_fan_percentage(0.5);
      }
    }
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void _set_color_bar_from_range(int val, int min, int middle, int max) {
  /*
    This method uses a temperature range (Celsius), to set the color of the
    RGBW color bar. It uses three data points in Celsius (min, middle, max),
    and does a linear transition between them, then multiplies by the three
    corresponding colors (cold, room, hot), to create a fade between colors
  */
  if (IS_TARGETING) {
    lights.set_color_bar_brightness(1.0);
  }
  else {
    lights.set_color_bar_brightness(0.5);
    lights.set_color_bar(0, 0, 0, 1);
  }
  float cold[4] = {0, 0, 1, 0};
  float room[4] = {0, 1, 0, 0};
  if (IS_TARGETING == false) {  // set normal color to white when not active
    room[1] = 0;
    room[3] = 1;
  }
  float hot[4] = {1, 0, 0, 0};
  if (val == middle) {
    lights.set_color_bar(room[0], room[1], room[2], room[3]);
  }
  // cold
  else if (val < middle) {
    if (val < min) {
      lights.set_color_bar(cold[0], cold[1], cold[2], cold[3]);
    }
    else {
      // scale it to cold color
      float m = float(val - min) / float(middle - min);  // 1=room, 0=cold
      float r = (m * room[0]) + ((1.0 - m) * cold[0]);
      float g = (m * room[1]) + ((1.0 - m) * cold[1]);
      float b = (m * room[2]) + ((1.0 - m) * cold[2]);
      float w = (m * room[3]) + ((1.0 - m) * cold[3]);
      lights.set_color_bar(r, g, b, w);
    }
  }
  // hot
  else {
    if (val > max) {
      lights.set_color_bar(hot[0], hot[1], hot[2], hot[3]);
    }
    else {
      // scale it to hot color
      float m = float(val - middle) / float(max - middle);  // 1=hot, 0=room
      float r = (m * hot[0]) + ((1.0 - m) * room[0]);
      float g = (m * hot[1]) + ((1.0 - m) * room[1]);
      float b = (m * hot[2]) + ((1.0 - m) * room[2]);
      float w = (m * hot[3]) + ((1.0 - m) * room[3]);
      lights.set_color_bar(r, g, b, w);
    }
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void update_temperature_display(int current_temp, boolean force=false){
  lights.display_number(current_temp, force);
  _set_color_bar_from_range(
    current_temp, TEMPERATURE_FEELS_COLD, TEMPERATURE_ROOM, TEMPERATURE_FEELS_HOT);
  if (current_temp > TEMPERATURE_MAX || current_temp < TEMPERATURE_MIN){
    // flash the lights or something...
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void activate_bootloader(){
  #if BOOTLOADER_ON_WDTO == TRUE
  // Method 1: Uses a WDT reset to enter bootloader.
  // Works on the modified Caterina bootloader that allows 
  // bootloader access after a WDT reset
  // -----------------------------------------------------------------
  wdt_enable(WDTO_1S);  //Timeout in 1 second
  unsigned long timerStart = millis();
  while(millis() - timerStart < WD_TIMEOUT + 100){
    //Wait out until WD times out
  }
  #else
  // Method 2: Uses a jump to bootloader location from within the code
  // -----------------------------------------------------------------
  // cli();
  // // disable watchdog, if enabled
  // // disable all peripherals
  // UDCON = 1;
  // USBCON = (1<<FRZCLK);  // disable USB
  // UCSR1B = 0;
  // _delay_ms(5);
  // #if defined(__AVR_ATmega32U4__)
  //     EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
  //     TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0; TIMSK4 = 0; UCSR1B = 0; TWCR = 0;
  //     DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0; TWCR = 0;
  //     PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
  //     asm volatile("jmp 0x7000");   //Bootloader start address
  // #endif
  #endif
  // Should never get here but in case it does because 
  // WDT failed to start or timeout..
  START_BOOTLOADER = false;
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void disengage() {
  peltiers.disable_peltiers();
  disable_target();
  set_fan_percentage(0.0);
}

void print_temperature() {
  if (IS_TARGETING) {
    gcode.print_targetting_temperature(
      TARGET_TEMPERATURE,
      thermistor.plate_temperature()
    );
  }
  else {
    gcode.print_stablizing_temperature(
      thermistor.plate_temperature()
    );
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void start_dfu_timeout() {
  gcode.print_warning("Restarting and entering bootloader in 3 second...");
  START_BOOTLOADER = true;
  start_bootloader_timestamp = millis();
}

void check_if_bootloader_starts() {
  if (START_BOOTLOADER) {
    if (start_bootloader_timestamp + start_bootloader_timeout < millis()) {
      activate_bootloader();
    }
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void read_gcode(){
  if (gcode.received_newline()) {
    while (gcode.pop_command()) {
      switch(gcode.code) {
        case GCODE_NO_CODE:
          break;
        case GCODE_GET_TEMP:
          print_temperature();
          break;
        case GCODE_SET_TEMP:
          if (gcode.read_int('S')) {
            set_target_temperature(gcode.parsed_int);
            set_fan_percentage(0.0);
            // wait for fan to shutdown
            unsigned long now = millis();
            while (now + 2500 > millis()){
              update_temperature_display(thermistor.plate_temperature());
            }
            // engage peltiers, and let current settle before turning on fan
            now = millis();
            float now_temp;
            while (now + 2500 > millis()){
              now_temp = thermistor.plate_temperature();
              update_temperature_display(now_temp);
              stabilize_to_target_temp(now_temp, false);  // no fan!
            }
          }
          break;
        case GCODE_DISENGAGE:
          disengage();
          break;
        case GCODE_DEVICE_INFO:
          gcode.print_device_info(device_serial, device_model, device_version);
          break;
        case GCODE_DFU:
          start_dfu_timeout();

          break;
        default:
          break;
      }
    }
    gcode.send_ack();
  }
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void setup() {

  #if BOOTLOADER_ON_WDTO == TRUE
  wdt_disable();          /* Disable watchdog if enabled by bootloader/fuses */
  #endif

  pinMode(pin_tone_out, OUTPUT);
  pinMode(pin_fan_pwm, OUTPUT);
  gcode.setup(115200);
  peltiers.setup_peltiers();
  set_fan_percentage(0);
  lights.setup_lights();
  lights.set_numbers_brightness(0.5);
  disengage();
  update_temperature_display(thermistor.plate_temperature(), true);
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////

void loop(){
  read_gcode();
  int current_temp = thermistor.plate_temperature();
  update_temperature_display(current_temp);
  stabilize_to_target_temp(current_temp, true);
  check_if_bootloader_starts();
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////
