#include "thermistorKS103J2.h"
#include "thermistor055114.h"

ThermistorKS103J2 thermistor_old = ThermistorKS103J2();
Thermistor055114 thermistor_new = Thermistor055114();

float current_temp_thermistor_old = 0.0;
float current_temp_thermistor_new = 0.0;

bool auto_print = false;
unsigned long print_timestamp = 0;
const int print_interval = 250;

void read_thermistor() {
    if (thermistor_old.update()) {
        current_temp_thermistor_old = thermistor_old.temperature();
    }
    if (thermistor_new.update()) {
        current_temp_thermistor_new = thermistor_new.temperature();
    }
}

void print_temperature(bool force=false) {
    if (auto_print && print_timestamp + print_interval < millis()) {
        print_timestamp = millis();
        Serial.print(current_temp_thermistor_old);
        Serial.print(',');
        Serial.println(current_temp_thermistor_new);
    }
    else if (!auto_print && force) {
        Serial.print(current_temp_thermistor_old);
        Serial.print(',');
        Serial.println(current_temp_thermistor_new);
    }
}

void setup() {
    Serial.begin(115200);
    thermistor_old.set_pin(A0);
    thermistor_new.set_pin(A5);
    while (!thermistor_old.update()) {}
    while (!thermistor_new.update()) {}
    current_temp_thermistor_old = thermistor_old.temperature();
}

void loop() {
    read_thermistor();
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
