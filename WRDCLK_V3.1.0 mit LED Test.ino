// Aktuelle Änderungen:
// - setSyncProvider eingebaut
// - Farben in variablen verpackt
// - EIN statt EINS bei voller Stunde
// - Beleuchtung mittels Sensortaste faden
// - RTC Zeit checken und vor ggf. DCF Suche übernehmen
// - SIEBENZ Bug gefixed
// - Test: Zur 0./5. Minute DCF Abgleich
// - DEBUG mittels define 
// - Fehlersuche Abstürze: Serial abgeschaltet
// - Helligkeitswert regelmäßig anhand der Tageszeit setzen
// - FASTLED.show nur im Minutentakt - evtl. LED Ausfallrate verringern
// - Helligkeitsstufen abhängig von Tageszeit
// - Sensortaste einmal pro Sekunde abfragen
// - Servicefunktion: LEDtest bei Verbindung D4 mit GND

//##### Libraries importieren
#include "FastLED.h"
#include "DCF77.h"
#include "TimeLib.h"
#include "CapacitiveSensor.h"
#include "DS3232RTC.h"
#include "Wire.h"
#include "avr/wdt.h"

//##### Pins am Arduino konfigurieren
#define NUM_LEDS 114        // Anzahl der LEDs
#define DATA_PIN 6          // Pin 6 für LED DATA konfigurieren
#define DCF_PIN 2           // Pin 2 für DCF 77 DATA konfigurieren
#define DCF_INTERRUPT 0     // Interrupt für Pin 2 setzen
#define TEST_PIN 4          // Pin 4 um LED Test Modus zu aktivieren (Mit GND verbinden)

//##### DEBUG an oder aus > auskommentieren oder eben nicht
#define DEBUG             
#ifdef DEBUG
  #define DEBUG_START(x) Serial.begin(x)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) ;Serial.println(x)
#else
  #define DEBUG_START(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

//##### Variablen und setzen und konfigurieren
time_t prevDisplay = 0;                                     
time_t time;
DCF77 DCF = DCF77(DCF_PIN,DCF_INTERRUPT);
CapacitiveSensor cs_4_8 = CapacitiveSensor(4,8);     // Pins 4 & 8 für Sensortaste

CRGB leds[NUM_LEDS];
CRGB color = CRGB::White;
CRGB dotcolor = CRGB::White;
CRGB dcfcolor = CRGB::Red;
CRGB esistcolor = CRGB::White;

int brightness_value = 150;
int sensor_trigger_on = 2800;
int sensor_trigger_off = 2800;
int brightness_value_max = brightness_value;
int brightness_value_temp = brightness_value;
int stunden, minuten;
int DCF_updated;
int now_minute;
int now_sekunde;
bool toggle = true;
int buttonState = 0;

void setup() {
  DEBUG_START(9600);
  pinMode(TEST_PIN, INPUT_PULLUP);
  DCF.Start();
  DEBUG_PRINTLN("WRDCLK by Fabian Doerr und Jan Evers");
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  for (int i=0; i<114; i++) {
        leds[i] = CRGB::Black;
  }
  FastLED.show();
//##### LED TEST aktivieren, wenn PIN 4 mit GND verbunden ist
  buttonState = digitalRead(TEST_PIN);
  if (buttonState == LOW) {
     for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
        leds[whiteLed] = CRGB::White;
        FastLED.show();
        delay(1000);
        leds[whiteLed] = CRGB::Black;
     }
  }
//##### Lightshow beim Einschalten
  for (brightness_value_temp = 0; brightness_value_temp < brightness_value_max; brightness_value_temp ++) {
    delay(10);
    LEDS.setBrightness(brightness_value_temp);
    leds[59] = CRGB::Blue;
    leds[72] = CRGB::Blue;
    leds[81] = CRGB::Yellow;
    leds[82] = CRGB::Yellow;
    leds[38] = CRGB::Red;
    leds[39] = CRGB::Red;
    leds[40] = CRGB::Red;
    leds[48] = CRGB::White;
    leds[61] = CRGB::White;
    leds[70] = CRGB::White;
    FastLED.show();
  }
  delay(10000);
  for (brightness_value_temp = brightness_value_max; brightness_value_temp > 0; brightness_value_temp --) {
      delay(10);
      LEDS.setBrightness(brightness_value_temp);
      FastLED.show();
  }
  for (int i=0; i<114; i++) {
        leds[i] = CRGB::Black;
  }
  LEDS.setBrightness(brightness_value);
  FastLED.show();
