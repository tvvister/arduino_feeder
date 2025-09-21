#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>

const byte VERSION = 14;
int N = 7;
DateTime schedule[] = {
  DateTime(2025, 9, 22, 8, 0, 0),
  DateTime(2025, 9, 23, 8, 0, 0),
  DateTime(2025, 9, 24, 8, 0, 0),
  DateTime(2025, 9, 25, 8, 0, 0),
  DateTime(2025, 9, 26, 8, 0, 0),
  DateTime(2025, 9, 27, 8, 0, 0),
  DateTime(2025, 9, 28, 8, 0, 0),
};


SLEEP_DURATION_MILLI_SECS = 5000;

DateTime prev_time;
RTC_DS3231 rtc;
#define DRV_PIN_1 2
#define DRV_PIN_2 4
#define DRV_PIN_3 3
#define DRV_PIN_4 5

double STEP_DEGREES = 44.0;
int STEP_TICKS = 260;

//recovery state block-----------------------------------------------------------------------
const int EE_ADDRESS = 0;
const int NOTHING_HANDLED = -1;

struct State {
  byte version;
  int index_of_last_handled_date;
};
State state;
int current_schedule_point_index = 0;

void write_state() {
  EEPROM.put(EE_ADDRESS, state);
}

void print_state() {
  Serial.print("Восстановленное состояние из EEPROM: (version, index_of_last_handled_date)=(");
  Serial.print(state.version);
  Serial.print(", ");
  Serial.print(state.index_of_last_handled_date);
  Serial.println(")");
}
void recover_state() {
  Serial.println("Читаем из EEPROM: ");
  EEPROM.get(EE_ADDRESS, state);
  print_state();
  if (state.version != VERSION) {
    state.index_of_last_handled_date = NOTHING_HANDLED;
    state.version = VERSION;
    write_state();
  }
  current_schedule_point_index = state.index_of_last_handled_date + 1;
}

//-----------------------------------------------------------------------

void _write4(bool p0, bool p1, bool p2, bool p3) {
  digitalWrite(DRV_PIN_1, p0);
  digitalWrite(DRV_PIN_2, p1);
  digitalWrite(DRV_PIN_3, p2);
  digitalWrite(DRV_PIN_4, p3);
  Serial.begin(57600);
}

// сделать шаг в направлении (0 или 1)
void step(bool dir) {
  static uint8_t curstep;
  dir ? ++curstep : --curstep;

  switch (curstep & 0b11) {
    case 0: _write4(1, 0, 1, 0); break;
    case 1: _write4(0, 1, 1, 0); break;
    case 2: _write4(0, 1, 0, 1); break;
    case 3: _write4(1, 0, 0, 1); break;
  }
}

void setup_motor() {
  pinMode(DRV_PIN_1, OUTPUT);
  pinMode(DRV_PIN_2, OUTPUT);
  pinMode(DRV_PIN_3, OUTPUT);
  pinMode(DRV_PIN_4, OUTPUT);
}


void setup() {
  setup_motor();
  setup_clock();
  recover_state();
}

void setup_clock() {
  Serial.begin(57600);

#ifndef ESP8266
  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB
#endif

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  prev_time = rtc.now();
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}



// void loop() {
//     // прочитать и вывести время строкой
//     Serial.println(rtc.getTime().toString());
//     delay(1000);
// }
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
void loop_clock() {
  DateTime time = rtc.now();
  if (prev_time < time) {
    Serial.println(String("DateTime::TIMESTAMP_FULL:\t") + time.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.flush();
    //Full Tim
  }
}

void make_step() {
  int steps = STEP_TICKS;
  while (steps--) {
    step(1);
    delay(20);
  }
}

void loop() {
  DateTime time = rtc.now();
  Serial.print("Текущий индекс: ");
  Serial.println(current_schedule_point_index);
  if (current_schedule_point_index < N) {
    if (time >= schedule[current_schedule_point_index]) {
      Serial.flush();
      make_step();
      state.index_of_last_handled_date = current_schedule_point_index;
      write_state();
      ++current_schedule_point_index;
    }
  }
  Serial.flush();
  delay(SLEEP_DURATION_MILLI_SECS);
  loop_clock();
}
