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
#include <Std_Types.h>


#if TARGET_CPU == ATMEGA2560
#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
typedef uint8 eRTK_TYPE;
#endif

#if TARGET_CPU == ATMEGA2560
#define F_CPU       (16000000ul)
#endif

//#define ERTK_DEBUG                     //gibt ein paar mehr infos zum debugging, kostet aber laufzeit

#define eRTKHZ            (1000u)         //system tick rate in HZ




//#define IDLELED                        //led in idle task ansteuern oder nichts machen

//ueberwachungsfunktionen
#define eRTK_STARTUP_MS       (0u)         //solange geben wir allen tasks zusammen zum hochlauf bis wir overload pruefen
#define eRTK_MAX_OVERLOAD     (0u)         //und dies ist die max. erlaubte zahl an overload phasen bevor deadbeef() aufgerufen wird


#if TARGET_CPU == ATMEGA2560
#define TIMERPREDIV       64ul         //verteiler timer clock
#define TIMERPRELOAD ( F_CPU/( TIMERPREDIV*eRTKHZ ) )
#if (TIMERPRELOAD>254) /* weil es ein 8 bit timer ist */
#error eRTK:Systematischer 8 Bit Overflow Fehler im SYSTIMER !
#endif
#endif

#if TARGET_CPU == ATMEGA2560
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
#endif

#endif /* ERTK_CONFIG_H_ */
