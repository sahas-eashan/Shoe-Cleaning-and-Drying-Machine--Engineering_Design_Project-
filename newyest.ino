#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h> // Include the DHT sensor library

#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 64 // OLED display height in pixels
#define OLED_RESET -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int b = 1; // for the switching

// Pin configuration
const int buttonPin = 2; // Digital pin connected to the push button
const int startButtonPin = 3;

// Debounce settings
const unsigned long debounceDelay = 50; // 50 milliseconds for debouncing

// State variables
bool lastButtonState = HIGH; // Default to HIGH (using a pull-down resistor)
unsigned long lastDebounceTime = 0; // Last time the button state changed
int currentMode = 1; // Start with Mode 1
int state;
int buttonState;

// Motor pins
int motor1SpeedPin = 10;
int motor1DirectionPin = 6;
int motor1Direction2Pin = 7;
int motor2SpeedPin = 11;
int motor2DirectionPin = 4;
int motor2Direction2Pin = 5;

int relayPin = 8; // Assigns the digital pin to control the relay

// DHT sensor
#define DHTTYPE DHT11
#define DHTPIN 9

DHT dht(DHTPIN, DHTTYPE);

// Timer variables
unsigned long startTime;
unsigned long timerDuration;
unsigned long currentTime;

void setup() {
  Serial.begin(9600); // Set the baud rate to 9600

  // Set pin modes
  pinMode(buttonPin, INPUT);
  pinMode(startButtonPin, INPUT);

  pinMode(motor1SpeedPin, OUTPUT);
  pinMode(motor1DirectionPin, OUTPUT);
  pinMode(motor1Direction2Pin, OUTPUT);
  pinMode(motor2SpeedPin, OUTPUT);
  pinMode(motor2DirectionPin, OUTPUT);
  pinMode(motor2Direction2Pin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(12, OUTPUT);

  digitalWrite(motor1DirectionPin, LOW);
  digitalWrite(motor1Direction2Pin, LOW);
  digitalWrite(motor2DirectionPin, LOW);
  digitalWrite(motor2Direction2Pin, LOW);
  digitalWrite(relayPin, HIGH); // Relay off initially
  digitalWrite(12, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  dht.begin();
}

void loop() {
  switch (b) {
    case 2:
      runQuickDrying();
      break;
    case 3:
      runRegularDrying();
      break;
    case 4:
      runCleaningAndDrying();
      break;
    case 5:
      finishRoutine();
      break;
    case 1:
      handleModeSelection();
      break;
  }
}

void runQuickDrying() {
  if (millis() - startTime <= 200 * 1000) {
    controlMotors(100, 0, 1);
    int temperature = dht.readTemperature();

    updateDisplay("Quick", "Drying", temperature);

    // Correctly toggle the relay based on temperature
    if (temperature < 55) {
      digitalWrite(relayPin, LOW); // Relay on
    } else {
      digitalWrite(relayPin, HIGH); // Relay off
    }
  } else {
    endCurrentMode();
    b = 5; // Go to finish routine
  }
}

void runRegularDrying() {
  if (millis() - startTime <= 300 * 1000) {
    controlMotors(100, 0, 1);
    int temperature = dht.readTemperature();

    updateDisplay("Regular", "Drying", temperature);

    // Correctly toggle the relay based on temperature
    if (temperature < 45) {
      digitalWrite(relayPin, LOW); // Relay on
    } else {
      digitalWrite(relayPin, HIGH); // Relay off
    }
  } else {
    endCurrentMode();
    b = 5; // Go to finish routine
  }
}

void runCleaningAndDrying() {
  if (millis() - startTime <= 300 * 1000) {
    controlMotors(100, 150, 1);
    int temperature = dht.readTemperature();

    updateDisplay("Cleaning", "& Drying", temperature);

    // Correctly toggle the relay based on temperature
    if (temperature < 45) {
      digitalWrite(relayPin, LOW); // Relay on
    } else {
      digitalWrite(relayPin, HIGH); // Relay off
    }
  } else {
    endCurrentMode();
    b = 5; // Go to finish routine
  }
}

void finishRoutine() {
  controlMotors(0, 0, 0);
  digitalWrite(relayPin, HIGH); // Relay off after the routine
  digitalWrite(12, HIGH);
  delay(500);
  digitalWrite(12, LOW);
  display.clearDisplay();

  buttonState = digitalRead(buttonPin);
  display.setCursor(0, 0);
  display.print("Select Mode to Start Again");
  display.display();

  if (buttonState) {
    display.clearDisplay();
    delay(100);
    b = 1; // Go back to mode selection
  }
}

void handleModeSelection() {
  state = digitalRead(startButtonPin);

  if (state) {
    startTime = millis(); // Reset start time
    b = currentMode + 1; // Move to the selected mode
  } else {
    buttonState = digitalRead(buttonPin);

    if (buttonState != lastButtonState) {
      lastButtonState = buttonState;
      if (buttonState == HIGH) {
        currentMode++;
      }
      if (currentMode > 3) {
        currentMode = 1;
      }
    }

    display.clearDisplay();
    display.setCursor(0, 0);

    switch (currentMode) {
      case 1:
        display.println("Quick");
        display.setCursor(0, 20);
        display.println("Drying");
        break;
      case 2:
        display.println("Regular");
        display.setCursor(0, 20);
        display.println("Drying");
        break;
      case 3:
        display.println("Cleaning");
        display.setCursor(0, 20);
        display.println("& Drying");
        break;
    }

    display.display();
  }
}

void controlMotors(int speed1, int speed2, int direction) {
  int pwmSpeed1 = constrain(speed1, 0, 255);
  int pwmSpeed2 = constrain(speed2, 0, 255);

  analogWrite(motor1SpeedPin, pwmSpeed1);
  analogWrite(motor2SpeedPin, pwmSpeed2);

  digitalWrite(motor1DirectionPin, direction);
  digitalWrite(motor2DirectionPin, direction);
}

void updateDisplay(const char* line1, const char* line2, int temperature) {
  display.setCursor(0, 0);
  display.println(line1);
  display.setCursor(0, 20);
  display.println(line2);

  // Clear only the temperature area
  display.fillRect(0, 40, 128, 24, SSD1306_BLACK);

  display.setCursor(0, 40);
  display.print("Temp: ");
  display.print(temperature);
  display.print(" C");
  display.display();
}

void endCurrentMode() {
  digitalWrite(relayPin, HIGH); // Ensure relay is off when the mode ends
  controlMotors(0, 0, 0); // Stop motors
  display.clearDisplay(); // Clear display for next mode
}
