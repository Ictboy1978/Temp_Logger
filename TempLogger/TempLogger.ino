/*
Real time clock, calendar, temperature, humidity data logger using Arduino, DS3231 and DHT22 sensor with SD card

Set date and time:
Syymmddhhmm - Example: 2019.11.10 15:30:00 ->  S1911101530

Set logging interval 000-999 in sec
I### - Example: Set to log every 25 sec -> I025
*/

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <EEPROM.h>
#include <dhtnew.h>
#include "types.h"
#include "ser_comm.h"

#define DHTPIN  2
#define iLED    LED_BUILTIN   //Built-in LED on NANO

DHTNEW dht(DHTPIN);

//save log every LOGINTERVAL sec / pre-defined value
#define LOGINTERVAL 5

File dataLog;
boolean sd_ok = 0;
char temperature[] = " 00.0";
char humidity[]    = " 00.0";
char Time[]     = "  :  :  ";
char Calendar[] = "20  /  /  ";

byte second, minute, hour, date, month, year, previous_second;
int Temp, RH;

u16 cnt;
u16 readinterval;
u8 rd[64];

//-------------------------------------------------------------------------------
void setup()
{
  pinMode(iLED, OUTPUT);

  Serial.begin(115200);
  Serial.print("Initializing SD card...");
  if (!SD.begin())
  {
    Serial.println("initialization failed!");
  }
  else
  {
    Serial.println("initialization done.");
    sd_ok = 1;
  }

  Wire.begin();                                  // Join i2c bus
  Serial.println("Date;Time;Temperature;Humidity");

  if (sd_ok)
  { // If SD card initialization was OK
    dataLog = SD.open("TempLog.txt", FILE_WRITE);    // Open file Logger.txt
    if (dataLog)
    { // if the file opened okay, write to it:
      dataLog.println("Date;Time;Temperature;Humidity");
      dataLog.close();                              // Close the file
    }
  }
}
//-------------------------------------------------------------------------------
void ReadClock()
{
    Wire.beginTransmission(0x68);                 // Start I2C protocol with DS3231 address
    Wire.write(0);                                // Send register address
    Wire.endTransmission(false);                  // I2C restart
    Wire.requestFrom(0x68, 7);                    // Request 7 bytes from DS3231 and release I2C bus at end of reading
    second = Wire.read();                         // Read seconds from register 0
    minute = Wire.read();                         // Read minuts from register 1
    hour   = Wire.read();                         // Read hour from register 2
    Wire.read();                                  // Read day from register 3 (not used)
    date   = Wire.read();                         // Read date from register 4
    month  = Wire.read();                         // Read month from register 5
    year   = Wire.read();                         // Read year from register 6
  
  // Convert BCD to decimal
  second = (second >> 4) * 10 + (second & 0x0F);
  minute = (minute >> 4) * 10 + (minute & 0x0F);
  hour   = (hour >> 4)   * 10 + (hour & 0x0F);
  date   = (date >> 4)   * 10 + (date & 0x0F);
  month  = (month >> 4)  * 10 + (month & 0x0F);
  year   = (year >> 4)   * 10 + (year & 0x0F);

  Time[7]     = second % 10 + 48;
  Time[6]     = second / 10 + 48;
  Time[4]      = minute % 10 + 48;
  Time[3]      = minute / 10 + 48;
  Time[1]      = hour   % 10 + 48;
  Time[0]      = hour   / 10 + 48;
  Calendar[3] = year   % 10 + 48;
  Calendar[2] = year   / 10 + 48;
  Calendar[6]  = month  % 10 + 48;
  Calendar[5]  = month  / 10 + 48;
  Calendar[9]  = date   % 10 + 48;
  Calendar[8]  = date   / 10 + 48;
}
//-------------------------------------------------------------------------------
void SetClock()
{
  // Convert decimal to BCD
  minute = ((minute / 10) << 4) + (minute % 10);
  hour = ((hour / 10) << 4) + (hour % 10);
  date = ((date / 10) << 4) + (date % 10);
  month = ((month / 10) << 4) + (month % 10);
  year = ((year / 10) << 4) + (year % 10);
  // Write data to DS3231 RTC
  Wire.beginTransmission(0x68);               // Start I2C protocol with DS3231 address
  Wire.write(0);                              // Send register address
  Wire.write(0);                              // Reset sesonds and start oscillator
  Wire.write(minute);                         // Write minute
  Wire.write(hour);                           // Write hour
  Wire.write(1);                              // Write day (not used)
  Wire.write(date);                           // Write date
  Wire.write(month);                          // Write month
  Wire.write(year);                           // Write year
  Wire.endTransmission();                     // Stop transmission and release the I2C bus
  delay(200);                                 // Wait 200ms
}
//-------------------------------------------------------------------------------
void Process_Serial()
{
  //Set date and time Syymmddhhmm
  if ((rd[0] == 'S') & (serlen > 9))
  {
    year   = 10 * (rd[1] - 48) + (rd[2] - 48);
    month  = 10 * (rd[3] - 48) + (rd[4] - 48);
    date   = 10 * (rd[5] - 48) + (rd[6] - 48);
    hour   = 10 * (rd[7] - 48) + (rd[8] - 48);
    minute = 10 * (rd[9] - 48) + (rd[10] - 48);
    SetClock();
    Serial.println("Ok");
  }

  //Set read interval I### in sec
  if ((rd[0]=='I') & (serlen==4))
  {
    readinterval = 100*(rd[1]-48) + 10*(rd[2]-48) +rd[3]-48;
    EEPROM.update(0, (readinterval >> 8) & 0xff);
    EEPROM.update(1, readinterval & 0xff);
    EEPROM.update(2, 'A');
    EEPROM.update(3, 'M');
    cnt=0;
    Serial.println("Ok");
  }

  //Reset EEPROM stored time interval value
  if((rd[0]=='R') & (rd[1]=='S') & (rd[2]=='T'))
  {
    EEPROM.update(2, '-');
    EEPROM.update(3, '-');
    Serial.println("Ok");
  }
}
//-------------------------------------------------------------------------------
void SetupEEprom()
{
  if ((EEPROM.read(2) == 'A') && (EEPROM.read(3) == 'M'))
  {
    readinterval = (EEPROM.read(0) << 8) + EEPROM.read(1);
  }
  else
  {
    EEPROM.update(0, (readinterval >> 8) & 0xff);
    EEPROM.update(1, readinterval & 0xff);
    EEPROM.update(2, 'A');
    EEPROM.update(3, 'M');
  }
}
//-------------------------------------------------------------------------------
void loop()
{
  cnt = 0;
  readinterval = LOGINTERVAL;
  SetupEEprom();
  
  while (1)
  {
    ReadClock();
    if (previous_second != second)
    {
      previous_second = second;
      cnt++;
    }

    if (cnt == readinterval)
    {
      cnt = 0;
      RH=0;
      Temp=0;
      for(u8 xc=0;xc<4;xc++)
      {
        dht.read();
        RH += 10 * dht.humidity;
        Temp += 10 * (dht.temperature -0.6);   //BUGFIX
        delay(50);
      }
      RH /= 4;
      Temp /=4;

      if (Temp < 0)
      {
        temperature[0] = '-';                     // If temperature < 0 put minus sign
        Temp = abs(Temp);                         // Absolute value of 'Temp'
      }
      else temperature[0] = ' ';                     // otherwise (temperature > 0) put space
      temperature[1]   = (Temp / 100) % 10  + 48;
      temperature[2]   = (Temp / 10)  % 10  + 48;
      temperature[4]  =  Temp % 10 + 48;

      if (RH >= 1000) humidity[0] = '1'; else humidity[0] = ' ';
      humidity[1]      = (RH / 100) % 10 + 48;
      humidity[2]      = (RH / 10) % 10 + 48;
      humidity[4]     =  RH % 10 + 48;

      // Send data to Arduino IDE serial monitor
      Serial.print(Calendar);
      Serial.print(";");
      Serial.print(Time);
      Serial.print(";");
      Serial.print(temperature);
      Serial.print(";");
      Serial.println(humidity);

      if (sd_ok)
      { // If SD card initialization was OK
        dataLog = SD.open("TempLog.txt", FILE_WRITE);    // Open file Logger.txt
        if (dataLog)
        { // if the file opened okay, write to it:
          dataLog.print(Calendar);
          dataLog.print(";");
          dataLog.print(Time);
          dataLog.print(";");
          dataLog.print(temperature);
          dataLog.print(";");
          dataLog.println(humidity);
          dataLog.close();                              // Close the file
        }
      }
    }
    ReadSerial(&rd[0], Process_Serial);
    delay(50); 
      
      //TOGGLE LED
      SPI.end();
      digitalWrite(iLED,1);
      delay(250);
      digitalWrite(iLED,0);
      delay(250);
      SPI.begin();
  }
}
//-------------------------------------------------------------------------------
