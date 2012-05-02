Radiation Pattern Measurement
=============================
Simple test automation setup for measuring radiation patterns of antennas in Carleton University's smaller anechoic chamber (in the microwave lab). The main control/data gathering is taken care of by a host computer running Agilent Vee (developed/tested on 9.2), and the actual turntable that positions the antenna under test is controlled by an Arduino, present in the .vee files as a serial port.

Device Addresses
----------------
The original equipment is an HP 8720A on GPIB address 17, and an Arduino Uno on COM6. These details are likely to be different from yours, so be ready to change them.

Hardware
--------
The turntable has a 3-phase stepper motor, potentiometer to measure position, and (normally-closed, to ground) limit switches. That is the setup the Arduino code is written for, you may need to modify it to fit your hardware.
