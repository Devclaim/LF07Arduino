//www.elegoo.com
//2018.10.25

#include <IRremote.hpp>
#include <LiquidCrystal.h>
#include <dht_nonblocking.h>
#define DHT_SENSOR_TYPE DHT_TYPE_11
#define PHOTORESISTOR_PIN A5
#define WATERLEVEL_PIN A0

// Temp/ Humidity Sensor
static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );
float temperature;
float humidity;

// LCD Display
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int currentPage = 0;
int maxPages = 3;

// Water Level Detector
int waterLevelValue = 0;

//IR Reciever
IRrecv irrecv(3);
uint32_t last_decodedRawData = 0;

// Warning LEDS
int warningLevel = 0;
int redLed = 4;
int yellowLed = 5;
int greenLed = 6;
String warningMsg = "";
bool warnOnLCD = false;

// Luminosity
int luminosity = 0;

/*
 * Initialize the serial port.
 */
void setup( )
{
  Serial.begin(9600);

  // init lcd
  lcd.begin(16, 2);

  // init receiver
  irrecv.enableIRIn();

  // init pins
  pinMode(redLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(WATERLEVEL_PIN, INPUT);
}

void translateIR()
{
  if (irrecv.decodedIRData.flags)
  {
    irrecv.decodedIRData.decodedRawData = last_decodedRawData;
  }
  switch (irrecv.decodedIRData.decodedRawData)
  {
    case 0xF807FF00: 
      currentPageDown();
      break;
    case 0xF609FF00: 
      currentPageUp();
      break;
    case 0xE619FF00:
      warnOnLCD = !warnOnLCD;
      break;
    default:
      break;
  }
  last_decodedRawData = irrecv.decodedIRData.decodedRawData;
  delay(500); // Do not get immediate repeat
}

void currentPageUp() {
  if(currentPage < maxPages) {
    currentPage++;
  } else {
    currentPage = 0;
  }
  clearCurrentPage();
}

void currentPageDown() {
  if(currentPage > 0) {
    currentPage--;
  } else {
    currentPage = maxPages;
  }
  clearCurrentPage();
}

void clearCurrentPage(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Page: " + String(currentPage));
}

void renderCurrentPage(){
  switch(currentPage) {
    case(0):
      lcd.setCursor(0,0);
      lcd.print("Smart Garden");
      lcd.setCursor(0,1);
      lcd.print("Press UP or DOWN");
      break;
    case(1):
      lcd.setCursor(0, 0);
      lcd.print("Temp.: " + String((int)temperature) + "C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: " + String((int)humidity) + "%");
      break;
    case(2):
      lcd.setCursor(0, 0);
      lcd.print("Water Level: ");
      lcd.setCursor(0, 1);
      lcd.print(String(waterLevelValue));
      break;
    case(3):
      lcd.setCursor(0, 0);
      lcd.print("Luminosity: ");
      lcd.setCursor(0, 1);
      lcd.print(String(luminosity));
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("404");
      break;
  }
}

void lightWarningLed() {
  switch(warningLevel) {
    case(0):
      digitalWrite(greenLed, HIGH);
      digitalWrite(yellowLed, LOW);
      digitalWrite(redLed, LOW);
      if(warnOnLCD) {
        lcd.setCursor(0,0);
        lcd.print("No Problems :)");
      }
      break;
    case(1):
      digitalWrite(greenLed, LOW);
      digitalWrite(yellowLed, HIGH);
      digitalWrite(redLed, LOW);
      if(warnOnLCD) {
        lcd.setCursor(0, 0);
        lcd.print("Warning ");
        lcd.setCursor(0, 1);
        lcd.print(warningMsg);
      }
      break;  
    case(2):
      digitalWrite(greenLed, LOW);
      digitalWrite(yellowLed, LOW);
      digitalWrite(redLed, HIGH);
      if(warnOnLCD) {
        lcd.setCursor(0, 0);
        lcd.print("CRITICAL!!! ");
        lcd.setCursor(0, 1);
        lcd.print(warningMsg); 
      }
      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("No Problems :)");
      break;
  }
}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );

  /* Measure once every second. */
  if( millis( ) - measurement_timestamp > 4000ul )
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      waterLevelValue = analogRead(WATERLEVEL_PIN);
      luminosity = analogRead(PHOTORESISTOR_PIN);

      Serial.println(String(*temperature) + "," + String(*humidity) + "," + String(waterLevelValue) + "," + String(luminosity));
      return( true );
    }
  }

  return( false );
}

/*
 * Main program loop.
 */
void loop( )
{
  /* Measure temperature and humidity.  If the functions returns
     true, then a measurement is available. */
  if( measure_environment( &temperature, &humidity ) == true ) {
    if(humidity < 30) {
      warningLevel = 1;
      warningMsg = "Humidity low";
    } else if(humidity > 60) {
      warningLevel = 1;
      warningMsg = "Humidity high";
    } else if(temperature >= 30) {
      warningLevel = 2;
      warningMsg = "Temps. HIGH";
    } else if(temperature <= 20) {
      warningLevel = 2;
      warningMsg = "Temps. LOW";
    } else if(waterLevelValue >= 300){
      warningLevel = 2;
      warningMsg = "WaterLevels HIGH";
    } else if(luminosity > 400){
      warningLevel = 1;
      warningMsg = "Luminosity high";
    } else if(luminosity < 50){
      warningLevel = 2;
      warningMsg = "Luminosity LOW";
    } else {
      warningLevel = 0;
    }
    lcd.clear();
  }
  
  // decode signal from receiver
  if (irrecv.decode())
  {
    translateIR();
    irrecv.resume(); // receive the next value
  }

  if(!warnOnLCD) {
     renderCurrentPage();
  }
  lightWarningLed();
}