//##### Zeit suchen   
  setSyncInterval(60);
  setSyncProvider(RTC.get);
  if (timeStatus()== timeNotSet) {
    setSyncProvider(getDCFTime);
    DEBUG_PRINTLN("Warte auf DCF77... ");
    DEBUG_PRINTLN("Es kann geraume Zeit dauern, bis die Zeit aktualisiert werden kann.");
    while(timeStatus()== timeNotSet) {
      DEBUG_PRINT(".");
      delay(2000);
      leds[60] = dcfcolor; 
      leds[71] = dcfcolor; 
      leds[82] = dcfcolor;
      FastLED.show();
    }
    RTC.set(now());
//  digitalWrite(13, HIGH);
  }
  setSyncProvider(RTC.get);
  wdt_enable(WDTO_8S);
  DEBUG_PRINTLN("Watchdog auf 8 Sekunden gesetzt"); 
}

void loop() {
  
  if (now() != prevDisplay) {
    prevDisplay = now();
    digitalClockDisplay();
  }
  //##### Minutentakt
  if (minute() != now_minute) {
    now_minute = minute();
    Times();
    DEBUG_PRINTLN("Signal auf Bus ausgegeben");
  }
  //##### Sekundentakt
  if (second() != now_sekunde) {
    now_sekunde = second();
    DEBUG_PRINTLN("Sensortaste abfragen");
    //SensorTaste();
  }
  //##### Tageszeit auswerten  
  TageszeitAuswerten()
  //##### Watchdog reset  
  DEBUG_PRINTLN("Watchdogtimer reset"); 
  wdt_reset();
}

//void TempAnzeigen() {
//  int t = RTC.temperature();
//  float celsius = t / 4.0;
//  DEBUG_PRINTLN(celsius);
//}



void TageszeitAuswerten() {
  if ((hour() >= 7)&&(hour() <= 18)) {    // Zwischen 07:00 und 18:59
    brightness_value_max = 120;
    brightness_value = 120;  
  }
  if ((hour() >= 19)&&(hour() <= 22)) {   // Zwischen 19:00 und 22:59
    brightness_value_max = 70;
    brightness_value = 70;
  }
  if (hour() == 23) {                     // Zwischen 23:00 und 23:59
      brightness_value_max = 30;
      brightness_value = 30;  
  }
  if ((hour() >= 0)&&(hour() <= 6)) {    // Zwischen 00:00 und 06:59
    if ((minute() == 0)||(minute() == 5)||(minute() == 10)||(minute() == 15)||(minute() == 20)||(minute() == 25)||(minute() == 30)||(minute() == 35)||(minute() == 40)||(minute() == 45)||(minute() == 50)||(minute() == 55)) {
      brightness_value_max = 0;
      brightness_value = 0;  
    }
  }
}

void LichtAn() {
  if (toggle == false) {
    toggle = true;
    for (brightness_value_temp = 0; brightness_value_temp < brightness_value_max; brightness_value_temp ++) {
      LEDS.setBrightness(brightness_value_temp);
      FastLED.show();
    }
    DEBUG_PRINTLN("Funktion: Licht an...");
  }
}

void LichtAus() {
  if (toggle == true) {
    toggle = false;
    for (brightness_value_temp = brightness_value_max; brightness_value_temp > 0; brightness_value_temp --) {
      LEDS.setBrightness(brightness_value_temp);
      FastLED.show();
    }
    DEBUG_PRINTLN("Funktion: Licht aus...");
  }
}

//##### Funktion der "Sensortaste" (Alurahmen)
void SensorTaste() {
  long valueSensor1 =  cs_4_8.capacitiveSensor(30);
  if ((valueSensor1 >= sensor_trigger_on)&&(toggle == false)) {
    toggle = true;
    for (brightness_value_temp = 0; brightness_value_temp < brightness_value_max; brightness_value_temp ++) {
      LEDS.setBrightness(brightness_value_temp);
      FastLED.show();
    }
    DEBUG_PRINTLN("Sensor: Licht an...");
  }
  else if ((valueSensor1 >= sensor_trigger_off)&&(toggle == true)) {
    toggle = false;
    for (brightness_value_temp = brightness_value_max; brightness_value_temp > 0; brightness_value_temp --) {
      LEDS.setBrightness(brightness_value_temp);
      FastLED.show();
    }
    DEBUG_PRINTLN("Sensor: Licht aus...");
  }
  if (now() != prevDisplay) {
    DEBUG_PRINT(valueSensor1);
    DEBUG_PRINT("/");
    DEBUG_PRINTLN(toggle);
  }
}

