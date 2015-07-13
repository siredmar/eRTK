# eRTK
Embedded Real Time Kernel for Micros

In AVR/html/index.html the source documentation is found, 
please read the wiki too.

To learn what a (R)eal (T)ime (K)ernel is please visit https://en.wikipedia.org/wiki/Real-time_operating_system

The project directory subdir AVR/ holds the ATmega and ATxmega Atmelstudio project.

In SAMD21/ the Atmelstudio projekt for SAMD21 is found with its machine setup files and some test code.

The Z180/ directory holds old Zilog Z180 code without makefile because the commercial C compiler i used that time isn't available today. 
So the code could only be used as reference when porting to newer z80 derivates on gcc.

I didn't use the Atmel ASF because i don't want to blow up this project with things that are not necessary for this project.

If one wants to use ASF or some other frame work, please create a new project with that and add the eRTK dirs.

After setting up plls,clock and so on call these

  eRTK_init();

  eRTK_timer_init();

  eRTK_go();

and the system should be started up, highest priority ready state task should be running.

Using a similar ATmega, ATxmega or Cortex M0+ CPU is possible too, 

try to adapt the #defines to your machine type.

AT(x)mega special: 

The code this time only supports machines with 24bit program counter, so only the ones with more than 128kB flash have a chance to run directly. 

If the smaller ones should be supported a 16bit program counter option has to be implemented, mostly the context switch push() and pop() have to be adopted.

