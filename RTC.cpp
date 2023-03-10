// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <Wire.h>
#include "RTC.h"
#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#elif defined(ARDUINO_ARCH_SAMD)
// nothing special needed
#elif defined(ARDUINO_SAM_DUE)
 #define PROGMEM
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
 #define Wire Wire1
#endif



#if (ARDUINO >= 100)
 #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
 // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
 #define _I2C_WRITE write
 #define _I2C_READ  read
#else
 #include <WProgram.h>
 #define _I2C_WRITE send
 #define _I2C_READ  receive
#endif


static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}


////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

uint8_t DateTime::dayOfTheWeek() {    
    uint16_t day = date2days(tahun, bulan, tanggal);
    return ((day + 6) % 7) + 1; // Jan 1, 2000 is a Saturday, i.e. returns 7
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    detik = t % 60;
    t /= 60;
    menit = t % 60;
    t /= 60;
    jam = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (tahun = 0; ; ++tahun) {
        leap = tahun % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
		tahun + 2000;
    for (bulan = 1; ; ++bulan) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + bulan - 1);
        if (leap && bulan == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    tanggal = days + 1;
	hari = dayOfTheWeek(); 
	
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t dayOfWeek, uint8_t hour, uint8_t min, uint8_t sec) {
    // if (year >= 2000)
        // year -= 2000;
    tahun = year;
    bulan = month;
    tanggal = day;
    jam = hour;
    menit = min;
    detik = sec;
	hari = dayOfTheWeek(); 
}

DateTime::DateTime (const DateTime& copy):
  tahun(copy.tahun),
  bulan(copy.bulan),
  tanggal(copy.tanggal),
  jam(copy.jam),
  menit(copy.menit),
  detik(copy.detik),
  hari(copy.hari)
{}

static uint8_t conv2d(const char* p) {
    uint16_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    tahun = conv2d(date + 9) + 2000;
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
    switch (date[0]) {
        case 'J': bulan = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7); break;
        case 'F': bulan = 2; break;
        case 'A': bulan = date[2] == 'r' ? 4 : 8; break;
        case 'M': bulan = date[2] == 'r' ? 3 : 5; break;
        case 'S': bulan = 9; break;
        case 'O': bulan = 10; break;
        case 'N': bulan = 11; break;
        case 'D': bulan = 12; break;
    }
    tanggal = conv2d(date + 4);
    jam = conv2d(time);
    menit = conv2d(time + 3);
    detik = conv2d(time + 6);
	hari = dayOfTheWeek();
}

// A convenient constructor for using "the compiler's time":
// This version will save RAM by using PROGMEM to store it by using the F macro.
//   DateTime now (F(__DATE__), F(__TIME__));
DateTime::DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    char buff[11];
    memcpy_P(buff, date, 11);
    tahun = conv2d(buff + 9) + 2000;
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (buff[0]) {
        case 'J': bulan = (buff[1] == 'a') ? 1 : ((buff[2] == 'n') ? 6 : 7); break;
        case 'F': bulan = 2; break;
        case 'A': bulan = buff[2] == 'r' ? 4 : 8; break;
        case 'M': bulan = buff[2] == 'r' ? 3 : 5; break;
        case 'S': bulan = 9; break;
        case 'O': bulan = 10; break;
        case 'N': bulan = 11; break;
        case 'D': bulan = 12; break;
    }
    tanggal = conv2d(buff + 4);
    memcpy_P(buff, time, 8);
    jam = conv2d(buff);
    menit = conv2d(buff + 3);
    detik = conv2d(buff + 6);
	hari = dayOfTheWeek();
}


uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(tahun, bulan, tanggal);
  t = time2long(days, jam, menit, detik);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const {
  long t;
  uint16_t days = date2days(tahun, bulan, tanggal);
  t = time2long(days, jam, menit, detik);
  return t;
}

DateTime DateTime::operator+(const TimeSpan& span) {
  return DateTime(unixtime()+span.totalseconds());
}

DateTime DateTime::operator-(const TimeSpan& span) {
  return DateTime(unixtime()-span.totalseconds());
}

TimeSpan DateTime::operator-(const DateTime& right) {
  return TimeSpan(unixtime()-right.unixtime());
}

////////////////////////////////////////////////////////////////////////////////
// TimeSpan implementation

TimeSpan::TimeSpan (int32_t seconds):
  _seconds(seconds)
{}

TimeSpan::TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds):
  _seconds((int32_t)days*86400L + (int32_t)hours*3600 + (int32_t)minutes*60 + seconds)
{}

TimeSpan::TimeSpan (const TimeSpan& copy):
  _seconds(copy._seconds)
{}

TimeSpan TimeSpan::operator+(const TimeSpan& right) {
  return TimeSpan(_seconds+right._seconds);
}

