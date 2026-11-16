#include<LiquidCrystal.h>

LiquidCrystal lcd(6, 7, 5, 4, 3, 2);
/*
  VSS = GND (GND for logic)
  VDD = VCC (VCC for logic)
  V0 = Pot
  RS = Pin A5
  RW = GND (0 = Write)
  E = Pin A4
  D4 = Pin A3
  D5 = Pin A2
  D6 = Pin A1
  D7 = Pin A0
  A = 220Ohm Resistor (VCC for LED)
  K = GND (GND for LED)
*/

void setup(){
  lcd.begin(16,2);
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Hello");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("This is a Test");
  delay(2000);
  lcd.clear();
}
