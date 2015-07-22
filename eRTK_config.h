/*
 * eRTK_config.h
 *
 * Created: 17.07.2015 14:55:34
 *  Author: er
 */ 


#ifndef ERTK_CONFIG_H_
#define ERTK_CONFIG_H_

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if defined (__AVR_ATmega2560__)|(__AVR_ATxmega384C3__)
  #include <avr/io.h>
  #include <util/atomic.h>
  #include <avr/interrupt.h>
  typedef uint8_t eRTK_TYPE;
#elif defined (__SAMD21J18A__)
  #include <samd21.h>
  #include <instance/mtb.h>
  #include <instance/port.h>
  #include "SAMD21/cortex-atomic.h"
  typedef uint32_t eRTK_TYPE;
#endif

#if defined (__AVR_ATmega2560__)
  #define F_CPU       16000000ul
#elif defined (__AVR_ATxmega384C3__)
  #define F_CPU        1000000ul
#elif defined (__SAMD21J18A__)
  #define F_CPU        1000000           //default without further actions
  //#define F_CPU       48000000           //maximum
#endif

#define ERTK_DEBUG                     //gibt ein paar mehr infos zum debugging, kostet aber laufzeit

#define eRTKHZ            1000         //system tick rate in HZ

#define VANZTASK             6         //anzahl definierter prozesse

#define ERTK_STACKSIZE     256         //stack in byte bei 8 bit bzw. word bei 32 bit maschinen
#define ERTK_STACKLOWMARK  240         //wenn debug aktiviert wird auf unterschreitung dieser marke geprueft


#define IDLELED                        //led in idle task ansteuern oder nichts machen

//ueberwachungsfunktionen
#define eRTK_STARTUP_MS       0         //solange geben wir allen tasks zusammen zum hochlauf bis wir overload pruefen
#define eRTK_MAX_OVERLOAD     0         //und dies ist die max. erlaubte zahl an overload phasen bevor deadbeef() aufgerufen wird


#if defined (__AVR_ATmega2560__)
  #define TIMERPREDIV       64ul         //verteiler timer clock
  #define TIMERPRELOAD ( F_CPU/( TIMERPREDIV*eRTKHZ ) )
  #if (TIMERPRELOAD>254) /* weil es ein 8 bit timer ist */
    #error eRTK:Systematischer 8 Bit Overflow Fehler im SYSTIMER !
  #endif
#endif

#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
  #ifdef IDLELED
    #ifndef sbi
      #define sbi(port,nr) (port|=_BV(nr))
      #define cbi(port,nr) (port&=~_BV(nr))
    #endif
    //diagnose led fuer idle anzeige, wenn gewuenscht
    #define oIDLE(a)          { (a) ? sbi( PORTE, PE2 ) : cbi( PORTE, PE2 ); sbi( DDRE, PE2 ); }
    #define oIDLEfast(a)      { (a) ? sbi( PORTE, PE2 ) : cbi( PORTE, PE2 ); }
  #else
    //sonst zwei nops damit das timing passt
    #define oIDLE(a)          /**/
    #define oIDLEfast(a)      asm volatile ( "nop\n nop\n" );
  #endif
#elif defined (__SAMD21J18A__)
  #ifdef IDLELED
    //diagnose led fuer idle anzeige, wenn gewuenscht
      #define oIDLE(a)          { (a) ? ( REG_PORT_OUT1|=( 1<<30 ) ) : ( REG_PORT_OUT1&=~( 1<<30 ) ); REG_PORT_DIR1|=( 1<<30 ); }
      #define oIDLEfast(a)      { (a) ? ( REG_PORT_OUT1|=( 1<<30 ) ) : ( REG_PORT_OUT1&=~( 1<<30 ) ); }
    #else
      #define oIDLE(a)          /**/
      #define oIDLEfast(a)      /**/
    #endif
#endif

#endif /* ERTK_CONFIG_H_ */