TimeSpan TimeSpan::operator-(const TimeSpan& right) {
  return TimeSpan(_seconds-right._seconds);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

void RTC_DS1307::begin(void) {
  Wire.begin();
  Wire.setClock(100000);
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t detik = Wire._I2C_READ();
  return !(detik>>7);
}

void RTC_DS1307::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.detik));
  Wire._I2C_WRITE(bin2bcd(dt.menit));
  Wire._I2C_WRITE(bin2bcd(dt.jam));
  Wire._I2C_WRITE(dt.hari);
  Wire._I2C_WRITE(bin2bcd(dt.tanggal));
  Wire._I2C_WRITE(bin2bcd(dt.bulan));
  Wire._I2C_WRITE(bin2bcd(dt.tahun - 2000));
  Wire.endTransmission();
}

DateTime RTC_DS1307::now() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);	
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  uint8_t detik = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t menit = bcd2bin(Wire._I2C_READ());
  uint8_t jam = bcd2bin(Wire._I2C_READ());
  uint8_t hari = Wire._I2C_READ();
  uint8_t tanggal = bcd2bin(Wire._I2C_READ());
  uint8_t bulan = bcd2bin(Wire._I2C_READ());
  uint16_t tahun = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (tahun, bulan, tanggal, hari, jam, menit, detik);
}

Ds1307SqwPinMode RTC_DS1307::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds1307SqwPinMode>(mode);
}

void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}

void RTC_DS1307::readnvram(uint8_t* buf, uint8_t size, uint8_t address) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t) DS1307_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = Wire._I2C_READ();
  }
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t* buf, uint8_t size) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    Wire._I2C_WRITE(buf[pos]);
  }
  Wire.endTransmission();
}

uint8_t RTC_DS1307::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime& dt) {
    offset = dt.unixtime() - millis() / 1000;
}

DateTime RTC_Millis::now() {
  return (uint32_t)(offset + millis() / 1000);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RTC_PCF8563 implementation

void RTC_PCF8523::begin(void) {
  Wire.begin();
  Wire.setClock(100000);
}

boolean RTC_PCF8523::initialized(void) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t detik = Wire._I2C_READ();
  return ((detik & 0xE0) != 0xE0);
}

void RTC_PCF8523::adjust(const DateTime& dt) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3); // start at location 3
  Wire._I2C_WRITE(bin2bcd(dt.detik));
  Wire._I2C_WRITE(bin2bcd(dt.menit));
  Wire._I2C_WRITE(bin2bcd(dt.jam));
  Wire._I2C_WRITE(bin2bcd(dt.tanggal));
  Wire._I2C_WRITE(dt.hari);
  Wire._I2C_WRITE(bin2bcd(dt.bulan));
  Wire._I2C_WRITE(bin2bcd(dt.tahun - 2000));
  Wire.endTransmission();

  // set to battery switchover mode
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire._I2C_WRITE((byte)0x00);
  Wire.endTransmission();
}

DateTime RTC_PCF8523::now() {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3);	
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 7);
  uint8_t detik = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t menit = bcd2bin(Wire._I2C_READ());
  uint8_t jam = bcd2bin(Wire._I2C_READ());
  uint8_t tanggal = bcd2bin(Wire._I2C_READ());
  uint8_t hari = Wire._I2C_READ();
  uint8_t bulan = bcd2bin(Wire._I2C_READ());
  uint16_t tahun = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (tahun, bulan, tanggal, hari, jam, menit, detik);
}

Pcf8523SqwPinMode RTC_PCF8523::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)PCF8523_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode >>= 3;
  mode &= 0x7;
  return static_cast<Pcf8523SqwPinMode>(mode);
}

void RTC_PCF8523::writeSqwPinMode(Pcf8523SqwPinMode mode) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire._I2C_WRITE(mode << 3);
  Wire.endTransmission();
}




////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

void RTC_DS3231::begin(void) {
  Wire.begin();
  Wire.setClock(100000);
}

bool RTC_DS3231::lostPower(void) {
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

void RTC_DS3231::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Serial.println(dt.detik);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.detik));
  Wire._I2C_WRITE(bin2bcd(dt.menit));
  Wire._I2C_WRITE(bin2bcd(dt.jam));
  Wire._I2C_WRITE(dt.hari);
  Wire._I2C_WRITE(bin2bcd(dt.tanggal));
  Wire._I2C_WRITE(bin2bcd(dt.bulan));
  Wire._I2C_WRITE(bin2bcd(dt.tahun - 2000));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
}

DateTime RTC_DS3231::now() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0);	
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  uint8_t detik = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t menit = bcd2bin(Wire._I2C_READ());
  uint8_t jam = bcd2bin(Wire._I2C_READ());
  uint8_t hari = Wire._I2C_READ();
  uint8_t tanggal = bcd2bin(Wire._I2C_READ());
  uint8_t bulan = bcd2bin(Wire._I2C_READ());
  uint16_t tahun = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (tahun, bulan, tanggal, hari, jam, menit, detik);
}

Ds3231SqwPinMode RTC_DS3231::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds3231SqwPinMode>(mode);
}

void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  if (mode == DS3231_OFF) {
    ctrl |= 0x04; // turn on INTCN
  } else {
    ctrl |= mode;
  } 
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);

  //Serial.println( read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), HEX);
}
