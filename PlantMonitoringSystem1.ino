//include used libraries
#include <WiFi.h>
#include "DHT.h"
#include <BlynkSimpleEsp32.h>

//define 
#define BLYNK_TEMPLATE_ID "TMPL6kADRwLAx"
#define BLYNK_TEMPLATE_NAME "PlantMonitoringSystem"
#define BLYNK_AUTH_TOKEN "DWTb_WZV6w-yhNRRb7KF-5DlXiaI5_NU"
#define BLYNK_PRINT Serial
//Wifi settings
#define SSID "Siong"
#define PASSWORD "12345678"
#define DHTPIN 16 //Digital humidity and temperature sensor pin
#define DHTTYPE DHT11   // DHT 11 

bool System_Auto = false; // fan and watering pipe control mode (manual or automatic)

//Pin declaration (using GP PIN)
const int WifiLedPin = 4;
const int fanPin = 33; 
const int wateringPin = 32; 
const int soilMoisturePin = A7;	/* Soil moisture sensor O/P pin */

//Variable declaration
long previousMillis = 0; 
long previousSensorCheckMillis = 0; 
const long WifiCheckTime = 2000; //every 2seconds
const long HumidityCheckTime = 1000; //every 1seconds

//Digital humidity and temperature sensor object declaration
DHT dht(DHTPIN, DHTTYPE);

//Sync the virtual pin when connected
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0);  // will cause BLYNK_WRITE(V0) to be executed
  Blynk.syncVirtual(V1);  // will cause BLYNK_WRITE(V1) to be executed
  Blynk.syncVirtual(V6);  // will cause BLYNK_WRITE(V6) to be executed
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Pin input/output setting 
  pinMode(WifiLedPin, OUTPUT);      // set the LED pin mode
  pinMode(fanPin, OUTPUT);      // set the LED pin mode
  pinMode(wateringPin, OUTPUT);      // set the wateringPin pin mode
  pinMode(soilMoisturePin, INPUT); // set soil moisture pin as input

  //default OFF ; My relay will on then the value is low or zero . will be off when High or 1.
  digitalWrite(fanPin, HIGH);  
  digitalWrite(wateringPin, HIGH); 
  //begin and initialized dht sensor  
  dht.begin();     

  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  

  //if the wifi connection failed , them excueted this while loop.
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      Serial.println("Try to connect..");

      //OUTPUT LOW MEAN TURN OFF LED
      digitalWrite(WifiLedPin, LOW);
  }
  // if the wifi successful connected, will skip above while loop and run this below code. 
  Serial.println("");
  Serial.println("WiFi connected.");
  digitalWrite(WifiLedPin, HIGH);
  Blynk.begin(BLYNK_AUTH_TOKEN, SSID, PASSWORD); 
}

 // Blynk button widget callback  , This is the AUTO AND MANUAL control in blynk.
BLYNK_WRITE(V6) 
  {
  int buttonState = param.asInt();
  // button is pressed
   if (buttonState == 1) 
    { 
      System_Auto = true; // toggle fan control mode
    }
    else 
    {
      System_Auto =false;
    }
  }


// Executes when the value of virtual pin 0 changes from webpage or mobile application
//In this control using Relay, it is active -LOW , and inactive -HIGH
BLYNK_WRITE(V0) 
{

// !System_Auto means manual/(true)  , when it is 'true'/manual , then will continue to execute this manual statement.
 if (!System_Auto)
  {  
    
    Serial.print("Blynk.Cloud is writing something to V0");
 // when bylnk side detect it is 1 means 'ON' , then will pass the 'Low' to the relay and on the fan, else will pass the 'HIGH' to off it.   
  if(param.asInt() == 1)
  {
    // execute this code if the switch widget is now ON
    digitalWrite(fanPin,LOW);  // On Fan   
  }
  else
  {
      digitalWrite(fanPin,HIGH);  // OFF Fan 
  }

  }
}

// Executes when the value of virtual pin 1 changes from webpage or mobile application
//In this control using Relay, it is active -LOW , and inactive -HIGH

BLYNK_WRITE(V1) 
{

if (!System_Auto)
  {  
  Serial.print("Blynk.Cloud is writing something to V1");
  if(param.asInt() == 1)
  {
    digitalWrite(wateringPin, LOW);  // On water
  }
  else
  {
    digitalWrite(wateringPin, HIGH);  // OFF water
  }

  }
} 

void loop() {
   //set a certain timing to keep checking the values of the humidity,temperature and soil moisture.
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  unsigned long currentSensorCheckMillis = millis();
  //printf("currentSensorCheckMillis :%lu\n",currentSensorCheckMillis);
  //printf("previousSensorCheckMillis :%lu\n",previousSensorCheckMillis);
  //read / upload sensor data every 1 seconds
  if(currentSensorCheckMillis - previousSensorCheckMillis >=HumidityCheckTime)
{
    int time = currentSensorCheckMillis - previousSensorCheckMillis;
    //printf("Time: %i\n", time);
    previousSensorCheckMillis = millis();
    float humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float temperatureCelsiues = dht.readTemperature();
    float moisture_percentage;
    int sensor_analog;

    //read soil moisture sensor reading
    sensor_analog = analogRead(soilMoisturePin);
    //Serial.println(sensor_analog);
    moisture_percentage = ( 100 - ( (sensor_analog/4095.00) * 100 ) );
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperatureCelsiues))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
    //print the sensor value in serial monitor
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperatureCelsiues);
    Serial.println(F("°C "));
    Serial.print("Moisture Percentage = ");
    Serial.print(moisture_percentage);
    Serial.print("%\n\n");
    
    //upload humidity and temperature to Blynk webpage/mobile application
    Blynk.virtualWrite(V2, humidity);
    Blynk.virtualWrite(V3, temperatureCelsiues);
    Blynk.virtualWrite(V4, moisture_percentage);
   // Blynk.virtualWrite(V5, temperatureCelsiues);

 


    //Default is false (Manual control), if change the boolean to 'true' by Blynk , will continue run the following statement.
  if (System_Auto)
  {

    // if temperature is higher than 30, auto switch on the fan till the temperature less than 30 then stop working
    //In this control using Relay, it is active -LOW , and inactive -HIGH
   if (temperatureCelsiues>30)
    {
      digitalWrite(fanPin, LOW);
      Blynk.virtualWrite(V0,1);
    } 
    else
    {
      digitalWrite(fanPin, HIGH);
      Blynk.virtualWrite(V0,0);
    }

  // if humidity is lower than 40, auto switch on the watering pipe till the humidity higher than 40 then stop working.
  //In this control using Relay, it is active -LOW , and inactive -HIGH
  if (moisture_percentage< 40 )
   {
    digitalWrite(wateringPin, LOW);
    Blynk.virtualWrite(V1,1);
    delay(1000);
    digitalWrite(wateringPin,HIGH);
    Blynk.virtualWrite(V1,0);
   } 
  else
   {
    digitalWrite(wateringPin, HIGH);
    Blynk.virtualWrite(V1,0);
   }
  }
}

  //check WIFI status every 2 seconds
  if (currentMillis - previousMillis >= WifiCheckTime)
  {
    previousMillis = millis();
    previousSensorCheckMillis = millis();
    if (WiFi.status() != WL_CONNECTED)
    {
      //turn off led if wifi not connected
      digitalWrite(WifiLedPin, LOW);
    }
    else
    {
        //turn on led if wifi connected
      digitalWrite(WifiLedPin, HIGH);
    }
  } 
}
