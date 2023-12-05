#include <doorLockFunctions.h>

// pin declatation
const int lR = 12;
const int lG = 13;
const int magnetPin = 11;
const int servoPin = 10;
const int btRx = 9;
const int btTx = 8;
const int btSt = 7;
const int openPin = 6;
const int fTx = 5; // white
const int fRx = 4; // green
const int rst = 3;
const unsigned int btnPress = 25;
const unsigned int btCheck = 50;
const unsigned int emergencyCheck = 25;
const unsigned short doorDidntOpenTime = 10000;
const unsigned int fpScanCheck = 100;
Bounce openLockBtn = Bounce();
hd44780_I2Cexp lcd;
PWMServo doorLock;
SoftwareSerial btModule(btRx, btTx);
SoftwareSerial fingerprintScanner(fRx, fTx);
Adafruit_Fingerprint fingerScan = Adafruit_Fingerprint(&fingerprintScanner);
boolean btState = false; // current bluetooth connection state 
boolean scannerConn = false; // scanner state
boolean lastBtState = false; // last bluetooth state - kept for comparison
char btComm; // received content from bluetooth
// variables used for keeping time for specific actions times
unsigned int btnPressLastTime = 0;
unsigned int btCheckLastTime = 0;
unsigned int fingerScanLastTime = 0;
unsigned int currentTime = 0;

// preparing pins and modules for their role
void initialSetup() {
  lcd.begin(16,2);
  pinMode(lR, OUTPUT);
  pinMode(lG, OUTPUT);
  pinMode(btSt, INPUT);
  pinMode(rst, OUTPUT);
  // I could use an external resistor for this pin, but it's not necessary
  // and I think the build-in resistor will suffice
  pinMode(magnetPin, INPUT_PULLUP);
  digitalWrite(lG, HIGH);
  digitalWrite(lR, HIGH);
  // attaching servo to correct pin
  doorLock.attach(servoPin);
  // attaching Bounce object to the correct pin and setting up interval for debouncing
  openLockBtn.attach(openPin, INPUT_PULLUP);
  openLockBtn.interval(25);
  // initializing bluetooth in serial module and setting up transfer speed - in bauds
  btModule.begin(9600);
  // initializing fingerprint scanner
  fingerScan.begin(57600);
  // checking if the door is open or not and setting up servo to correct position
  if(digitalRead(magnetPin) == HIGH){
    doorLock.write(0);
    lockOpenBehavior();
    delay(500);
  } else {
    doorLock.write(180);
    delay(500);
  }
}
// checking if fingerprint scanner is alive
void fpScannerCheck() {
  // although there is no password set to this fingerprint scanner
  // this function helps with checking if there is an connection to the scanner
  scannerConn = fingerScan.verifyPassword();
  if (scannerConn) {
    defaultScreenSwitcher();
  } else {
    defaultScreenSwitcher();
    while(1){
      // looking up if there's bluetooth connection
      currentTime = millis();
      if (currentTime - btCheckLastTime > btCheck) {
        btCheckLastTime = currentTime;
        btState = digitalRead(btSt);
        checkPrevBtState();
      }
      // if yes then engage maintenance mode
      maintenanceModeMenu();
    }
    }
}
// function comparing previous and current bluetooth state
void checkPrevBtState(){
  if(lastBtState != btState){
    defaultScreenSwitcher();
    lastBtState = btState;
    }
}
//manual lock open and lock open behavior
void manualOpen() {
  openLock();
  lockOpenBehavior();
}

uint8_t unlockWithFingerprint() {
  // switching to scanner - because serial can't listen to more than one thing at once
  fingerprintScanner.listen();
  // fetching image from scanner
  uint8_t p = fingerScan.getImage();
  switch (p) {
    // message that finger has been detected
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,4,3,"Finger","detected!", 200);
      break;
      // re-checking the connection if something goes wrong
    case FINGERPRINT_PACKETRECIEVEERR:
      fpScannerCheck();
    default:
      // if none of above has been return by scanner, check button status
      openLockBtn.update();
      if (openLockBtn.fell()) manualOpen();
      return p;
  }
  // downloading an image from scanner and converting it to the readable format
  p = fingerScan.image2Tz();
  switch (p) {
    // if valis image, then check database
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,1,1,"Checking for","valid match...",200);
      break;
    case FINGERPRINT_IMAGEMESS:
    // if not, then return an error
    lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      // wait until finger is off the scanner
      p = fingerScan.getImage();
    }
  }
  // search database for finger template matching the recently fetched one
  p = fingerScan.fingerSearch();
  switch(p){
    case FINGERPRINT_OK:
      lcdAndLedMsg(LOW,LOW,2,1,"Found match!","Opening lock...",0);
      openLock();
      lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
      while (p != FINGERPRINT_NOFINGER) {
        p = fingerScan.getImage();
      }
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
      break;
    case FINGERPRINT_NOTFOUND:
      lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
      lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
      while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
    default:
      defaultScreenSwitcher();
  }
  return p;
}