unsigned long getDCFTime() { 
  time_t DCFtime = DCF.getTime();
  if (DCFtime!=0) {
    DEBUG_PRINTLN("Update von DCF"); 
    RTC.set(now());   
    DEBUG_PRINTLN("RTC mit DCF Zeit aktualisiert");
  }
  return DCFtime;
}

void Times() {
  LED_reset();
  minuten_leds();
  zwischenzeiten();
  uhrzeiten();
  LEDS.setBrightness(brightness_value);
  FastLED.show();
}

void LED_reset() {
  for (int i=0; i<114; i++) {
        leds[i] = CRGB::Black;
  }
}

//##### Die 4 Minuten LEDS setzen
void minuten_leds() {
  if ((minute() == 0)||(minute() == 5)||(minute() == 10)||(minute() == 15)||(minute() == 20)||(minute() == 25)||(minute() == 30)||(minute() == 35)||(minute() == 40)||(minute() == 45)||(minute() == 50)||(minute() == 55)) {
    for (int i=110; i<114; i++) {
        leds[i] = CRGB::Black;
    }
    setup_5MINUTEN();
    if ((second() == 0) && (DCF_updated == 0)) {
      time_t DCFtime = DCF.getTime();
      if (DCFtime!=0) {
        DEBUG_PRINTLN("Update von DCF"); 
        setTime(DCFtime);
        RTC.set(now());   
        DEBUG_PRINTLN("RTC mit DCF Zeit aktualisiert");
      }
      else {
        DEBUG_PRINTLN("Kein DCF...");
      }
      DCF_updated = 1;
    }
  }
  if ((minute() == 1)||(minute() == 6)||(minute() == 11)||(minute() == 16)||(minute() == 21)||(minute() == 26)||(minute() == 31)||(minute() == 36)||(minute() == 41)||(minute() == 46)||(minute() == 51)||(minute() == 56)) {
     for (int i=110; i<114; i++) {
        leds[i] = CRGB::Black;
    }
    setup_1MINUTEN();
  }
  if ((minute() == 2)||(minute() == 7)||(minute() == 12)||(minute() == 17)||(minute() == 22)||(minute() == 27)||(minute() == 32)||(minute() == 37)||(minute() == 42)||(minute() == 47)||(minute() == 52)||(minute() == 57)) {
     for (int i=110; i<114; i++) {
        leds[i] = CRGB::Black;
    }
    setup_2MINUTEN();
  }
  if ((minute() == 3)||(minute() == 8)||(minute() == 13)||(minute() == 18)||(minute() == 23)||(minute() == 28)||(minute() == 33)||(minute() == 38)||(minute() == 43)||(minute() == 48)||(minute() == 53)||(minute() == 58)) {
     for (int i=110; i<114; i++) {
        leds[i] = CRGB::Black;
    }
    setup_3MINUTEN();
  }
  if ((minute() == 4)||(minute() == 9)||(minute() == 14)||(minute() == 19)||(minute() == 24)||(minute() == 29)||(minute() == 34)||(minute() == 39)||(minute() == 44)||(minute() == 49)||(minute() == 54)||(minute() == 59)) {
     for (int i=110; i<114; i++) {
        leds[i] = CRGB::Black;
    }
    setup_4MINUTEN();
    DCF_updated = 0;
  }
}

