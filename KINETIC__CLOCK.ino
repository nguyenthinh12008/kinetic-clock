#include <Servo.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

// ===== Cấu hình =====
#define ANGLE_ON    90
#define ANGLE_OFF    0
#define MOVE_DELAY   6
#define NUM_DIGITS   4
#define SEGS_PER_DIG 7

const int servoPins[NUM_DIGITS][SEGS_PER_DIG] = {
  { 2,  3,  4,  5,  6,  7,  8},  // Digit 0: giờ chục  (pin 2-8)
  { 9, 10, 11, 12, 13, 14, 15},  // Digit 1: giờ đơn   (pin 9-15)
  {22, 23, 24, 25, 26, 27, 28},  // Digit 2: phút chục (pin 22-28)
  {29, 30, 31, 32, 33, 34, 35}   // Digit 3: phút đơn  (pin 29-35)
};

// Thứ tự segment: a, b, c, d, e, f, g
//   aaa
//  f   b
//   ggg
//  e   c
//   ddd
const bool digitMap[10][SEGS_PER_DIG] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

Servo servos[NUM_DIGITS][SEGS_PER_DIG];
int currentDigits[NUM_DIGITS] = {-1, -1, -1, -1};
unsigned long lastPrintMs = 0;

// ===================== SETUP =====================
void setup() {
  Serial.begin(9600);
  delay(1000);

  if (!rtc.begin()) {
    Serial.println("Khong tim thay DS3231!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC mat nguon, dang dat gio theo luc nap code...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  for (int d = 0; d < NUM_DIGITS; d++) {
    for (int s = 0; s < SEGS_PER_DIG; s++) {
      servos[d][s].attach(servoPins[d][s]);
      servos[d][s].write(ANGLE_OFF);
    }
  }

  delay(1000);
  Serial.println("Kinetic Clock Ready!");
  Serial.println("Dat gio: T2026-03-25 19:30:00");
  // In header CSV
  Serial.println("Ngay,Gio,Gio_chuc,Gio_don,Phut_chuc,Phut_don,Uptime");
}

// ===================== LOOP =====================
void loop() {
  checkSerialSetTime();

  DateTime now = rtc.now();
  int h = now.hour();
  int m = now.minute();

  int digits[NUM_DIGITS] = {
    h / 10,
    h % 10,
    m / 10,
    m % 10
  };

  for (int d = 0; d < NUM_DIGITS; d++) {
    if (digits[d] != currentDigits[d]) {
      animateDigit(d, digits[d]);
      currentDigits[d] = digits[d];
    }
  }

  if (millis() - lastPrintMs >= 1000) {
    lastPrintMs = millis();
    // Xuất định dạng CSV: Ngay,Gio,Gio_chuc,Gio_don,Phut_chuc,Phut_don,Uptime
    printDate(now);   Serial.print(",");
    printTime(now);   Serial.print(",");
    Serial.print(now.hour() / 10); Serial.print(",");
    Serial.print(now.hour() % 10); Serial.print(",");
    Serial.print(now.minute() / 10); Serial.print(",");
    Serial.print(now.minute() % 10); Serial.print(",");
    printUptime(millis());
    Serial.println();
  }

  delay(50);
}

// ===================== HIỂN THỊ =====================
void animateDigit(int digitIndex, int value) {
  // Tắt segment không cần trước
  for (int s = 0; s < SEGS_PER_DIG; s++) {
    if (!digitMap[value][s]) {
      sweepServo(
        servos[digitIndex][s],
        getAngle(currentDigits[digitIndex], s),
        ANGLE_OFF
      );
    }
  }

  delay(20);

  // Bật segment cần thiết sau
  for (int s = 0; s < SEGS_PER_DIG; s++) {
    if (digitMap[value][s]) {
      sweepServo(
        servos[digitIndex][s],
        getAngle(currentDigits[digitIndex], s),
        ANGLE_ON
      );
    }
  }
}

int getAngle(int digitValue, int segIndex) {
  if (digitValue < 0) return ANGLE_OFF;
  return digitMap[digitValue][segIndex] ? ANGLE_ON : ANGLE_OFF;
}

void sweepServo(Servo &srv, int fromAngle, int toAngle) {
  if (fromAngle == toAngle) return;
  int step = (toAngle > fromAngle) ? 1 : -1;
  for (int pos = fromAngle; pos != toAngle; pos += step) {
    srv.write(pos);
    delay(MOVE_DELAY);
  }
  srv.write(toAngle);
}

// ===================== SERIAL SET TIME =====================
void checkSerialSetTime() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 20 && cmd[0] == 'T') {
      int yr = cmd.substring(1, 5).toInt();
      int mo = cmd.substring(6, 8).toInt();
      int dy = cmd.substring(9, 11).toInt();
      int hr = cmd.substring(12, 14).toInt();
      int mn = cmd.substring(15, 17).toInt();
      int sc = cmd.substring(18, 20).toInt();

      rtc.adjust(DateTime(yr, mo, dy, hr, mn, sc));
      Serial.println("Da cap nhat thoi gian!");
    } else {
      Serial.println("Sai dinh dang! Dung: T2026-03-25 19:30:00");
    }
  }
}

// ===================== IN NGÀY =====================
void printDate(const DateTime &dt) {
  if (dt.day()   < 10) Serial.print("0"); Serial.print(dt.day());   Serial.print("/");
  if (dt.month() < 10) Serial.print("0"); Serial.print(dt.month()); Serial.print("/");
  Serial.print(dt.year());
}

// ===================== IN GIỜ =====================
void printTime(const DateTime &dt) {
  if (dt.hour()   < 10) Serial.print("0"); Serial.print(dt.hour());   Serial.print(":");
  if (dt.minute() < 10) Serial.print("0"); Serial.print(dt.minute()); Serial.print(":");
  if (dt.second() < 10) Serial.print("0"); Serial.print(dt.second());
}

// ===================== IN NGÀY GIỜ =====================
void printDateTime(const DateTime &dt) {
  printDate(dt);
  Serial.print(" ");
  printTime(dt);
}

// ===================== IN UPTIME =====================
void printUptime(unsigned long ms) {
  unsigned long total = ms / 1000;
  unsigned long h = total / 3600;
  unsigned long m = (total % 3600) / 60;
  unsigned long s = total % 60;
  if (h < 10) Serial.print("0"); Serial.print(h); Serial.print(":");
  if (m < 10) Serial.print("0"); Serial.print(m); Serial.print(":");
  if (s < 10) Serial.print("0"); Serial.print(s);
}