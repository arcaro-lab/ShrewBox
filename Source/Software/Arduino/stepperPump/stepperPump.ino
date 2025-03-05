// constants won't change. Used here to set a pin number:
const int ledPin = LED_BUILTIN;  // the number of the LED pin

// Variables will change:
int ledState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long currentMicro = 0;
// constants won't change:
const long interval = 2;  // interval at which to blink (milliseconds)
int steps;
String action = "";
String inputString = "";
bool isStarted = false;
bool stringComplete = false; 
char dtm[100];
char inputChar[4];
void setup() {
  Serial.begin(57600);
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  printLine("Started");
  steps = 10;
}

void loop() {
  // here is where you'd put code that needs to be running all the time.

  if (stringComplete) {
        //Serial.println(inputString);
        if(inputString.indexOf("step") >= 0) {
            Serial.println("starting");
            printLine(inputString);
            processString(inputString);
            isStarted = true;
        }
        else if(inputString.indexOf("stop") >= 0) {
            isStarted = false;
            digitalWrite(ledPin, LOW);
        }
        // clear the string:
        inputString = "";
        stringComplete = false;
    }

    // if(isStarted) {
    //     currentMicro = micros (); 
    //     if(currentMicro-Last <= TimeBetween) return;
    //     Last=currentMicro;
    //     sinWave(CurLoop);
    //     // sawTooth();
    //     CurLoop++;
    //     if(CurLoop>= nmLoops) CurLoop=0; 
    // }
    if (steps > 0) {
      printLine("STEPS ",steps);
      while(steps !=0){
        singleStep();
        steps = steps - 1;
      }
      printLine("STEPS ",steps);
    }
    
}

void singleStep() {
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(400);
  digitalWrite(ledPin, LOW);
  delayMicroseconds(400);
}


//For Serial Function
/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

//For printLine
void printLine() {
    Serial.println();
}

void processString(String str)
{
  strcpy(dtm, str.c_str());
  sscanf(dtm, "%s %d", inputChar, &steps );
  printLine("STEPS ",steps);
}

template <typename T, typename... Types>
void printLine(T first, Types... other) {
    Serial.print(first);
    printLine(other...) ;
}
