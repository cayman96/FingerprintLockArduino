//biblioteki
#include <Arduino.h>
#include <Bounce2.h>
#include <Wire.h>
#include <PWMServo.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
//deklaracja pinów
const int lR = 13;
const int lG = 12;
const int magnetPin = 11;
const int servoPin = 10;
const int btRx = 9;
const int btTx = 8;
const int btSt = 7;
const int openPin = 6;
const int fTx = 5; //biały
const int fRx = 4; //zielony
const int rst = 3;
const unsigned int btnPress = 25;
const unsigned int btCheck = 50;
const unsigned int emergencyCheck = 25;
const unsigned short doorDidntOpenTime = 10000;
const unsigned int fpScanCheck = 100;
extern char btComm;
//pobranie peryferiów zadeklarowanych w pliku głównym
extern Bounce openLockBtn;
extern hd44780_I2Cexp lcd;
extern PWMServo doorLock;
extern SoftwareSerial btModule;
extern SoftwareSerial fingerprintScanner;
extern Adafruit_Fingerprint fingerScan;
//pobranie zmiennych, na których odbędą się operacje z pliku głównym
extern boolean btState;
extern boolean scannerConn;
extern boolean lastBtState;
//nieużywane w funkcjach innych niż loop, ale zarezerwowane na przyszłość.
extern unsigned int btnPressLastTime;
extern unsigned int btCheckLastTime;
extern unsigned int fingerScanLastTime;
extern unsigned int currentTime;
//ustawianie wejścia/wyjścia pinom, określenie serwa i przyciski, rozpoczęcie komunikacji dla czytnika i bluetooth
void initialSetup();
void checkPrevBtState();
//sprawdzenie czy czytnik jest podłączony i tryb awaryjny
void fpScannerCheck();
//manualne otwarcie i wykrywanie palca
void manualOpen();
uint8_t unlockWithFingerprint();
//funkcje otwierania i zamykania drzwi
void closeLock();
void openLock();
void lockOpenBehavior();
//sprawdzenie czy podłączył się ktoś sparowany - zrobić od tego funkcję?
//tryb serwisowy
void maintenanceModeMenu();
//zdalny restart
void softwareReset();
void btRestartConfirmation();
//pobranie id porzez bluetooth
uint8_t getFingerId();
//dodawanie nowego odcisku
void btAddFingerProcedure();
void btEnrollFingerprint(uint8_t fId);
//usuwanie istniejącego odcisku
void btRemoveFingerProcedure();
void btRemoveFingerprint(uint8_t fId);
void btRemoveFingerConfirmation(uint8_t fId);
//czyszczenie bazy
void btClearDatabaseProcedure();
void btClearConfirmation();
//funkcja dot. komunikatów na ekranie i ich zmiany
void lcdAndLedMsg(boolean stateR, boolean stateG, int lcdPos1, int lcdPos2, String msg1, String msg2, int dl);
void defaultScreenSwitcher();
