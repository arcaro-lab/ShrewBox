#include <Arduino.h>
#include <Adafruit_TinyUSB.h>  // OK to keep if your board uses TinyUSB (nRF52 Feather etc.)

// ===================== Pin assignments =====================
// Stepper 1
const int S1_DIR  = D4;
const int S1_STEP = D5;
const int S1_EN   = D6;

// Stepper 2
const int S2_DIR  = D2;
const int S2_STEP = D13;
const int S2_EN   = D3;

// Stepper 3
const int S3_DIR  = D0;
const int S3_STEP = D11;
const int S3_EN   = D1;

// Toggle inputs (active LOW with INPUT_PULLUP)
const int S1_LEFT  = D19;
const int S1_RIGHT = D7;

const int S2_LEFT  = D18;
const int S2_RIGHT = D8;

const int S3_LEFT  = D10;
const int S3_RIGHT = D9;

// ===================== Motion defaults =====================
// steps = number of step pulses
// sp_us = delayMicroseconds used for HIGH and LOW (half-period)
// Example: sp_us=400 => 800us per step => 1250 steps/sec (approx)
int defaultSteps = 100;
int defaultSpUs  = 400;

// ===================== Stepper description table =====================
struct StepperPins {
  int dir;
  int step;
  int en;
};

StepperPins motors[3] = {
  { S1_DIR, S1_STEP, S1_EN },
  { S2_DIR, S2_STEP, S2_EN },
  { S3_DIR, S3_STEP, S3_EN }
};

// ===================== Serial line input buffer =====================
static const size_t LINE_BUF_LEN = 64;
char lineBuf[LINE_BUF_LEN];
size_t lineLen = 0;

// --------------------- Helper: printLine ---------------------
template<typename T, typename... Types>
void printLine(T first, Types... other) {
  Serial.print(first);
  printLine(other...);
}
void printLine() { Serial.println(); }

// --------------------- Stepper low-level helpers ---------------------
void enableMotor(uint8_t idx, bool enable) {
  // Many TMC2209 breakouts use EN as active LOW.
  // Your original code sets EN LOW to enable, so we keep that.
  digitalWrite(motors[idx].en, enable ? LOW : HIGH);
}

void setDir(uint8_t idx, uint8_t dir01) {
  // Match your existing button behavior:
  // RIGHT used DIR=LOW, LEFT used DIR=HIGH.
  // Here: dir=0 (CW) -> LOW, dir=1 (CCW) -> HIGH.
  digitalWrite(motors[idx].dir, (dir01 == 0) ? LOW : HIGH);

  // If you need to flip CW/CCW meaning, swap the LOW/HIGH above.
}

void stepMotor(uint8_t idx, long steps, int sp_us) {
  if (idx >= 3) return;
  if (steps <= 0) return;
  if (sp_us < 2) sp_us = 2; // avoid too-small delays

  // Blocking pulse train (simple + reliable)
  for (long i = 0; i < steps; i++) {
    digitalWrite(motors[idx].step, HIGH);
    delayMicroseconds(sp_us);
    digitalWrite(motors[idx].step, LOW);
    delayMicroseconds(sp_us);
  }
}

// ===================== Serial command parsing =====================
// Supported commands:
//
//   S1,0,100        -> motor 1, dir=0, steps=100 (uses defaultSpUs)
//   S2,1,200,300    -> motor 2, dir=1, steps=200, sp_us=300
//
// Also supports whitespace, and lowercase "s1" etc.

