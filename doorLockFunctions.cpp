#include "doorLockFunctions.h"
void initialSetup() {
  pinMode(magnetPin, INPUT_PULLUP);
  digitalWrite(rst, HIGH);
  pinMode(lR, OUTPUT);
  pinMode(lG, OUTPUT);
  pinMode(btSt, INPUT);
  pinMode(rst, OUTPUT);
  digitalWrite(lG, HIGH);
  digitalWrite(lR, HIGH);
  doorLock.attach(servoPin);
  openLockBtn.attach(openPin, INPUT_PULLUP);
  openLockBtn.interval(25);
  btModule.begin(9600);
  fingerScan.begin(57600);
  if(digitalRead(magnetPin) == HIGH){
    doorLock.write(0);
    lockOpenBehavior();
    delay(500);
  } else {
    doorLock.write(180);
    delay(500);
  }
}
void fpScannerCheck() {
  scannerConn = fingerScan.verifyPassword();
  if (scannerConn) {
    defaultScreenSwitcher();
  } else {
    defaultScreenSwitcher();
    while(1){
      currentTime = millis();
      if (currentTime - btCheckLastTime > btCheck) {
      btCheckLastTime = currentTime;
      btState = digitalRead(btSt);
      checkPrevBtState();
      }
      maintenanceModeMenu();
    }
    }
}

void checkPrevBtState(){
  if(lastBtState != btState){
    defaultScreenSwitcher();
    lastBtState = btState;
    }
}