//##### Uhrzeiten setzen 
void uhrzeiten() {
  setup_ES_IST();
  if (minute() >= 0 && minute() < 5) {
    setup_UHR();
  }
  if ((hour() == 10) || (hour() == 22)) {
    if (minute() >= 25) {
      setup_ELF_Stunde();
    }
    else {
      setup_ZEHN_Stunde();
    }
  }
  if ((hour() == 11) || (hour() == 23)) {
    if (minute() >= 25) {
      setup_ZWOELF_Stunde();
    }
    else {
      setup_ELF_Stunde();
    }    
  }
  if ((hour() == 12)||(hour() == 0)) {
    if (minute() >= 25) {
      setup_EINS_Stunde();
    }
    else {
      setup_ZWOELF_Stunde();
    }
  }
  if ((hour() == 13)||(hour() == 1)) {
    if (minute() <= 4) {
      setup_EIN_Stunde();
    }
    else if ((minute() >= 5)&&(minute() <= 24)) {
      setup_EINS_Stunde();
    }
    else {
      setup_ZWEI_Stunde();
    }
  }
  if ((hour() == 14)||(hour() == 2)) {
    if (minute() >= 25) {
      setup_DREI_Stunde();
    }
    else {
      setup_ZWEI_Stunde();
    }
  }
  if ((hour() == 15)||(hour() == 3)) {
    if (minute() >= 25) {
      setup_VIER_Stunde();
    }
    else {
      setup_DREI_Stunde();
    }
  }
  if ((hour() == 16)||(hour() == 4)) {
    if (minute() >= 25) {
      setup_FUENF_Stunde();
    }
    else {
      setup_VIER_Stunde();
    }
  }
  if ((hour() == 17)||(hour() == 5)) {
    if (minute() >= 25) {
      setup_SECHS_Stunde();
    }
    else {
      setup_FUENF_Stunde();
    }
  }
  if ((hour() == 18)||( hour() == 6)) {
    if (minute() >= 25) {
      setup_SIEBEN_Stunde();
    }
    else {
      setup_SECHS_Stunde();
    }
  }
  if ((hour() == 19)||(hour() == 7)) {
    if (minute() >= 25) {
      setup_ACHT_Stunde();
    }
    else {
      setup_SIEBEN_Stunde();
    }
  }
  if ((hour() == 20)||(hour() == 8)) {
    if (minute() >= 25) {
      setup_NEUN_Stunde();
    }
    else {
      setup_ACHT_Stunde();
    }
  }
  if ((hour() == 21)||(hour() == 9)) {
    if (minute() >= 25) {
      setup_ZEHN_Stunde();
    }
    else {
      setup_NEUN_Stunde();
    }
  }
}

//##### Zwischenzeiten setzen
void zwischenzeiten() {
  if (minute() >= 5 && minute() < 10) {
    setup_FUENF_Minuten();
    setup_NACH();
  }
  if (minute() >= 10 && minute() < 15) {
    setup_ZEHN_Minuten();
    setup_NACH();
  }
  if (minute() >= 15 && minute() < 20) {
    setup_VIERTEL();
    setup_NACH();
  }
  if (minute() >= 20 && minute() < 25) {
    setup_ZWANZIG_Minuten();
    setup_NACH();
  }
  if (minute() >= 25 && minute() < 30) {
    setup_FUENF_Minuten();
    setup_VOR();
    setup_HALB();
    hour() + 1;
  }
  if (minute() >= 30 && minute() < 35) {
    setup_HALB();
    hour() + 1;
  }
  if (minute() >= 35 && minute() < 40) {
    setup_FUENF_Minuten();
    setup_NACH();
    setup_HALB();
    hour() + 1;
  }
  if (minute() >= 40 && minute() < 45) {
    setup_ZWANZIG_Minuten();
    setup_VOR();
    hour() + 1;
  }
  if (minute() >= 45 && minute() < 50) {
    setup_VIERTEL();
    setup_VOR();
    hour() + 1;
  }
  if (minute() >= 50 && minute() < 55) {
    setup_ZEHN_Minuten();
    setup_VOR();
    hour() + 1;
  }
  if (minute() >= 55 && minute() < 60) {
    setup_FUENF_Minuten();
    setup_VOR();
    hour() + 1;
  }
}

