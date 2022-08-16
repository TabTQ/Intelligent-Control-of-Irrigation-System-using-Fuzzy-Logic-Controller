#include <Wire.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal.h>
#include <DHT.h>

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
Adafruit_INA219 ina219;

const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int motor_pin1 = 4;
int motor_pin2 = 5;
int ENA = 6;

int soil_moisture_sensor_pin = A0;

int relay = 2;

float Vo;
float R1 = 6770;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

int voltage_input_pin = A7;
float v_out = 0.0;
float v_in = 0.0;
float r1 = 13690.0; 
float r2 = 9430.0; 

union BtoF{
  byte b[16];
  float fval;
}u;

const int buffer_size = 16;
byte buf[buffer_size];

float myVal;
int motor_analog_value = 0;
int temp=0;

void setup() {
   Serial.begin(9600);
   pinMode(motor_pin1, OUTPUT);
   pinMode(motor_pin2, OUTPUT);
   pinMode(ENA, OUTPUT);
   pinMode(relay,OUTPUT);
   pinMode(voltage_input_pin, INPUT);
   digitalWrite(motor_pin1, HIGH);
   digitalWrite(motor_pin2, LOW);
   uint32_t currentFrequency;
   lcd.begin(16, 2);
   ina219.begin();
   dht.begin();
   delay(100);
   }

void loop() {
   transmitSerial();
   if(Serial.available()>0){
    myVal = readFromMatlab();
    adjustSpeed(myVal);
    lcd.setCursor(0, 0);
    lcd.print("Output Voltage:");
    lcd.setCursor(0, 1);
    lcd.print(measureVoltage());
   }
   else{
    lcd.setCursor(0, 0);
    lcd.print("Output Voltage:");
    lcd.setCursor(0, 1);
    lcd.print(measureVoltage());
   }
   delay(2000);
   lcd.clear();
  }
  
void adjustSpeed(float voltage){
  temp = 100;
  float volts = voltage*100; 
  int v = volts; 
  bool is_equal = false;
  while(!is_equal){
    delay(100);
    if(v>1080){
      analogWrite(ENA, 255);
      break;
    }
    else if(v<330){
      analogWrite(ENA, 0);
      break;
    }
    float myVolts = measureVoltage()*100;
    int myV = myVolts; 
    if(myV<(v-10)){
      temp = temp+1;
      analogWrite(ENA, temp);
    }
    else if(myV>(v+10)){
      temp = temp-1;
      analogWrite(ENA, temp);
    }
    else if((myV>=(v-10))&&(myV<=(v+10))){
      is_equal = true;
      break;
    }
    else{
      is_equal = false;
    }
  }
}

float readFromMatlab(){
  int reln = Serial.readBytesUntil("\r\n", buf, buffer_size);
  for(int i=0; i<buffer_size; i++){
    u.b[i] = buf[i];
  }
  float output = u.fval;
  return output;
}

float measureVoltage(){
  float sum = 0.0;
  for(int i=0;i<200;i++){
  int analog_voltage = analogRead(voltage_input_pin);
  v_out = (analog_voltage*5.06)/1024;
  v_in = v_out / (r2/(r1+r2));
  sum = sum + v_in;
  }
  v_in = sum/200;
  return v_in;
}

float measureSoilMoisture(){
  float soil_moisture_value= analogRead(soil_moisture_sensor_pin);
  soil_moisture_value = map(soil_moisture_value,1023,160,0,100);
  return soil_moisture_value;
  }

float measurePanelCurrent(){
  digitalWrite(relay, HIGH);
  delay(500);
  float current_mA = 0;
  
  float sum=0;
  for(int i=0;i<200;i++){
    current_mA = ina219.getCurrent_mA();
    sum = sum + current_mA;  
  }
  
  digitalWrite(relay, LOW);
  current_mA = sum/200;
  
  current_mA = current_mA/1000; //Ampere
  return current_mA;
   }
   
float measurePanelTemperature(){
   Vo = analogRead(A6);
   R2 = R1 * ((1023.0 / Vo) - 1.0);
   logR2 = log(R2);
   T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
   T = T - 273.15;
   T = (T * 9.0)/ 5.0 + 32.0-7.7;//Fahrenheit
   T = ((T-32)*5/9)+273.15;
   return T;
}

float measureAirTemperature(){
   float air_temperature = dht.readTemperature();
   if (isnan(air_temperature)) {
    measureAirTemperature();
       }
   return air_temperature;
   }
float measureAirHumidity(){
   float air_humidity = dht.readHumidity();
   if (isnan(air_humidity)) {
    measureAirHumidity();
       }
   return air_humidity;
   }

void transmitSerial(){
   float at = measureAirTemperature();
   float ah = measureAirHumidity();
   float sm = measureSoilMoisture();
   float pt = measurePanelTemperature();
   float pc = measurePanelCurrent();
   
   byte *b1 = (byte *) &sm;
   byte *b2 = (byte *) &pt;
   byte *b3 = (byte *) &pc;
   byte *b4 = (byte *) &at;
   byte *b5 = (byte *) &ah;
   
   Serial.write(b1,4);
   Serial.write(b2,4);
   Serial.write(b3,4);
   Serial.write(b4,4);
   Serial.write(b5,4);
   Serial.write(13);
   Serial.write(10);
  
   startLCD(sm,pt,pc,at,ah);
    }

void startLCD(float sm,float pt,float pc,float at,float ah){
  lcd.setCursor(0, 0);
  lcd.print("Soil Moisture : ");
  lcd.setCursor(0, 1);
  lcd.print(sm);lcd.print("%");
  delay(2000);
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Panel Temp :    ");
  lcd.setCursor(0, 1);
  lcd.print(pt);lcd.print("K");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Panel Current : ");
  lcd.setCursor(0, 1);
  lcd.print(pc);lcd.print("A");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Air Temp :      ");
  lcd.setCursor(0, 1);
  lcd.print(at);lcd.print("'C");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Air Humidity :  ");
  lcd.setCursor(0, 1);
  lcd.print(ah);lcd.print("%");
  delay(2000);
  lcd.clear();
}
