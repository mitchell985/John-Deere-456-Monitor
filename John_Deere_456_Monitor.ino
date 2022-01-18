/*
  John Deere 456 Monitor

  The Freetonics LCD circuit:
 * LCD RS pin to digital pin 8
 * LCD Enable pin to digital pin 9
 * LCD D4 pin to digital pin 4
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 6
 * LCD D7 pin to digital pin 7
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

  By Mitchell Caddy 5/02/16

  with help from the LiquidCrystal Ardunio Library
  and EEROM stuff online
  and reed counter stuff too
 */

// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h> 

//constants
#define VERSION "1.3"
#define COUNTERMEMLOC 0//this well change to counter 1 position id
#define GRANDMEMLOC 996

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to address + 3.
long EEPROMReadlong(long address) {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value) {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

// initialize all the gobal variables
int sensorInput = 2;
unsigned long counter = EEPROMReadlong(COUNTERMEMLOC);//could move all the global variable but this to the top...
long lastDebounce = 0;
long debounceDelay = 500;
String inString = "";
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//sensor and display setup
void setup() {
  //Sensor Setup
  pinMode(sensorInput, INPUT);
  digitalWrite(sensorInput, HIGH);
  attachInterrupt(0, trigger, FALLING);
  
  // Setup LCD
  lcd.begin(16, 2);
  //Show Spashscreen text
  spashscreen(1,2000);
  mainScreen();
}

//looping to check if the reset button was pushed
void loop() {
 checkButton();
}

//displays a spashscreen when monitor is turned on
//with an option to display the bottom Text or not
//and the amount of time the spashscreen should show for
void spashscreen(int showBottom, int enableDelay) {
  clearCenterTitleNextLine("John Deere 456");
  if (showBottom) {
    lcd.print("Monitor v");
    lcd.print(VERSION);
  }
  if (enableDelay) delay(enableDelay);
}

//creates the main screen of the monitor
void mainScreen() {
  spashscreen(0,0);
  //creating the counter
  lcd.print(counter);
  lcd.setCursor(11, 1);
  lcd.print("Bales");
}

//this is a trigger function triggered by the attachInterrupt to increase the count
void trigger() {
  if( (millis() - lastDebounce) > debounceDelay){
    //adds to the count
    counter++;
    //writes the the eeprom in case of power loss
    EEPROMWritelong(COUNTERMEMLOC, counter);
    //writes the new counter number on the screen
    lcd.setCursor(0,1);
    lcd.print(counter);
    lastDebounce = millis();
  }
}

void grandTotal() {
  clearCenterTitleNextLine("Grand Total");
  lcd.print(EEPROMReadlong(GRANDMEMLOC));
  lcd.setCursor(11,1);
  lcd.print("Bales");
}

void clearCenterTitleNextLine(String title) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0,1);
}

void enableSerial() {
  clearCenterTitleNextLine("Laptop Control");
  lcd.print("Enabled");
  
  // initialize serial:
  Serial.begin(9600);
  // reserve 200 bytes for the inputString:
  inString.reserve(200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // the intro information
  Serial.println("Serial control enabled");
  Serial.print("Current Grand Total is: ");
  Serial.println(EEPROMReadlong(GRANDMEMLOC));
  Serial.println("Enter new Total:");
}

void serialEvent() {
  while (Serial.available()) {
    int inChar = Serial.read();
    if (isDigit(inChar)) {
      // convert the incoming byte to a char
      // and add it to the string:
      inString += (char)inChar;
    }
    
    // if you get a newline, print the string,
    // then the string's value:
    if (inChar == '\n') {
      Serial.print("Old Grand Total:");
      Serial.println(EEPROMReadlong(GRANDMEMLOC));
      Serial.print("New Grand Total:");
      Serial.println(inString.toInt());
      EEPROMWritelong(GRANDMEMLOC,inString.toInt());
      // clear the string for new input:
      inString = "";
    }
  }
}

//checks if the reset button also know as the select button on the pcb is pressed
void checkButton() {
  //puts analog pin 0 reading into a variable
  int analogReading = analogRead(0);
  //checks and cleans the reading
  delay(5);
  int k = (analogRead(0) - analogReading);
  if (5 < abs(k)) return;
  //checking what button was pressed
  if (analogReading > 1000) return;//None was pressed
  if (analogReading < 50)   return;//Right Button
  if (analogReading < 195) { //Up Button
    grandTotal();
    return;
  }
  if (analogReading < 380) {//Down Button
    mainScreen();
    return;
  }
  if (analogReading < 555) {//Left Button
    enableSerial();
    return;
  }
  if (analogReading < 790) {//Reset Button or Select
    counter = 0;
    mainScreen();
    return;
  }
  return;
}
