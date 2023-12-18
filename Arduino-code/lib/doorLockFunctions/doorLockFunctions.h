// I'm doing ifndef to prevent double declaration of everything by the program
#ifndef _DOORLOCKFUNCTIONS_H_
#define _DOORLOCKFUNCTIONS_H_

// libraries required
#include <Arduino.h>
#include <Bounce2.h>
#include <Wire.h>
#include <PWMServo.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

// this helps let main.cpp know that those variables are already declared
extern const int lR;
extern const int lG;
extern const int magnetPin;
extern const int servoPin;
extern const int btRx;
extern const int btTx;
extern const int btSt;
extern const int openPin;
extern const int fTx;
extern const int fRx;
extern const int rst;
extern const unsigned short btnPress;
extern const unsigned short btCheck;
extern const unsigned short emergencyCheck;
extern const unsigned short doorDidntOpenTime;
extern const unsigned short fpScanCheck;
extern Bounce openLockBtn;
extern hd44780_I2Cexp lcd;
extern PWMServo doorLock;
extern SoftwareSerial btModule;
extern SoftwareSerial fingerprintScanner;
extern Adafruit_Fingerprint fingerScan;
extern boolean btState; // current bluetooth connection state 
extern boolean scannerConn; // scanner state
extern boolean lastBtState; // last bluetooth state - kept for comparison
extern char btComm; // received content from bluetooth
extern unsigned long btnPressLastTime;
extern unsigned long btCheckLastTime;
extern unsigned long fingerScanLastTime;
extern unsigned long currentTime;
extern unsigned long doorClosedTimeout;
// function declarations
void initialSetup();
void checkPrevBtState();
void fpScannerCheck();
void manualOpen();
uint8_t unlockWithFingerprint();
void closeLock();
void openLock();
void lockOpenBehavior();
void maintenanceModeMenu();
void softwareReset();
void btRestartConfirmation();
uint8_t getFingerId();
void btAddFingerProcedure();
int btEnrollFingerprint(uint8_t fId);
void btRemoveFingerProcedure();
uint8_t btRemoveFingerprint(uint8_t fId, String msgBuffer);
void btRemoveFingerConfirmation(uint8_t fId);
void btClearDatabaseProcedure();
void btClearConfirmation();
void lcdAndLedMsg(boolean stateR, boolean stateG, int lcdPos1, int lcdPos2, String msg1, String msg2, int dl);
void defaultScreenSwitcher();
#endif
