// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTC_Semesin.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][7] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};

void setup () {

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  Serial.begin(9600);

  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Modul RTC tidak ditemukan");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 3, 0, 0));
  }
}

void loop () {
    DateTime now = rtc.now();
    
    Serial.print(now.tahun, DEC);
    Serial.print('/');
    Serial.print(now.bulan, DEC);
    Serial.print('/');
    Serial.print(now.tanggal, DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.hari-1]);
    Serial.print(") ");
    Serial.print(now.jam, DEC);
    Serial.print(':');
    Serial.print(now.menit, DEC);
    Serial.print(':');
    Serial.print(now.detik, DEC);
    Serial.println();
    
    Serial.print(" sejak 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
    
    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now + TimeSpan(7,12,30,6));
    
    Serial.print(" Sekarang + 7d + 30s: ");
    Serial.print(future.tahun, DEC);
    Serial.print('/');
    Serial.print(future.bulan, DEC);
    Serial.print('/');
    Serial.print(future.tanggal, DEC);
    Serial.print(' ');
    Serial.print(future.jam, DEC);
    Serial.print(':');
    Serial.print(future.menit, DEC);
    Serial.print(':');
    Serial.print(future.detik, DEC);
    Serial.println();
    
    Serial.println();
    delay(3000);
}