# PlayStepMini
The PlayStep Mini is a tiny device that allows a parent to limit screen time from any HDMI device based on “charging up” energy using an exercise stepper.

[![Video](https://img.youtube.com/vi/-LQPoaPvWRQ/0.jpg)](https://www.youtube.com/watch?v=-LQPoaPvWRQ)


[https://oshwlab.com/Ransom/playstepnano_copy_copy_copy_copy](EasyEDA pcb project to clone/edit)

The .hex and .ino source for the Atmega328 are in the Arduino sub-folder.

[The accompanying codedojo article](https://www.codedojo.com/?p=2763)

License for hardware and software: GPL 3.0

Atmega328 setup:

Should be run at 8mhz internal clock. (the PCB supports adding a crystal as well, but you don't need 
that to use the optional ISCP interface to write to the chip.  (well, unless you mess up the clock
fuses, in which case you might.  Or remove the chip and use a TL866 II+ to fix it)

[media/8mhz Arduino fuse settings.png](See this screenshot for fuse settings)

The .ino source file can be compiled with the Arduino IDE, make sure you set it at 8mhz.

Credits: 

PlayStepMini code/design: Seth A. Robinson

Some peizo beeper effects are taken or modified from the Volume Library Demo Sketch (c) 2016 Connor Nishijima which is
released under the GPLv3 license.   https://github.com/connornishijima/arduino-volume




