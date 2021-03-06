#ifndef GCODE_H
#define GCODE_H

#include "Arduino.h"

#define CODE_INT(gcode) static_cast<int>(gcode)

#define MAX_SERIAL_BUFFER_LENGTH    100
#define MAX_SERIAL_DIGITS_IN_NUMBER 7
#define SERIAL_DIGITS_IN_RESPONSE   3
#define DIGITS_IN_DEBUG_RESPONSE    4

#define GCODES_TABLE  \
  GCODE_DEF(no_code, -),            \
  GCODE_DEF(get_lid_status, M119),  \
  GCODE_DEF(open_lid, M126),        \
  GCODE_DEF(close_lid, M127),       \
  GCODE_DEF(set_lid_temp, M140),    \
  GCODE_DEF(get_lid_temp, M141),    \
  GCODE_DEF(deactivate_lid_heating, M108),  \
  GCODE_DEF(set_plate_temp, M104),  \
  GCODE_DEF(get_plate_temp, M105),  \
  GCODE_DEF(deactivate_plate, M14), \
  GCODE_DEF(set_led_override, M200),\
  GCODE_DEF(set_led, M210),         \
  GCODE_DEF(set_ramp_rate, M566),   \
  GCODE_DEF(get_pid_params, M300),  \
  GCODE_DEF(edit_pid_params, M301), \
  GCODE_DEF(pause, M76),            \
  GCODE_DEF(deactivate_all, M18),   \
  GCODE_DEF(get_device_info, M115), \
  GCODE_DEF(set_offset_constants, M116), \
  GCODE_DEF(get_offset_constants, M117), \
  GCODE_DEF(set_offset, M20), \
  GCODE_DEF(heatsink_fan_pwr_manual, M106), \
  GCODE_DEF(heatsink_fan_auto_on, M107),  \
  GCODE_DEF(dfu, dfu),              \
  GCODE_DEF(motor_reset, mrst),     \
  GCODE_DEF(debug_mode, M111),      \
  GCODE_DEF(print_debug_stat, stat),\
  GCODE_DEF(continous_debug_stat, cont),\
  GCODE_DEF(max, -)

#define GCODE_DEF(name, _) name

enum class Gcode
{
  GCODES_TABLE
};

/* This class sets up the serial port to receive gcodes and send responses.
 * A valid gcode would be in the format:
 *         <gcode1> <gcode1_arg1> <gcode1_arg2> <gcode2> <gcode2_arg1> \r\n
 * Note that each serial input line can contain as many number of gcodes and
 * their arguments as the Serial Buffer Size allows, however, the line should end
 * with a `\r\n`. Spaces can be skipped since they are ignored.
 *
 * To use a GCodeHandler object, use `setup(baudrate)` to start serial communication,
 * then in a loop, check for `received_newline()` and use `get_command()`
 * until `buffer_empty()` to cache the received gcode (fifo order) and its arguments.
 * Use `pop_arg(key)` to cache the desired keyed argument and read the argument
 * using `popped_arg()`.
 * Responses can be sent using the appropriate 'response' methods below.
 */
class GcodeHandler
{
    public:
      GcodeHandler();
      void setup(int baudrate);
      bool received_newline();
      Gcode get_command();
      void send_ack();
      bool buffer_empty();
      void device_info_response(String serial, String model, String version);
      void targetting_temperature_response(float target_temp, float current_temp);
      void targetting_temperature_response(float target_temp, float current_temp,
                                           float time_left, float total_hold_time,
                                           bool at_target);
      void idle_temperature_response(float current_temp);
      void idle_lid_temperature_response(float current_temp);
      float popped_arg();
      bool pop_arg(char key);
      void response(String msg);
      void response(String param, String msg);
      void system_error_message(String msg);
      void add_debug_response(String param, float val);
      void add_debug_timestamp();

    private:
      struct
      {
        Gcode code;
        String args_string;
      }_command;
      String _gcode_buffer_string;
      String _serial_buffer_string;
      void _strip_serial_buffer();
      float _parsed_arg;
      bool _find_command(const String&, uint8_t *, uint8_t *);
};

#endif
