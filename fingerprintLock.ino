#include "doorLockFunctions.h"
Bounce openLockBtn = Bounce();
hd44780_I2Cexp lcd;
PWMServo doorLock;
SoftwareSerial btModule(btRx, btTx);
SoftwareSerial fingerprintScanner(fRx, fTx);
Adafruit_Fingerprint fingerScan = Adafruit_Fingerprint(&fingerprintScanner);
boolean btState = false;
boolean scannerConn = false;
boolean lastBtState = false;
char btComm;
unsigned int btnPressLastTime = 0;
unsigned int btCheckLastTime = 0;
unsigned int fingerScanLastTime = 0;
unsigned int currentTime = 0;

void setup() {
  //inicjalizacja lcd musi znaleźć się tutaj, ponieważ w zewnętrzynych funkcjach inicjalizuje sie on niepoprawnie.
  lcd.begin(16,2);
  initialSetup();
  fpScannerCheck();
}
void loop() {
  currentTime = millis();
  if (currentTime - btCheckLastTime > btCheck) {
    btCheckLastTime = currentTime;
    btState = digitalRead(btSt);
    checkPrevBtState();
  }
  if (btState == true) {
    maintenanceModeMenu();
  } else {
    if (doorLock.read() != 180) {
      lockOpenBehavior();
    }
    if (currentTime - btnPressLastTime > btnPress) {
      btnPressLastTime = currentTime;
      openLockBtn.update();
      if (openLockBtn.fell()) {
        manualOpen();
      }
    }
    if (currentTime - fingerScanLastTime > fpScanCheck) {
      fingerScanLastTime = currentTime;
      unlockWithFingerprint();
    }
  }
}
