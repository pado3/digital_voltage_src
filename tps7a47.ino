/*
tps7a47.ino
digitally controlled voltage source using TI TPS7A47 by @pado3@mstdn.jp

r1.1 2023/04/28 correct the code format
r1.0 2023/04/14 initial release, same as r0.3
r0.3 2023/04/12 formatting Serial.print()
r0.2 2023/04/11 start debug, console OK
r0.1 2023/04/09 start coding

encoder library: http://www.pjrc.com/teensy/td_libs_Encoder.html
*/
#include <Wire.h>
#include <Encoder.h>

// pin assign
// Encoder best: both pins have interrupt capability
Encoder myEnc(3, 2);  // (B, A) = (D3, D2) = (INT1, INT0)
// o6r4a/o6r4/o3r2/o1r6/o0r8/o0r4/o0r2/o0r1 of TPS7A47
//   D5 / D6 / D7 / D8 /D12 /D11 /D10 / D9
#define o6r4a 5      // 6.4V-2
#define o6r4 6       // 6.4V-1
#define o3r2 7       // 3.2V
#define o1r6 8       // 1.6V
#define o0r8 12      // 0.8V
#define o0r4 11      // 0.4V
#define o0r2 10      // 0.2V
#define o0r1 9       // 0.1V
#define LED 13       // LED @ D13 (10k)
#define Vin 16       // ADC for Vout/2, Arduino D16=A2
#define sdaPin 18    // Arduino D18=A4
#define sclPin 19    // Arduino D19=A5
#define LCDadr 0x3e  // LCD I2C address, fixed

// other definition
#define OFFSET 1.4     // output offset of TPS7A47
#define CONTRAST 0x1A  // fixed, 3.3V
#define maxV 50        // maximum output voltage, in 0.1V
#define minV 14        // minimum output voltage, in 0.1V

long preV = 14;  // previous position of rotary encoder, should global

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("START " __FILE__ " from " __DATE__ " " __TIME__));
  // LED setup
  pinMode(LED, OUTPUT);
  // initialize rotary encoder
  myEnc.write(-999);
  // LCD setup
  Wire.begin();
  // delay(500);
  lcd_cmd(0b00111000);                            // function set
  lcd_cmd(0b00111001);                            // function set
  lcd_cmd(0b00000100);                            // EntryModeSet
  lcd_cmd(0b00010100);                            // interval osc
  lcd_cmd(0b01110000 | (CONTRAST & 0xF));         // contrast Low
  lcd_cmd(0b01011100 | ((CONTRAST >> 4) & 0x3));  // contast High/icon/power
  lcd_cmd(0b01101100);                            // follower control
  delay(200);
  lcd_cmd(0b00111000);                            // function set
  lcd_cmd(0b00001100);                            // Display On
  lcd_cmd(0b00000001);                            // Clear Display
  delay(2);
  // スプラッシュ表示
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_printStr("Digital ");
  lcd_setCursor(0, 1);
  lcd_printStr("Volt.src");
  delay(2000);
}

void loop() {
  char tmp[16];
  long newV = myEnc.read();
  if (newV != preV || newV == -999) {  // -999:first time
    newV = preV + (newV - preV) / 3;   // 1click 2~4step
    // set limitter
    if (newV > maxV) newV = maxV;
    if (newV < minV) newV = minV;
    myEnc.write(newV);
    Serial.print("newV: ");
    Serial.print(newV);
    Serial.print(", ");
    preV = newV;
    // voltage output
    double vset = set_vout(newV);
    // display set voltage
    lcd_clear();
    lcd_setCursor(0, 0);
    lcd_printStr("SET ");
    dtostrf(vset, 3, 1, tmp);  // 1.4-5.0
    lcd_printStr(tmp);
    lcd_printStr("V");
  }
  // read output voltage
  double vout = read_vout();
  // display measured voltage
  lcd_setCursor(0, 1);
  lcd_printStr("OUT ");
  dtostrf(vout, 3, 1, tmp);
  lcd_printStr(tmp);
  lcd_printStr("V");
  // prevent chattering
  delay(100);
}