// function for closing the lock
void closeLock() {
  // well, this is mostly for remote control
  if(doorLock.read() == 180){
    lcdAndLedMsg(LOW,HIGH,4,1,"Lock is","already closed.",500);
    } else {
    // closing the lock
      lcdAndLedMsg(LOW,LOW,3,0,"Closing...","",0);
      doorLock.write(180);
      delay(500);
      lcdAndLedMsg(HIGH,LOW,4,2,"Lock has","been closed!",500);
    }
  // switch the message on the screen to the appropriate
  defaultScreenSwitcher();
}
// ditto, but for opening
void openLock() {
  if(doorLock.read() == 0){
    lcdAndLedMsg(LOW,HIGH,4,2,"Lock is","already open.",500);
    } else {
    lcdAndLedMsg(LOW,LOW,3,0,"Opening...","",0);
    doorLock.write(0);
    delay(500);
    lcdAndLedMsg(HIGH,LOW,3,2,"Lock has","been opened!",500);
  }
  defaultScreenSwitcher();
}
// this controls behavior when the lock has been opened
void lockOpenBehavior() {
  // this will help measure the timeout to automatically lock the... well, lock.
  unsigned short doorClosedTimeout = 0;
  lcdAndLedMsg(HIGH,LOW,3,1,"Unlocked!","Now open doors.",0);
  // if the lock is closed, then count 10 seconds
  while (digitalRead(magnetPin) == LOW) {
    // if 10 seconds have passed
    if(doorClosedTimeout == 10000){
      // jump to this label and break out of the loop
      goto doorDidntOpen;
      break;
      }
      // a very barbaric method of measuring time, need to switch to millis, but I remember I had problems with it here
      doorClosedTimeout++;
      delay(1);
  }
  magnetHigh:
  // if magnets are still in proximity, await for state change
  lcdAndLedMsg(LOW,LOW,2,1,"Doors open.","Standing by...",0);
  while (digitalRead(magnetPin) == HIGH) {
  }
  // close detection
  lcdAndLedMsg(LOW,LOW,2,0,"Doors shut.","Locking shortly", 2000);
  if(digitalRead(magnetPin) == HIGH){
    // if the doors are closed, and suddenly get open, then jump to this label
    lcdAndLedMsg(LOW,HIGH,1,2,"Doors has been","reopened.", 1000);
    goto magnetHigh;
    }
  // closing
  doorDidntOpen:
  lcdAndLedMsg(LOW,LOW,3,0,"Locking...","", 0);
  closeLock();
}
// the maintenance mode - this is the menu that shows up when bluetooth is connected to the hc-05 module
void maintenanceModeMenu() {
  // switching serial to bluetooth
  btModule.listen();
  // watching the lock open button
  if (currentTime - btnPressLastTime > btnPress){
    btnPressLastTime = currentTime;
      openLockBtn.update();
      if (openLockBtn.fell()) {
        // opening and returning to the maintenance mode
        manualOpen();
      }
    }
  // awaiting for any data in bluetooth module buffer
  if (btModule.available() > 0) {
    btComm = btModule.read();
    switch (btComm) {
      // the PROPER options in maintenance mode
      // most of them checks for scanner, except the lock controls and reset
      case 'a':
        if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        // executes fingerprint addition procedure
        btAddFingerProcedure();
        }
        break;
      case 'd':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        // executes fingerprint removal procedure
        btRemoveFingerProcedure();
        }
        break;
      case 'w':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        // fingerprint database wipe
        btClearDatabaseProcedure();
        }
        break;
      case 'o':
        // remote open
        openLock();
        break;
      case 'c':
        // remote close
        closeLock();
        break;
      case 'r':
        // remote restart
        btRestartConfirmation();
        break;
      // this is supposed to break and go back to the buffer check, but I'm not sure if it's necessary
      default:
        break;
    }
  }
}
// remote reset
void softwareReset() {
  lcdAndLedMsg(LOW,LOW,2,0,"Restarting...","",2000);
  digitalWrite(rst, LOW);
  // theoretically, this should never execute if the function works correctly
  delay(1000);
  digitalWrite(rst, HIGH);
  defaultScreenSwitcher();
}
//w procedurze dodawania i usuwania wpisu z bazy danych odcisków trzeba podać nr ID. ta funkcja się zajmuje konwersją na uint8_t (wg. specyfikacji taki typ danych jest obsługiwany (całkowite nieujemne od 0 do 127 - 8-bitowy integer)
uint8_t getFingerId() {
  //przełączenie na bt, znowu
  btModule.listen();
  //w id będziemy trzymać to, co przekażemy dalej
  uint8_t id;
  //generalnie, trig przełączymy na true jeśli wprowadzimy poprawne it - po to, by popchnąć funkcję dalej.
  bool trig = false;
  while (!trig) {
    //jeśli nic nie zostało wysłane jeszcze to na bieząco odświeżamy i sprawdzamy, czy coś znajduje się w buforze.
    while (!btModule.available()) {}
     id = btModule.parseInt();
     //mówiłem, że może obsługiwać od 0 do 127, aczkolwiek ograniczyłem to do 20. Designer's choice!
     //tak naprawdę z jakiegoś powodu do max tego id mi działał czytnik, powyżej tego nie zapisywał nowych 
     if(id == 0 || id <= 20){
      //jak sie mieści między 0 a 20 to trig na true
       trig = !trig;
     } else {
      //jeśli ww. warunek nie jest spełniony to wracamy do wprowadzania (albo wracamy do menu? sprawdzić!)
      lcdAndLedMsg(LOW,HIGH,2,0,"Please pick","valid id! <1,20>",0);
      }
    }
  return id;
}
void btAddFingerProcedure() {
  lcdAndLedMsg(LOW,LOW,0,0,"Pick ID to save","pattern <1,20>",0);
  //odebranie id z bt i przekazanie do zmiennej
  uint8_t fingerId = getFingerId();
  //jeśli nie 0 to przejść do zapisywania palca
  if (fingerId != 0)
  {
    btEnrollFingerprint(fingerId);
  }
  //jeśli 0 to anulujemy całą procedurę
  else {
    lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",500);
  }
  btModule.listen();
  defaultScreenSwitcher();
}
//jak mamy poprawne ID to dodajemy nowy odcisk
void btEnrollFingerprint(uint8_t fId) {
  //generalnie, dodajemy sobie do bufora msg i id, żeby potem wyświetlić na ekranie w którym id zapisaliśmy ten odcisk
  String msg = "at ID ";
  String msgBuffer = msg + fId;
  //switch na skaner
  fingerprintScanner.listen();
  lcdAndLedMsg(LOW,LOW,1,2,"Place finger","on scanner.",0);
  int p = -1; //czyścimy nasz input ze skanera
  while (p != FINGERPRINT_OK) {
    p = fingerScan.getImage();
    switch (p) {
      case FINGERPRINT_OK:
      lcdAndLedMsg(LOW,LOW,4,3,"Finger","detected!", 200);
        break;
      case FINGERPRINT_NOFINGER:
        break;
      default:
      lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
      //return p;
    }
  }
  //robimy pierwszy zkan do pliku - przy dodawaniu robione są dwa, żeby sprawdzić, czy czasem podczas dodawania nie próbujemy dodać dwóch różnych odcisków palca
  p = fingerScan.image2Tz(1);
  if(p == FINGERPRINT_OK){
    lcdAndLedMsg(LOW,LOW,1,2,"Got the image","of finger.", 200);
    }
  else{
    lcdAndLedMsg(LOW,HIGH,1,1,"Image too messy!","Start all over.",500);
    //return p;
   }
  p = 0;
  lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
  while (p != FINGERPRINT_NOFINGER) {
    p = fingerScan.getImage();
  }
  lcdAndLedMsg(LOW,LOW,2,1,"Now place","finger again.",0);
  p = -1;
  while (p != FINGERPRINT_OK)
  {
    p = fingerScan.getImage();
  }
  lcdAndLedMsg(LOW,LOW,4,3,"Finger","detected!", 200);
  //drugi zrzut do pliku
  p = fingerScan.image2Tz(2);
    if (p == FINGERPRINT_OK){
    lcdAndLedMsg(LOW,LOW,1,2,"Got the image","of finger.", 200);
    } else {
      lcdAndLedMsg(LOW,HIGH,1,1,"Something went","wrong, try again.",500);
      //return p; 
      }
  //porównanie dwóch plików i utworzenie z nich modelu odcisku
  p = fingerScan.createModel();
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,LOW,1,2,"Creating model","of finger...", 100);
  } else {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    //return p;
  }
  //zapisanie modelu do bazy do wpisu o danym id
  p = fingerScan.storeModel(fId);
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(HIGH,LOW,0,3,"Finger is saved",msgBuffer, 1000);
  } else {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    //return p;
  }
  lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
  while (p != FINGERPRINT_NOFINGER) {
    p = fingerScan.getImage();
  }
}
//procedura usuwania - pierw wprowadzenie id od 1 do 20 lub anulowanie operacji
void btRemoveFingerProcedure() {
  lcdAndLedMsg(LOW,LOW,0,0,"Pick ID to wipe","from mem <1,20>",0);
  //pobranie id z buforu bt
  uint8_t fingerId = getFingerId();
  if (fingerId != 0)
  {
    btRemoveFingerConfirmation(fingerId);
  }
  else {
    lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",500);
  }
  btModule.listen();
  defaultScreenSwitcher();
}
void btRemoveFingerprint(uint8_t fId) {
  //znowu przełączka na skaner
  fingerprintScanner.listen();
  uint8_t p = -1;
  //usunięcie modelu znajdującego się pod danym id
  p = fingerScan.deleteModel(fId);
  if (p != FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    //return p;
    }
}
//upewniamy sie, czy chcemy na pewno usunąć model znajdujący sie pod danym id.
void btRemoveFingerConfirmation(uint8_t fId){
  String msgOne = "ID ";
  String msgTwo = " to wipe.";
  //buffer do dalszych komunikatów - umieszczam go w funkcji, bo w sumie tu jest tylko on potrzebny.
  // a może nie...? może wywalić te buffery do headera i potem tylko dodać jako argumenty do funkcji? Bede o tym jeszcze myślał...
  String msgBuffer = msgOne + fId + msgTwo;
  String msgThree = " has been";
  //czekamy co nam user wyśle przez bt y/n
  lcdAndLedMsg(LOW,LOW,1,2,msgBuffer,"Sure? (Y/N)",0);
  while (!btModule.available()) {}
  char option = btModule.read();
  switch (option) {
    case 'y':
      //przełączka na skaner, sklecenie do bufora komunikatu, usunięcie z bazy modelu pod danym id i potwierdzenie
      msgBuffer = msgOne + fId + msgThree;
      fingerprintScanner.listen();
      btRemoveFingerprint(fId);
      lcdAndLedMsg(HIGH,LOW,1,3,msgBuffer,"cleared!",1000);
      break;
    case 'n':
      //anulowanie operacji
      lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",500);
      break;
    default:
      break;
  }
}
//procedura czyszczenia
void btClearDatabaseProcedure() {
  btClearConfirmation();
  btModule.listen();
  defaultScreenSwitcher();
}
//standard, komunikat, czekanie na odp z bufora bt, w zależności od wyboru albo czyszczenie, albo anulowanie operacji
void btClearConfirmation() {
  lcdAndLedMsg(LOW,LOW,0,1,"Clear database.","Really? (Y/N)",0);
  while (!btModule.available()) {}
  char option = btModule.read();
  switch (option) {
    case 'y':
      fingerprintScanner.listen();
      lcdAndLedMsg(LOW,LOW,4,2,"Wiping","database...",500);
      if(fingerScan.verifyPassword()){
        fingerScan.emptyDatabase();
        lcdAndLedMsg(HIGH,LOW,2,0,"Fingerprint","database clear!",1000);
      }
      else {
        lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
      }
      return;
    case 'n':
      lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",1000);
      break;
    default:
      break;
  }
}
//potwierdzenie wyboru restartu układu
void btRestartConfirmation() {
  lcdAndLedMsg(LOW,LOW,2,3,"Restart lock.","Sure? (Y/N)",0);
  while (!btModule.available()) {}
  char option = btModule.read();
  switch (option) {
    case 'y':
      softwareReset();
      return;
    case 'n':
      lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",1000);
      defaultScreenSwitcher();
      break;
    default:
      break;
  }
}
//generyczna funkcja służąca do wyświetlania na ekranie lcd komunikatów oraz zmiany koloru leda zależnie od kontekstu
void lcdAndLedMsg(boolean stateR, boolean stateG, int lcdPos1, int lcdPos2, String msg1, String msg2, int dl){
  digitalWrite(lR, stateR);
  digitalWrite(lG, stateG);
  lcd.clear();
  lcd.setCursor(lcdPos1,0);
  lcd.print(msg1);
  lcd.setCursor(lcdPos2,1);
  lcd.print(msg2);
  if(dl > 0) delay(dl);
  };
//przełączenie domyślnego ekranu pomiędzy oczekiwaniem na palec, błedu skanera i trybu serwisowego
 void defaultScreenSwitcher(){
  switch(scannerConn){
    case true:
    switch(btState){
      case true:
      lcdAndLedMsg(LOW,LOW,0,3,"Maintenance mode","engaged!",0);
      btModule.println(scannerConn);
      break;
      case false:
      lcdAndLedMsg(HIGH,HIGH,3,1,"Waiting for","finger to scan...",0);
      }
      break;
    break;
    case false:
    switch(btState){
      case true:
      lcdAndLedMsg(LOW,LOW,0,3,"Maintenance mode","engaged!",0);
      btModule.println(scannerConn);
      break;
      case false:
      lcdAndLedMsg(LOW,HIGH,2,3,"Scanner error","Connect app.",0);
      }
      break;
    }
  }
