#include "Adafruit_FreeTouch.h"
// Include the Wire library for I2C
#include <Wire.h>

Adafruit_FreeTouch qt_1 = Adafruit_FreeTouch(A1, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_2 = Adafruit_FreeTouch(A6, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_3 = Adafruit_FreeTouch(A10, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_4 = Adafruit_FreeTouch(A7, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_5 = Adafruit_FreeTouch(A8, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_6 = Adafruit_FreeTouch(A9, OVERSAMPLE_16, RESISTOR_0, FREQ_MODE_NONE);

int BaseLine[6] = {0,0,0,0,0,0}; 
int Results[6] = {0,0,0,0,0,0}; 
bool sendF = false;
// int qt1, qt2, qt3, qt4, qt5, qt6;
void setup() {
  Serial.begin(115200);
  
  //while (!Serial);
  Serial.println("FreeTouch test");
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // Join I2C bus as slave with address 8
  Wire.begin(0x8);
  
  // Call receiveEvent when data received                
  Wire.onRequest(recRequest);
  // Wire.onReceive(handler);
  if (! qt_1.begin())  
    Serial.println("Failed to begin qt on pin A1");
  if (! qt_2.begin())  
    Serial.println("Failed to begin qt on pin A6");
  if (! qt_3.begin())  
    Serial.println("Failed to begin qt on pin A7");
  if (! qt_4.begin())  
    Serial.println("Failed to begin qt on pin A8");
  if (! qt_5.begin())  
     Serial.println("Failed to begin qt on pin A9");
  if (! qt_6.begin())  
     Serial.println("Failed to begin qt on pin A10");

  BaseLine[0] = qt_1.measure();
  BaseLine[1] = qt_2.measure(); 
  BaseLine[2] = qt_3.measure(); 
  BaseLine[3] = qt_4.measure(); 
  BaseLine[4] = qt_5.measure(); 
  BaseLine[5] = qt_6.measure(); 


}



void loop() {
  int counter, result = 0;
  
  // DIY
  // Serial.println("\n*************************************");

  counter = millis();
  Results[0] = qt_1.measure() - BaseLine[0];
  // qt1 = qt_1.measure();
  // Serial.print("QT1:"); Serial.print(Results[0]); Serial.print(",");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");

  counter = millis();
  Results[1] = qt_2.measure() - BaseLine[1];
  // qt2 = qt_2.measure();
  // Serial.print("QT2:"); Serial.print(Results[1]); Serial.print(",");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");

  counter = millis();
  Results[2] = qt_3.measure() - BaseLine[2];
  // qt3 = qt_3.measure();
  // Serial.print("QT3:"); Serial.print(Results[2]); Serial.print(",");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");
  
  counter = millis();
  Results[3] = qt_4.measure() - BaseLine[3];
  // qt4 = qt_4.measure();
  // Serial.print("QT4:"); Serial.print(Results[3]); Serial.print(",");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");

  counter = millis();
  Results[4] = qt_5.measure() - BaseLine[4];
  // qt5 = qt_5.measure();
  // Serial.print("QT5:"); Serial.print(Results[4]); Serial.print(",");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");

  counter = millis();
  Results[5] = qt_6.measure() - BaseLine[5];
  // qt6 = qt_6.measure();
  // Serial.print("QT6:"); Serial.print(Results[5]); Serial.print("\r\n");
  // Serial.print(" (");  Serial.print(millis() - counter); Serial.println(" ms)");

  // Serial.print("QT1:"); Serial.print(Results[0]); Serial.print(",");
  // Serial.print("QT2:"); Serial.print(Results[1]); Serial.print(",");
  // Serial.print("QT3:"); Serial.print(Results[2]); Serial.print(",");
  // Serial.print("QT4:"); Serial.print(Results[3]); Serial.print(",");
  // Serial.print("QT5:"); Serial.print(Results[4]); Serial.print(",");
  // Serial.print("QT6:"); Serial.print(Results[5]); Serial.print("\r\n");
  delay(20);
}

void recRequest() { 
    Serial.println("I2C recRequest");
    int val = 0;
    uint8_t buffer[12];
    // val = qt_1.measure(); 
    buffer[0] = highByte(Results[0]);
    buffer[1] = lowByte(Results[0]);
    // val = qt_2.measure(); 
    buffer[2] = highByte(Results[1]);
    buffer[3] = lowByte(Results[1]); 
    // val = qt_3.measure(); 
    buffer[4] = highByte(Results[2]);
    buffer[5] = lowByte(Results[2]);
    // val = qt_4.measure(); 
    buffer[6] = highByte(Results[3]);
    buffer[7] = lowByte(Results[3]); 
    // val = qt_5.measure(); 
    buffer[8] = highByte(Results[4]);
    buffer[9] = lowByte(Results[4]); 
    // val = qt_6.measure(); 
    buffer[10] = highByte(Results[5]);
    buffer[11] = lowByte(Results[5]); 
    Serial.println(Results[5]);
    Wire.write(buffer, 12);
} 

void handler(int howMany) {
    Serial.println("I2C recWrite");
    byte message = Wire.read();
    Serial.println(message);
}