void handleCommand(char* cmd) {
  // Trim leading spaces
  while (*cmd == ' ' || *cmd == '\t' || *cmd == '\r' || *cmd == '\n') cmd++;
  if (*cmd == 0) return;

  // Quick help
  if (strcasecmp(cmd, "HELP") == 0) {
    printLine("Commands:");
    printLine("  S1,0,100         (motor 1, dir 0=CW, steps 100)");
    printLine("  S2,1,200,400     (motor 2, dir 1=CCW, steps 200, sp_us 400)");
    printLine("  POS?             (not implemented here)");
    return;
  }

  // Expect "S<id>,<dir>,<steps>[,<sp_us>]"
  // Example: "S1,0,100"
  if (tolower(cmd[0]) != 's') {
    printLine("ERR: command must start with S (try HELP)");
    return;
  }

  // Parse motor number after 'S'
  int motorId = cmd[1] - '0';   // '1'..'3'
  if (motorId < 1 || motorId > 3) {
    printLine("ERR: motor must be S1..S3");
    return;
  }
  uint8_t idx = (uint8_t)(motorId - 1);

  // Find first comma
  char* p = strchr(cmd, ',');
  if (!p) { printLine("ERR: format S1,dir,steps[,sp_us]"); return; }
  p++; // move past comma

  // Tokenize remaining fields by comma
  // dir
  char* tokDir = p;
  char* tokSteps = strchr(tokDir, ',');
  if (!tokSteps) { printLine("ERR: missing steps"); return; }
  *tokSteps = 0;
  tokSteps++;

  // steps
  char* tokSp = strchr(tokSteps, ',');
  int sp_us = defaultSpUs;
  if (tokSp) {
    *tokSp = 0;
    tokSp++;
    sp_us = atoi(tokSp);
  }

  int dir01 = atoi(tokDir);
  long steps = atol(tokSteps);

  if (!(dir01 == 0 || dir01 == 1)) {
    printLine("ERR: dir must be 0 or 1");
    return;
  }
  if (steps <= 0) {
    printLine("ERR: steps must be > 0");
    return;
  }

  // Execute
  enableMotor(idx, true);
  setDir(idx, (uint8_t)dir01);
  stepMotor(idx, steps, sp_us);

  printLine("OK: S", motorId, " dir=", dir01, " steps=", steps, " sp_us=", sp_us);
}

// Read serial bytes into a line buffer; process on newline
void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    // End of line -> process
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        lineBuf[lineLen] = 0;  // null-terminate
        handleCommand(lineBuf);
        lineLen = 0;
      }
      continue;
    }

    // Normal char
    if (lineLen < (LINE_BUF_LEN - 1)) {
      lineBuf[lineLen++] = c;
    } else {
      // overflow: reset
      lineLen = 0;
      printLine("ERR: line too long");
    }
  }
}

// ===================== Setup / Loop =====================
void setup() {
  Serial.begin(57600);
  while (!Serial) { delay(10); }

  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, LOW);

  // Motor pins
  for (int i = 0; i < 3; i++) {
    pinMode(motors[i].dir, OUTPUT);
    pinMode(motors[i].step, OUTPUT);
    pinMode(motors[i].en, OUTPUT);

    // Enable by default (your original behavior)
    enableMotor(i, true);
  }

  // Buttons
  pinMode(S1_LEFT,  INPUT_PULLUP);
  pinMode(S1_RIGHT, INPUT_PULLUP);
  pinMode(S2_LEFT,  INPUT_PULLUP);
  pinMode(S2_RIGHT, INPUT_PULLUP);
  pinMode(S3_LEFT,  INPUT_PULLUP);
  pinMode(S3_RIGHT, INPUT_PULLUP);

  printLine("Started. Type HELP. Serial format: S1,0,100 or S2,1,200,400");
}

void loop() {
  pollSerial();

  // ---------- Existing button behavior ----------
  if (digitalRead(S1_LEFT) == LOW) {
    digitalWrite(motors[0].dir, HIGH);
    stepMotor(0, defaultSteps, defaultSpUs);
    printLine("BTN: S1_LEFT");
  } else if (digitalRead(S1_RIGHT) == LOW) {
    digitalWrite(motors[0].dir, LOW);
    stepMotor(0, defaultSteps, defaultSpUs);
    printLine("BTN: S1_RIGHT");
  }

  if (digitalRead(S2_LEFT) == LOW) {
    digitalWrite(motors[1].dir, HIGH);
    stepMotor(1, defaultSteps, defaultSpUs);
    printLine("BTN: S2_LEFT");
  } else if (digitalRead(S2_RIGHT) == LOW) {
    digitalWrite(motors[1].dir, LOW);
    stepMotor(1, defaultSteps, defaultSpUs);
    printLine("BTN: S2_RIGHT");
  }

  if (digitalRead(S3_LEFT) == LOW) {
    digitalWrite(motors[2].dir, HIGH);
    stepMotor(2, defaultSteps, defaultSpUs);
    printLine("BTN: S3_LEFT");
  } else if (digitalRead(S3_RIGHT) == LOW) {
    digitalWrite(motors[2].dir, LOW);
    stepMotor(2, defaultSteps, defaultSpUs);
    printLine("BTN: S3_RIGHT");
  }
}
