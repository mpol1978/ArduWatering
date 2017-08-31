
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "RTClibMod.h"

RTC_DS1307 rtc;

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  if (! rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");

    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop()
{
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print("days from 01/01/2000: ");
  uint16_t days_from_01_01_2000;
  days_from_01_01_2000 = date2days(now.year(), now.month(), now.day());
  Serial.println(days_from_01_01_2000, DEC);
  Serial.print("seconds from 01/01/2000: ");
  Serial.println(time2long(days_from_01_01_2000, now.hour(), now.minute(), now.second()));
  delay(3000);
}