// voltage output
double set_vout(double newV) {
  byte tps = 0;
  if (minV <= newV && newV <= maxV) {
    tps = (byte)(newV - minV);  // 14~50 -> 0-36
  } else {
    tps = 0;
  }
  // debug
  char tmp[16];
  dtostrf(tps, 2, 0, tmp);
  Serial.print("TPS set to 1.4+(");
  Serial.print(tmp);
  Serial.print("/10)V = ");
  double vset = 1.4 + 0.1 * (double)tps;
  dtostrf(vset, 4, 1, tmp);
  Serial.print(vset);
  Serial.print("V, ");
  // set 6.4V-2
  Serial.print("6.4-1.6 0.8-0.1 : ");
  if (tps & B10000000) {
    pinMode(o6r4a, OUTPUT);
    digitalWrite(o6r4a, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o6r4a, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 6.4V-1
  if (tps & B01000000) {
    pinMode(o6r4, OUTPUT);
    digitalWrite(o6r4, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o6r4, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 3.2V
  if (tps & B00100000) {
    pinMode(o3r2, OUTPUT);
    digitalWrite(o3r2, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o3r2, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 1.6V
  if (tps & B00010000) {
    pinMode(o1r6, OUTPUT);
    digitalWrite(o1r6, LOW);  // set tps port LOW
    Serial.print("1 ");
  } else {
    pinMode(o1r6, INPUT);     // set tps port Hi-Z
    Serial.print("0 ");
  }
  // set 0.8V
  if (tps & B00001000) {
    pinMode(o0r8, OUTPUT);
    digitalWrite(o0r8, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o0r8, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 0.4V
  if (tps & B00000100) {
    pinMode(o0r4, OUTPUT);
    digitalWrite(o0r4, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o0r4, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 0.2V
  if (tps & B00000010) {
    pinMode(o0r2, OUTPUT);
    digitalWrite(o0r2, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o0r2, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  // set 0.1V
  if (tps & B00000001) {
    pinMode(o0r1, OUTPUT);
    digitalWrite(o0r1, LOW);  // set tps port LOW
    Serial.print("1");
  } else {
    pinMode(o0r1, INPUT);     // set tps port Hi-Z
    Serial.print("0");
  }
  dtostrf(tps, 2, 0, tmp);
  Serial.print(" (");
  Serial.print(tmp);
  Serial.println(")");
  return vset;
}

// measure output voltage
double read_vout() {
  int value = analogRead(Vin);
  double vout = (double)value * 10.0 / 1023.0;  // Ain is divide by 2
  // debug
  /*
  Serial.print("readout voltage: ");
  Serial.print(vout);
  Serial.println("V");
  */
  return vout;
}

// LCD related functions from here
void lcd_cmd(byte x) {
  Wire.beginTransmission(LCDadr);
  Wire.write(0b00000000);  // CO = 0,RS = 0
  Wire.write(x);
  Wire.endTransmission();
}

void lcd_clear() {
  lcd_cmd(0b00000001);
}

void lcd_contdata(byte x) {
  Wire.write(0b11000000);  // CO = 1, RS = 1
  Wire.write(x);
}

void lcd_lastdata(byte x) {
  Wire.write(0b01000000);  // CO = 0, RS = 1
  Wire.write(x);
}

// display strings
void lcd_printStr(const char *s) {
  Wire.beginTransmission(LCDadr);
  while (*s) {
    if (*(s + 1)) {
      lcd_contdata(*s);
    } else {
      lcd_lastdata(*s);
    }
    s++;
  }
  Wire.endTransmission();
}

// display int number
void lcd_printInt(int num) {
  char int2str[10];
  sprintf(int2str, "%d", num);
  lcd_printStr(int2str);
}

// set cursor position
void lcd_setCursor(byte x, byte y) {
  lcd_cmd(0x80 | (y * 0x40 + x));
}
