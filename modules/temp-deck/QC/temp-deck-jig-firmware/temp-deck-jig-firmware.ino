#include "Adafruit_MAX31865.h"
// use hardware SPI, just pass in the CS pin
Adafruit_MAX31865 rtdSensor = Adafruit_MAX31865(10);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

#include "lights.h"


Lights lights = Lights();

double current_temp_sum = 0.0;
double current_temp_counter = 0;

uint16_t rtd;
float ratio;
uint8_t fault;

bool auto_print = false;
unsigned long print_timestamp = 0;
const int print_interval = 250;

double read_rtd() {
  fault = rtdSensor.readFault();
  if (fault) {
    Serial.print(F("Fault 0x")); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println(F("RTD High Threshold")); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println(F("RTD Low Threshold")); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println(F("REFIN- > 0.85 x Bias")); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println(F("REFIN- < 0.85 x Bias - FORCE- open")); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println(F("RTDIN- < 0.85 x Bias - FORCE- open")); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println(F("Under/Over voltage"));
    }
    rtdSensor.clearFault();
  }
  else {
    rtd = rtdSensor.readRTD();
    ratio = rtd;
    ratio /= 32768;
    float new_temp = rtdSensor.temperature(RNOMINAL, RREF);
    current_temp_sum += new_temp;
    current_temp_counter += 1;
    lights.display_number(new_temp + 0.5, true);
  }
}

void print_temperature(bool force=false) {
    if (auto_print && print_timestamp + print_interval < millis()) {
        print_timestamp = millis();
        Serial.println(current_temp_sum / current_temp_counter);
        current_temp_sum = 0;
        current_temp_counter = 0;
    }
    else if (!auto_print && force) {
        Serial.println(current_temp_sum / current_temp_counter);
        current_temp_sum = 0;
        current_temp_counter = 0;
    }
}

void setup() {
    Serial.begin(115200);
    lights.setup_lights();
    lights.set_numbers_brightness(0.25);
    rtdSensor.begin(MAX31865_2WIRE);
}

void loop() {
    read_rtd();
    print_temperature();
    if (Serial.available()) {
        if (Serial.peek() == '1') {
            auto_print = true;
        }
        else if (Serial.peek() == '0') {
            auto_print = false;
        }
        print_temperature(true);
        while (Serial.available()) {
            Serial.read();
            delay(2);
        }
    }
}
