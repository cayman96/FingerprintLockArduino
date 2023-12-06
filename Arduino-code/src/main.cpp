// any libraries required to work are included in the doorLockFunctions.h file, 
// alongside with pins, functions and modules working in this circuit
#include <doorLockFunctions.h>
void setup() {
  // lcd declaration, pins role declaration and checking if magnetic sensors are close
  initialSetup();
  // check if scanner has connection
  fpScannerCheck();
}
void loop() {
  // main function - first, we're starting counting time from boot of the program
  // we're doing a "fake" parallelization, so that the program can check for certain states in parallel
  currentTime = millis();
  // interval between next bluetooth state check
  if (currentTime - btCheckLastTime > btCheck) {
    btCheckLastTime = currentTime;
    // reading bluetooth status state and comparing it with last detected
    btState = digitalRead(btSt);
    checkPrevBtState();
  }
  // if btState is true, then engage maintenance mode, otherwise check if servo is not open
  if (btState == true) {
    maintenanceModeMenu();
  } else {
    if (doorLock.read() != 180) {
      lockOpenBehavior();
    }
    // interval between checking button press
    if (currentTime - btnPressLastTime > btnPress) {
      btnPressLastTime = currentTime;
      openLockBtn.update();
      // if pressed, manually retract the servo
      if (openLockBtn.fell()) {
        manualOpen();
      }
    }
    // interval between checking if there is a finger on the scanner
    // if yes, then engage unlocking with fingerprint scanner procedure
    if (currentTime - fingerScanLastTime > fpScanCheck) {
      fingerScanLastTime = currentTime;
      unlockWithFingerprint();
    }
  }
}