void manualOpen() {
  openLock();
  lockOpenBehavior();
}
void unlockWithFingerprint() {
  fingerprintScanner.listen();
  uint8_t p = fingerScan.getImage();
  switch (p) {
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,4,3,"Finger","detected!", 200);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      fpScannerCheck();
      break;
    default:
      openLockBtn.update();
      if (openLockBtn.fell()) manualOpen();
      return p;
  }
  p = fingerScan.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,1,1,"Checking for","valid match...",200);
      break;
    case FINGERPRINT_IMAGEMESS:
    lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
      return p;
    default:
      return p;
  }
  p = fingerScan.fingerSearch();
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,LOW,2,1,"Found match!","Opening lock...",0);
    openLock();
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
    lockOpenBehavior();
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
    defaultScreenSwitcher();
    return p;
  } 
  
}
void closeLock() {
  if(doorLock.read() == 180){
    lcdAndLedMsg(LOW,HIGH,4,1,"Lock is","already closed.",500);
    } else {
    lcdAndLedMsg(LOW,LOW,3,0,"Closing...","",0);
    doorLock.write(180);
    delay(500);
    lcdAndLedMsg(HIGH,LOW,4,2,"Lock has","been closed!",500);
  }
  defaultScreenSwitcher();
}
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
void lockOpenBehavior() {
  unsigned short doorClosedTimeout = 0;
  lcdAndLedMsg(HIGH,LOW,3,1,"Unlocked!","Now open doors.",0);
  doorClosedTimeout = 0;
  while (digitalRead(magnetPin) == LOW) {
    if(doorClosedTimeout == 10000){
      goto doorDidntOpen;
      break;
      }
      doorClosedTimeout++;
      delay(1);
  }
  magnetHigh:
  lcdAndLedMsg(LOW,LOW,2,1,"Doors open.","Standing by...",0);
  while (digitalRead(magnetPin) == HIGH) {
  }
  lcdAndLedMsg(LOW,LOW,2,0,"Doors shut.","Locking shortly", 2000);
  if(digitalRead(magnetPin) == HIGH){
    lcdAndLedMsg(LOW,HIGH,1,2,"Doors has been","reopened.", 1000);
    goto magnetHigh;
    }
  doorDidntOpen:
  lcdAndLedMsg(LOW,LOW,3,0,"Locking...","", 0);
  closeLock();
}
void maintenanceModeMenu() {
  btModule.listen();
  if (currentTime - btnPressLastTime > btnPress){
    btnPressLastTime = currentTime;
      openLockBtn.update();
      if (openLockBtn.fell()) {
        manualOpen();
      }
    }
  if (btModule.available() > 0) {
    btComm = btModule.read();
    switch (btComm) {
      case 'a':
        if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        btAddFingerProcedure();
        }
        break;
      case 'd':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        btRemoveFingerProcedure();
        }
        break;
      case 'w':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        btClearDatabaseProcedure();
        }
        break;
      case 'o':
        openLock();
        break;
      case 'c':
        closeLock();
        break;
      case 'r':
        btRestartConfirmation();
        break;
      default:
        break;
    }
  }
}
void softwareReset() {
  lcdAndLedMsg(LOW,LOW,2,0,"Restarting...","",2000);
  digitalWrite(rst, LOW);
  delay(1000);
  digitalWrite(rst, HIGH);
}
uint8_t getFingerId() {
  btModule.listen();
  uint8_t id = 0;
  bool trig = false;
  while (!trig) {
    while (!btModule.available()) {}
     id = btModule.parseInt();
     if(id == 0 || id <= 20){
       trig = !trig;
     } else {
      lcdAndLedMsg(LOW,HIGH,2,0,"Please pick","valid id! <1,20>",0);
      }
    }
  return id;
}
void btAddFingerProcedure() {
  lcdAndLedMsg(LOW,LOW,0,0,"Pick ID to save","pattern <1,20>",0);
  uint8_t fingerId = getFingerId();
  if (fingerId != 0)
  {
    btEnrollFingerprint(fingerId);
  }
  else {
    lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",500);
  }
  btModule.listen();
  defaultScreenSwitcher();
}
void btEnrollFingerprint(uint8_t fId) {
  String msg = "at ID ";
  String msgBuffer = msg + fId;
  fingerprintScanner.listen();
  lcdAndLedMsg(LOW,LOW,1,2,"Place finger","on scanner.",0);
  int p = -1;
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
      return p;
    }
  }
  p = fingerScan.image2Tz(1);
  if(p == FINGERPRINT_OK){
    lcdAndLedMsg(LOW,LOW,1,2,"Got the image","of finger.", 200);
    }
  else{
    lcdAndLedMsg(LOW,HIGH,1,1,"Image too messy!","Start all over.",500);
    return p;
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
  p = fingerScan.image2Tz(2);
    if (p == FINGERPRINT_OK){
    lcdAndLedMsg(LOW,LOW,1,2,"Got the image","of finger.", 200);
    } else {
    lcdAndLedMsg(LOW,HIGH,1,1,"Something went","wrong, try again.",500);
      return p; }
  p = fingerScan.createModel();
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,LOW,1,2,"Creating model","of finger...", 100);
  } else {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
  }
  p = fingerScan.storeModel(fId);
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(HIGH,LOW,0,3,"Finger is saved",msgBuffer, 1000);
  } else {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
  }
  lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
  while (p != FINGERPRINT_NOFINGER) {
    p = fingerScan.getImage();
  }
}
void btRemoveFingerProcedure() {
  lcdAndLedMsg(LOW,LOW,0,0,"Pick ID to wipe","from mem <1,20>",0);
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
  fingerprintScanner.listen();
  uint8_t p = -1;
  p = fingerScan.deleteModel(fId);
  if (p != FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
    }
}

void btRemoveFingerConfirmation(uint8_t fId){
  String msgOne = "ID ";
  String msgTwo = " to wipe.";
  String msgBuffer = msgOne + fId + msgTwo;
  String msgThree = " has been";
  lcdAndLedMsg(LOW,LOW,1,2,msgBuffer,"Sure? (Y/N)",0);
  while (!btModule.available()) {}
  char option = btModule.read();
  switch (option) {
    case 'y':
      msgBuffer = msgOne + fId + msgThree;
      fingerprintScanner.listen();
      btRemoveFingerprint(fId);
      lcdAndLedMsg(HIGH,LOW,1,3,msgBuffer,"cleared!",1000);
      break;
    case 'n':
      lcdAndLedMsg(LOW,HIGH,3,3,"Operation","cancelled.",500);
      break;
    default:
      break;
  }
}

void btClearDatabaseProcedure() {
  btClearConfirmation();
  btModule.listen();
  defaultScreenSwitcher();
}
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
