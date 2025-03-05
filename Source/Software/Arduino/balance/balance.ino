#include <TeensyLoadCell.h>
#include <TeensyView.h>  // Include the SFE_TeensyView library

///////////////////////////////////
// TeensyView Object Declaration //
///////////////////////////////////
//Standard
#define PIN_RESET 15
#define PIN_DC    5
#define PIN_CS    10
#define PIN_SCK   13
#define PIN_MOSI  11

#define PIN_TARE  3

using namespace TeensyLoadcell;

ADC adc;
Loadcell loadcell(adc.adc0);
TeensyView oled(PIN_RESET, PIN_DC, PIN_CS, PIN_SCK, PIN_MOSI);
float maxVal = 0;
float currentVal = 0;
bool isRunning = false;
elapsedMillis stopwatch = 0;

#define HWSERIAL Serial

void setup()
{
  //while (!Serial);

  pinMode(PIN_TARE, INPUT_PULLUP);

  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.display();  // Display what's in the buffer (splashscreen)
  oled.setFontType(1);  // Set font to type 1
  Serial.begin(9600);
  Serial.println("Enter s to start/stop, t to tare");

  loadcell.setTau(0.2);
  loadcell.c1 = 4.52f; // 5.4f;
  
  loadcell.start();
  delay(5000);
  loadcell.tare();
  maxVal = 0;
  isRunning = !isRunning;
}



void loop()
{
  int c = Serial.read();
  switch (c)
  {
    case 't':
      maxVal = 0;
      loadcell.tare();
      break;

    case 's':
      isRunning = !isRunning;
      break;
  }

  if (digitalRead(PIN_TARE) == 0)
  {
    maxVal = 0;
    loadcell.tare();
  }

  if (stopwatch > 250 && isRunning)
  {
    updateDisplay();
    // Serial.printf("%5.0f\n", loadcell.getValue());
    stopwatch = 0;
  }
}

void updateDisplay() 
{
  currentVal = abs(loadcell.getValue());
  Serial.printf("Weight %5.0f\n", currentVal);

  if (currentVal > 1)
  {
    if (currentVal > maxVal)
    {
      // Serial.printf("max val updated");
      maxVal = currentVal;
    }
  }

  char buffA[15];
  sprintf(buffA, "Weight: %5.0fg", currentVal);
  oled.setCursor(0, 0);
  oled.clear(PAGE);
  oled.print(buffA);
  sprintf(buffA, "Max:    %5.0fg", maxVal);
  oled.setCursor(0, 15);
  oled.print(buffA);
  oled.display();

}
