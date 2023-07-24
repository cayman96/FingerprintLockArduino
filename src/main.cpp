//jedyne, co tu zawieram do bibliotekę z moimi funkcjami - biblioteki dot. skanera, serwa itp. są w tym headerze.
#include <doorLockFunctions.h>
//UWAGA!!! deklaracja obiektów takich jak Bouncer, SoftwareSetial, skanera, ekranu lcd i serwa dodałem TUTAJ!!! 
//dodanie w doorLockFunctions.h powoduje, że jednak z jakiegoś powodu się one nie deklarują, wskutek tego - cały układ nie działa prawidłowo!
Bounce openLockBtn = Bounce();
hd44780_I2Cexp lcd;
PWMServo doorLock;
SoftwareSerial btModule(btRx, btTx);
SoftwareSerial fingerprintScanner(fRx, fTx);
Adafruit_Fingerprint fingerScan = Adafruit_Fingerprint(&fingerprintScanner);
//nie wiem, czy poniższych globalnych nie mogę do headera przenieść, sprawdzę....
boolean btState = false; //obecny status połączenia bt
boolean scannerConn = false; //status skanera
boolean lastBtState = false; //poprzedni status bt - po to, by sprawdzić różnice w statusie bt i czy układ ma się przełączyć na/z tryb/u serwisowy/ego czy nie
char btComm; //co otrzymaliśmy od bt układ trzyma w tej zmiennej
//te inty są nam po to, by odmierzyć czas od ostatniego sprawdzenia, skanu, wciśnięcia przycisku i czas minięty od włączenia układu.
unsigned int btnPressLastTime = 0;
unsigned int btCheckLastTime = 0;
unsigned int fingerScanLastTime = 0;
unsigned int currentTime = 0;

void setup() {
  //inicjalizacja lcd musi znaleźć się tutaj, ponieważ w zewnętrzynych funkcjach inicjalizuje sie on niepoprawnie.
  lcd.begin(16,2);
  //deklaracja pinów, sprawdzenie czy drzwi są otwarte/zamknięte i status skanera.
  initialSetup();
  fpScannerCheck();
}
void loop() {
  //ach, millis... tutaj to działa perfekcyjnie - na bieżąco sprawdzamy, czy przycisk został wciśnięty lub nastąpiło połączenie przez bt
  //jak i sprawdzamy, czy user położył palec na skanerze.
  //generalnie, chce aby w pliku .ino znajdowało sie jak najmniej kodu. spróbuje jeszcze bardziej to uprościć i przenieść do .cpp i deklaracje w .h.
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
