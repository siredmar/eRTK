# eRTK
Embedded Real Time Kernel for Micros

In eRTK/html/index.html the source documentation is found, 
please read the wiki too.

The project directory holds the ATmega and ATxmega Atmelstudio project.

In SAMD21 the Atmelstudio projekt for SAMD21 is found with its machine setup files.

I didn't use the ASF because i don't want to blow up this project with things that not are necessary for this project.

If one wants to use ASF or some other frame work, please create a new project with that and add the eRTK dirs.

After setting up plls,clock and so on call these
  eRTK_init();

  eRTK_timer_init();

  eRTK_go();

and the system should be up.

Using a similar ATmega, ATxmega or Cortex M0+ CPU is possibly too, try to adapt the #defines to your machine type.

AT(x)mega special: 

The code this time only supports machines with 24bit program counter, so only the ones with more than 128kB flash have a chance to run directly. If the smaller ones should be supported a 16bit program counter option has to be implemented, mostly the context switch push() and pop() are to be adopted.

