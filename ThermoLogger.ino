#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <FastCRC_tables.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <Arduino.h>
#include <DB.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <ThermoLogger.h>

#define EEPROM_ADDR 1

const int tmp102Address = 0x48;

const short ledGreen = 7;
const short ledPurple = 6;
const short pwrLed = 13;

const short printPin = 2;
const short flushPin = 3;

const long  delayTime = 5;
unsigned long previousTime = 0;
unsigned long curTime = 0;

short DelayCount = 1800;

DB db;
RTC_DS1307 RTC;
DateTime now;

struct Record {
  unsigned long time;
  float temp;
  unsigned short checkSum;
} record;

void setup(){

  //Setup Functions
  setupPins();
  setupSerial();
  setupRTC();
  setupTimer1();
  db.open(EEPROM_ADDR);
  Serial.print("Restoring ");
  Serial.print(db.nRecs());
  Serial.println(" records");
  //Enable Inturrupts
  attachInterrupt(0, isr_printRecords, LOW);
  attachInterrupt(1, isr_flushRecords, LOW);
  delay(250);
}

void loop()
{
  if (checkTime()) {
    checkSensors();
  }
  sleepnow();
}

void checkSensors()
{
  digitalWrite(ledGreen, HIGH);
  record.time = now.unixtime();
  record.temp = getTemperature();
  record.checkSum = checkSum();
  db.append(DB_REC record);
  delay(250);
  digitalWrite(ledGreen, LOW);
}

int checkSum()
{
  // todo: crc16? 
  return 0;
}

ISR(TIMER1_COMPA_vect)
{
  sleep_disable();
  digitalWrite(pwrLed, HIGH);
}

float getTemperature()
{
  digitalWrite(ledPurple, HIGH);
  Wire.requestFrom(tmp102Address,2);
  byte MSB = Wire.read();
  byte LSB = Wire.read();
  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4;
  float celsius = TemperatureSum*0.0625;
  float fahrenheit = (1.8 * celsius) + 32;
  delay(250);
  digitalWrite(ledPurple, LOW);
  
  return fahrenheit;
}

void isr_printRecords()
{
  detachInterrupt(0);
  digitalWrite(ledPurple, !HIGH);
  printRecords();
  digitalWrite(ledPurple, !LOW);
  attachInterrupt(0, isr_printRecords, LOW);
}

void isr_flushRecords()
{
  detachInterrupt(1);
  digitalWrite(ledPurple, !HIGH);
  digitalWrite(ledGreen, !HIGH);

  Serial.println("Flushing Database");
  db.create(EEPROM_ADDR,sizeof(record));

  digitalWrite(ledPurple, !LOW);
  digitalWrite(ledGreen, !LOW);
  attachInterrupt(1, isr_flushRecords, LOW);
}

void printRecords()
{
  int recnum;
  Serial.println();
  Serial.println("time, temp, checkSum");

  for(recnum=1;recnum<=(db.nRecs());recnum++)
  {
    db.read(recnum, DB_REC record);
    Serial.print(record.time);
    Serial.print(",");
    Serial.print(record.temp);
    Serial.print(",");
    Serial.print(record.checkSum);
    Serial.println();
  }
  Serial.println();
}

void setupTimer1()
{
  /// set entire TCCR1A register to 0 to start fresh
  TCCR1A = 0x0;
  // same for TCCR1B
  TCCR1B = 0x0;
  //initialize counter value to 0 for starting point
  TCNT1  = 0x0;
  // set compare match register **(all registers are 16bit on this chip)**
  OCR1A = 0x7FFF;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 (the largest) prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt (called at TIMER1_COMPA_vect vector)
  TIMSK1 |= (1 << OCIE1A);
}

void setupRTC()
{
  int j;
  for(j=0;j<5;j++)
  {
    if (RTC.isrunning()) 
    {
      return;
    }
    else
    {
      delay(250);
      Serial.println("RTC is NOT running!");
      Serial.print("Setting Time to: ");
      RTC.begin();
      delay(250);
      Serial.print(__DATE__);
      Serial.println(__TIME__);
      delay(250);
      RTC.adjust(DateTime(__DATE__, __TIME__));
      Serial.print("Set Time to: ");
    }
  }
  Serial.println("Failed to set clock... resetting device");
  delay(1000);
  resetFunc();
}


void setupPins()
{
  pinMode(ledGreen, OUTPUT);
  pinMode(ledPurple, OUTPUT);
  pinMode(pwrLed, OUTPUT);
  pinMode(printPin, INPUT_PULLUP);
  pinMode(flushPin, INPUT_PULLUP);
  digitalWrite(ledPurple, LOW);
  digitalWrite(ledGreen, LOW);
}

void setupSerial()
{
  Serial.begin(9600);
  while (!Serial) {;}
  Wire.begin();
}

void sleepnow()
{
  digitalWrite(pwrLed, LOW);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledPurple, HIGH);
  delay(250);
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_mode();
}

void print2digits(int number) 
{
  if (number >= 0 && number < 10)
  {
    Serial.write('0');
  }
  Serial.print(number);
}

bool checkTime()
{
  now = RTC.now();
  curTime = now.unixtime();
  delay(250);
  if (delayTime <= (curTime - previousTime))
  {
    previousTime = curTime;
    return true;
  }
  else 
  {
    return false;
  }
}