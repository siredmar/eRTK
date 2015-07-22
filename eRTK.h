﻿/*
 * eRTK.h
 *
 * Created: 27.04.2015 13:38:51
 *  Author: er
 */ 
#ifndef ERTK_H_
#define ERTK_H_

#include <stddef.h>
#include <limits.h>

#if defined (__AVR_ATmega2560__)|(__AVR_ATxmega384C3__)
  #include <avr/io.h>
  #include <util/atomic.h>
  #include <avr/interrupt.h>
  typedef uint8_t eRTK_TYPE;
 #endif

#if defined (__SAMD21J18A__)
  #include <samd21.h>
  #include <instance/mtb.h>
  #include <instance/port.h>
  #include "SAMD21/cortex-atomic.h"
  typedef uint32_t eRTK_TYPE;

  //arm cortex systimer register on SAMD21
  #define NVIC_SYSTICK_CTRL		( ( volatile uint32_t *) 0xe000e010 )
  #define NVIC_SYSTICK_LOAD		( ( volatile uint32_t *) 0xe000e014 )
  #define NVIC_SYSTICK_CLK		4
  #define NVIC_SYSTICK_INT		2
  #define NVIC_SYSTICK_ENABLE	1
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

//gruende fuer den toten bueffel
typedef enum { SYS_NOTASK, SYS_NULLPTR, SYS_NULLTIMER, SYS_OVERLOAD, SYS_VERIFY, SYS_STACKOVERFLOW, SYS_UNKNOWN } tsys; 
void deadbeef( tsys reason );              //allgemeine fehler routine


typedef struct {                           //der task control block
	void ( *task )( uint16_t param0, void *param1 );
	uint8_t prio;
	uint16_t param0;
	void * param1;
 } t_eRTK_tcb;

extern const t_eRTK_tcb rom_tcb[VANZTASK]; //das soll in der anwendung definiert sein

void __attribute__ ((naked)) eRTK_scheduler( void ); // start der hoechstprioren ready task, notfalls idle

//eRTK funktionsinterface
void     eRTK_init( void );                  //initialisieren der datenstrukturen
void     eRTK_timer_init( void );            //system timer initialisieren
void     eRTK_go( void );                    //start der hoechstprioren ready task, notfalls idle
uint8_t  eRTK_GetTimer8( void );             //systemzeit 1000Hz in 8 Bit
uint16_t eRTK_GetTimer16( void );            //systemzeit 1000Hz in 16 Bit
eRTK_TYPE  eRTK_GetTid( void );              //holen der eigenen task id
void     eRTK_SetReady( eRTK_TYPE tid );     //task fuer bereit erklaeren
void     eRTK_SetSuspended( eRTK_TYPE tid ); //task suspendieren
void     eRTK_WaitUntil( eRTK_TYPE then );   //warte auf den zeitpunkt
void     eRTK_Sleep_ms( uint16_t ms );       //warte eine gewisse zeit
void     eRTK_get_sema( eRTK_TYPE semaid );  //Warten bis Semaphore frei ist und danach besetzen
void     eRTK_wefet( eRTK_TYPE timeout );    //Task suspendieren fuer eine gewisse zeit
//eRTK_cpri() setzen der prioritaet fuer eine task
//tid=0 setze die eigene prioritaet oder sonst die einer anderen task
//prio=neue prioritaet
//schedule_immediately wenn true dann wird sofort eine neue Prozesstabelle ermittelt und der hoechstpriore prozess gestartet
void eRTK_cpri( eRTK_TYPE tid, eRTK_TYPE prio, eRTK_TYPE schedule_immediately );

#endif /* ERTK_H_ */
