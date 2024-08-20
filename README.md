# Fingerprint Lock
This is my most complex - but still unfinished and WIP - project by far.

This is a project of (supposedly) door lock under control of Arduino Nano which is also reading data from the fingerprint scanner, 
which - when the fingerprint image matches one of its entries in the database - steers the servo, allowing access to the storage/room.
The firmware was written in **C/C++** and has been rewritten to the PlatformIO project structure. The companion app was started in **Java** as an Android Studio project, 
but can be also controlled by Bluetooth Serial Terminal - not a specific one, but any that is available on the app store (Play Store, possibly any available for Android)

## Parts

The circuit itself contains
- Arduino Nano - well, as the brains of the whole circuit
- Adafruit Optical Fingerprint Scanner
- some cheap servo,
- magnetic sensor,
- a tactile switch,
- HC-05 Bluetooth module,
- HD44710 16x2 LCD screen with I2C controller,
- a power delivery module - made with 18650 Li-Ion cells, TP4056 battery module, step-up buck converter.
- a single RGB LED.

The complete build has been presented in the included .fzz schematics file, which can be opened with Fritzing.

## Functions

So far, the following functionalities have been implemented:
- Boot sequence, checking if there is a connection with fingerprint scanner - if not, an *Emergency mode* is being initialized and the circuit can
  only be controlled by the Bluetooth commands,
- After detecting the scanner, the circuits checks whether the lock is opened or closed by checking magnetic sensor and adjusts the servo position
  and - depending on the servo position - calls for open lock procedure,
- When the boot sequence is finished - the circuit awaits for the fingerprint,
- When the fingerprint is being detected by the scanner - the scanner seeks for matching entries in database and - if it finds any - changes position of servo,
- After successful fingerprint detection - a procedure handling lock opening behavior is being called and lasts until the lock is fully closed.
- In parallel to the fingerprint handling procedure, the circuit seeks if there is paired connection from the Bluetooth module - if so, the *Maintenance mode* is being called,
- *Maintenance mode* allows for adding and removing fingerprints to the database, as well as wiping the database, remote control of the lock and circuit reboot.

## To be done

Although the circuit works as-is, there still some things to be done, such as:
- probably some code refining, especially moving everything from main loop of the program to the external library,
- some circuit modifications - especially at the power delivery module - it just ***begs*** for some capacitors at the buck converter output
  and reverse polarity protection with diode - don't ask me how I figured that out...
- a dedicated mobile app - sure, I could just leave it as-is, and let it be only controlled by serial terminal, but having a dedicated app
  with ***immutable*** commands, that do not require prior knowledge by the user would be nice.
