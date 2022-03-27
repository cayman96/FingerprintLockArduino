#include "doorLockFunctions.h"
void initialSetup() {
  pinMode(lR, OUTPUT);
  pinMode(lG, OUTPUT);
  pinMode(btSt, INPUT);
  pinMode(rst, OUTPUT);
  pinMode(magnetPin, INPUT_PULLUP); //używam tu input-pullup bo mi sie nie chce podłączać kolejnego rezystora - najwyżej podłączę go jak faktycznie będzie sprawiało to jakies kłopoty, np. jak zamontuje alarm jakiś.
  digitalWrite(rst, LOW); //to powinno powtrzymać wysyłanie sygnału do resetu przy reboocie układu
  digitalWrite(lG, HIGH);
  digitalWrite(lR, HIGH);
  //przypisanie serwa
  doorLock.attach(servoPin);
  //przypisanie przycisku do obiektu Bounce - do jednej nóżki jest podłączony rezystor, bo samo ustawienie input_pullup niestety nic nie dało, pamiętam ten case
  openLockBtn.attach(openPin, INPUT_PULLUP);
  openLockBtn.interval(25);
  //uruchamianie ortu szeregowego do obsługi bt - prędkość 9600 baud
  btModule.begin(9600);
  //uruchamianie portu szeregowego do obsługi czytnika linii papilarnych - więcej danych tu idzie, dlatego pewnie i prędkość wyższa
  fingerScan.begin(57600);
  //o, to jest fajne - na tym etapie program sprawdza, czy magnesy się stykają i jeśli nie - drzwi są otwarte, także czeka sobie, aż user zamknie i na wszelki wypadek chowa wsuwkę zamka.
  if(digitalRead(magnetPin) == HIGH){
    doorLock.write(0);
    lockOpenBehavior();
    delay(500);
  } else {
    doorLock.write(180);
    delay(500);
  }
}
//sprawdzenie, czy skaner jest podłączony do zasilania i do tx/rx
void fpScannerCheck() {
  //gwoli wyjaśnienia - nie ustalałem żadnego hasła dla czytnika.
  //metoda verifyPassword w tym miejscu potrzebna mi jest po to, aby sprawdzić czy skaner jest podłączony.
  scannerConn = fingerScan.verifyPassword();
  if (scannerConn) {
    defaultScreenSwitcher();
  } else {
    defaultScreenSwitcher();
    while(1){
      //czekanie na połączenie bt i odpalenie trybu serwisowego, gdy połączenie zostanie nawiązane
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
//funkcja sprawdzająca stan bt, zmieniająca wartość btState w zależności od połączenia i zmieniająca tekst na ekranie
void checkPrevBtState(){
  if(lastBtState != btState){
    defaultScreenSwitcher();
    lastBtState = btState;
    }
}
//manualne otwarcie po kliknięciu przycisku
void manualOpen() {
  openLock();
  lockOpenBehavior();
}

void unlockWithFingerprint() {
  //przełączenie na skaner - niestety, arduino nie może jednocześnie podsłuchiwać/nadawać domyślnie na dwóch portach szeregowych na raz.
  fingerprintScanner.listen();
  //pobranie obrazu z szybki skanera
  uint8_t p = fingerScan.getImage();
  switch (p) {
    //układ daje znać, że znalazło palec
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,4,3,"Finger","detected!", 200);
      break;
      //sprawdzenie, czy skaner wgl "żyje"
    case FINGERPRINT_PACKETRECIEVEERR:
      fpScannerCheck();
      break;
    default:
      //jeśli 2 ww. warunki się nie spełnią, to sprawdza stan przycisku
      openLockBtn.update();
      if (openLockBtn.fell()) manualOpen();
      return p;
  }
  //pobranie obrazu palca z szybki i kompliacja do zgodnego formatu dla skanera
  p = fingerScan.image2Tz();
  switch (p) {
    //obraz palca poprawny, przechodzi do szukania w bazie
    case FINGERPRINT_OK:
    lcdAndLedMsg(LOW,LOW,1,1,"Checking for","valid match...",200);
      break;
    case FINGERPRINT_IMAGEMESS:
    //jak coś pójdzie nie tak to wypluje komunikat
    lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      //jeśli nic nie wykryje konkretnego to skanuje ponownie
      p = fingerScan.getImage();
    }
      return p;
    default:
      //po prostu idlujej jeśli żaden z ww. warunków się nie spełni
      return p;
  }
  //szuka w bazie zgodnego wzoru palca
  p = fingerScan.fingerSearch();
  //SPRÓBUJ PRZEROBIĆ TEN WARUNEK NA SWITCH..CASE!!!
  if (p == FINGERPRINT_OK) {
    //wzór zgodny, otwiera zamek i request, żeby palec zdjąć
    lcdAndLedMsg(LOW,LOW,2,1,"Found match!","Opening lock...",0);
    openLock();
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
    //funkcja kontrolująca co się dzieje gdy zamek jest otwarty
    lockOpenBehavior();
    return p;
    //
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //w przypadku gdy wystąpi jakiś błąd podczas skanowania to wypluje to i wróci do punktu wyjścia
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    //nie znajdzie żadnego wzoru, zdjąć palec i czeka aż user faktycznie go zdejmie
    lcdAndLedMsg(LOW,HIGH,2,1,"Invalid","fingerprint.",1000);
    lcdAndLedMsg(LOW,LOW,0,2,"Take your finger","off scanner.",0);
    while (p != FINGERPRINT_NOFINGER) {
      p = fingerScan.getImage();
    }
    //zmiana na adekwatny komunikat na screenie
    defaultScreenSwitcher();
    return p;
  } 
  
}
/*closeLock i openLock pisane były z myślą o zdalnym otwieraniu zamka i budową są podobne (może przerobibć to na jedną funkcję?:
 * sprawdzenie, czy zamek jest otwarty lub zamknięty,
 * jeśli chcemy otworzyć/zamknąć, a już jest otwarty/zamknięty to wypluwa komunikat, że już otwarliśmy/zamknęliśmy
 * w przeciwnym wypadku otwiera/zamyka zamek.
 * wyświetla default komunikat, dlatego wywołanie defaultScreenSwitcher
*/
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
//w tej funkcji opisane jest zachowanie zamka w przypadku, gdy drzwi zostaną otwarte
void lockOpenBehavior() {
  unsigned short doorClosedTimeout = 0; //zmienna przechowująca czas od otwarcia zamka (czemu to się nie znajduje w headerze? przenieść i sprawdzić, czy będzie działać)
  lcdAndLedMsg(HIGH,LOW,3,1,"Unlocked!","Now open doors.",0);
  doorClosedTimeout = 0; //jeśli zostawiam to pierwsze, to to jest redundant
  //jeśli zamek jest zamknięty
  while (digitalRead(magnetPin) == LOW) {
    //UWAGA! Pamiętam ten przypadek, chciałem to odmierzać za pomocą millis, ale z jakiegoś powodu za pierwszym razem działało, potem zamykał się od razu -.-
    //jeśli timeout +- 10 sekund to przeskocz do zamknięcia
    if(doorClosedTimeout == 10000){
      goto doorDidntOpen;
      break;
      }
      //zliczamy czas po otwarciu. Again, millis mi nie działało, więc robie to w tak barbarzyński sposób. Ja nie lubjiu eto metady, ale no cóż...
      doorClosedTimeout++;
      delay(1);
  }
  magnetHigh:
  //czuwa sobie, dopóki drzwi nie zostaną zamknięte
  lcdAndLedMsg(LOW,LOW,2,1,"Doors open.","Standing by...",0);
  while (digitalRead(magnetPin) == HIGH) {
  }
  //wykrycie zamknięcia
  lcdAndLedMsg(LOW,LOW,2,0,"Doors shut.","Locking shortly", 2000);
  if(digitalRead(magnetPin) == HIGH){
    //jeśli w międzyczasie się otworzą ponownie to przerywamy i przechodzimy znowu do czuwania
    lcdAndLedMsg(LOW,HIGH,1,2,"Doors has been","reopened.", 1000);
    goto magnetHigh;
    }
  //zamykamy!
  doorDidntOpen:
  lcdAndLedMsg(LOW,LOW,3,0,"Locking...","", 0);
  closeLock();
}
//
void maintenanceModeMenu() {
  //buckle up, buckeroo. tryb serwisowy.
  //przełączam na słuchanie modułu bt, wyżej wspomniałem dlaczego
  btModule.listen();
  //pilnujemy, czy user wcisnął przycisk czy nie wcysnął czasem przycisku, jak tak to otwieramy
  if (currentTime - btnPressLastTime > btnPress){
    btnPressLastTime = currentTime;
      openLockBtn.update();
      if (openLockBtn.fell()) {
        //to jest trochę inna funkcja od openLockBehavior, wytłumaczę dalej dlaczego
        manualOpen();
      }
    }
  //sprawdzamy, czy w buforze modułu bt coś jest
  if (btModule.available() > 0) {
    //czytamy co tam się znajduje się i porównujemy niżej w switch..casie co tam mamy
    btComm = btModule.read();
    switch (btComm) {
      //mógłbym wysyłać liczby, ale w sumie zostałem przy znakach. każdy case prócz resetu, otwarcia i zamknięcia ma jeszcze dodatkowy warunek sprawdzający, 
      //czy czasem się połączenie ze skanerem nie zeptuło - może dodam odświeżanie skanera jakieś? pomyślę nad tym
      case 'a':
        if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        //procedura dodawania nowego odcisku
        btAddFingerProcedure();
        }
        break;
      case 'd':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        //procedura usuwania zapisanego (lub pustego) odcisku o danym id 
        btRemoveFingerProcedure();
        }
        break;
      case 'w':
      if (!scannerConn){
          lcdAndLedMsg(LOW,HIGH,0,0,"Unavailable when","Scanner is off.",750);
          defaultScreenSwitcher();
        } else {
        //wywalenie całej bazy
        btClearDatabaseProcedure();
        }
        break;
      case 'o':
        //zdalne otwarcie
        openLock();
        break;
      case 'c':
        //zdalne zamknięcie
        closeLock();
        break;
      case 'r':
        //zdalny restart
        btRestartConfirmation();
        break;
      //defaultowo non stop sprawdza, czy bufor coś zawiera
      default:
        break;
    }
  }
}
//tu się za bardzo nie będę rozpisywał - restart i tyle.
void softwareReset() {
  lcdAndLedMsg(LOW,LOW,2,0,"Restarting...","",2000);
  digitalWrite(rst, LOW);
  //teoretycznie dwie pozostałe linijki kodu nie powinny nigdy się wykonać, ale w ramach debuggingu je tu zostawiam
  delay(1000);
  digitalWrite(rst, HIGH);
  defaultScreenSwitcher();
}
//w procedurze dodawania i usuwania wpisu z bazy danych odcisków trzeba podać nr ID. ta funkcja się zajmuje konwersją na uint8_t (wg. specyfikacji taki typ danych jest obsługiwany (całkowite nieujemne od 0 do 127 - 8-bitowy integer)
uint8_t getFingerId() {
  //przełączenie na bt, znowu
  btModule.listen();
  //w id będziemy trzymć to, co przekażemy dalej
  uint8_t id = 0;
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
      return p;
    }
  }
  //robimy pierwszy zkan do pliku - przy dodawaniu robione są dwa, żeby sprawdzić, czy czasem podczas dodawania nie próbujemy dodać dwóch różnych odcisków palca
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
  //drugi zrzut do pliku
  p = fingerScan.image2Tz(2);
    if (p == FINGERPRINT_OK){
    lcdAndLedMsg(LOW,LOW,1,2,"Got the image","of finger.", 200);
    } else {
    lcdAndLedMsg(LOW,HIGH,1,1,"Something went","wrong, try again.",500);
      return p; }
  //porównanie dwóch plików i utworzenie z nich modelu odcisku
  p = fingerScan.createModel();
  if (p == FINGERPRINT_OK) {
    lcdAndLedMsg(LOW,LOW,1,2,"Creating model","of finger...", 100);
  } else {
    lcdAndLedMsg(LOW,HIGH,3,1,"An error ","has occured.",1000);
    return p;
  }
  //zapisanie modelu do bazy do wpisu o danym id
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
    return p;
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