//##### Uhrzeit seriell ausgeben
void digitalClockDisplay(){
  // digital clock display of the time
  DEBUG_PRINT(hour());
  printDigits(minute());
  printDigits(second());
  DEBUG_PRINT(" ");
//  Serial.print(day());
//  Serial.print(" ");
//  Serial.print(month());
//  Serial.print(" ");
//  Serial.print(year()); 
  DEBUG_PRINT("...Stunde: ");
  DEBUG_PRINT(hour());
  DEBUG_PRINTLN(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  DEBUG_PRINT(":");
  if(digits < 10)
    DEBUG_PRINT('0');
  DEBUG_PRINT(digits);
}

//##### Beleuchtung für die Wörter setzen
void setup_ES_IST() {
  leds[0] = esistcolor; 
  leds[1] = esistcolor; 
  leds[3] = esistcolor; 
  leds[4] = esistcolor;  
  leds[5] = esistcolor;   
}
void setup_FUENF_Minuten()  {
  leds[7] = color; 
  leds[8] = color;
  leds[9] = color; 
  leds[10] = color; 
}
void setup_ZEHN_Minuten()  {
  leds[18] = color; 
  leds[19] = color;
  leds[20] = color; 
  leds[21] = color; 
}
void setup_ZWANZIG_Minuten()  {
  leds[11] = color; 
  leds[12] = color;
  leds[13] = color; 
  leds[14] = color; 
  leds[15] = color;
  leds[16] = color; 
  leds[17] = color; 
}
void setup_DREI_Minute()  {
  leds[22] = color; 
  leds[23] = color;
  leds[24] = color; 
  leds[25] = color; 
}
void setup_VIERTEL() {
  leds[26] = color; 
  leds[27] = color;
  leds[28] = color; 
  leds[29] = color;  
  leds[30] = color; 
  leds[31] = color;  
  leds[32] = color; 
}
void setup_VOR() {
  leds[41] = color;
  leds[42] = color; 
  leds[43] = color;  
}
void setup_NACH() {
  leds[33] = color; 
  leds[34] = color;
  leds[35] = color; 
  leds[36] = color;  
}
void setup_HALB() {
  leds[44] = color; 
  leds[45] = color;
  leds[46] = color; 
  leds[47] = color;  
}
void setup_ELF_Stunde() {
  leds[49] = color;
  leds[50] = color; 
  leds[51] = color;  
}
void setup_FUENF_Stunde() {
  leds[51] = color;
  leds[52] = color; 
  leds[53] = color;  
  leds[54] = color;  
}
void setup_EIN_Stunde() {
  leds[63] = color;
  leds[64] = color; 
  leds[65] = color;  
}
void setup_EINS_Stunde() {
  leds[62] = color;
  leds[63] = color; 
  leds[64] = color;  
  leds[65] = color;  
}
void setup_ZWEI_Stunde() {
  leds[55] = color;
  leds[56] = color; 
  leds[57] = color;  
  leds[58] = color;  
}
void setup_DREI_Stunde() {
  leds[66] = color;
  leds[67] = color; 
  leds[68] = color;  
  leds[69] = color;  
}
void setup_VIER_Stunde() {
  leds[73] = color;
  leds[74] = color; 
  leds[75] = color;  
  leds[76] = color;  
}
void setup_SECHS_Stunde() {
  leds[83] = color;
  leds[84] = color; 
  leds[85] = color;  
  leds[86] = color; 
  leds[87] = color;  
}
void setup_ACHT_Stunde() {
  leds[77] = color;
  leds[78] = color; 
  leds[79] = color;  
  leds[80] = color;  
}
void setup_SIEBEN_Stunde() {
  leds[88] = color; 
  leds[89] = color;
  leds[90] = color; 
  leds[91] = color;  
  leds[92] = color; 
  leds[93] = color;  
}
void setup_ZWOELF_Stunde() {
  leds[94] = color;
  leds[95] = color;
  leds[96] = color; 
  leds[97] = color;  
  leds[98] = color;  
}
void setup_ZEHN_Stunde() {
  leds[106] = color;
  leds[107] = color; 
  leds[108] = color;  
  leds[109] = color;  
}
void setup_NEUN_Stunde() {
  leds[103] = color;
  leds[104] = color; 
  leds[105] = color;  
  leds[106] = color;  
}
void setup_UHR() {
  leds[99] = color;
  leds[100] = color; 
  leds[101] = color;  
}
void setup_1MINUTEN() {
  leds[110] = dotcolor;
}
void setup_2MINUTEN() {
  leds[110] = dotcolor; 
  leds[111] = dotcolor;
}
void setup_3MINUTEN() {
  leds[110] = dotcolor; 
  leds[111] = dotcolor;
  leds[112] = dotcolor;
}
void setup_4MINUTEN() {
  leds[110] = dotcolor; 
  leds[111] = dotcolor;
  leds[112] = dotcolor; 
  leds[113] = dotcolor;  
}
void setup_5MINUTEN() {
}
