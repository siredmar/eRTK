/*
 * eRTK.h
 *
 * Created: 27.04.2015 13:38:51
 *  Author: er
 */ 
#include <avr/io.h>
#include <stddef.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#ifndef ERTK_H_
#define ERTK_H_

#define VANZTASK             6         //anzahl definierter prozesse

#define F_CPU       16000000ul

#define eRTKHZ            1000         //system tick rate
#define TIMERPREDIV       64ul         //verteiler timer clock
#define TIMERPRELOAD ( F_CPU/( TIMERPREDIV*eRTKHZ ) )

#if (TIMERPRELOAD>254) /* weil es ein 8 bit timer ist */
#error eRTK:Systematischer 8 Bit Overflow Fehler im SYSTIMER !
#endif

#define ERTKDEBUG                      //gibt ein paar mehr infos zum debugging
//ueberwachungsfunktionen
#define eRTK_STARTUP_MS      0         //solange geben wir allen tasks zusammen zum hochlauf bis wir overload pruefen
#define eRTK_MAX_OVERLOAD    0         //und dies ist die max. erlaubte zahl an overload phasen bevor deadbeef() aufgerufen wird

#define ERTK_STACKSIZE     256

#define IDLELED
#ifdef IDLELED
#ifndef sbi
#define sbi(port,nr) (port|=_BV(nr))
#define cbi(port,nr) (port&=~_BV(nr))
#endif
//diagnose led fuer idle anzeige, wenn gewuenscht
#define oIDLE(a)          { ( a ) ? sbi( PORTE, PE2 ) : cbi( PORTE, PE2 ); sbi( DDRE, PE2 ); }
#define oIDLEfast(a)      { ( a ) ? sbi( PORTE, PE2 ) : cbi( PORTE, PE2 ); }
#else
//sonst zwei nops damit das timing passt
#define oIDLE(a)          /**/
#define oIDLEfast(a)      asm volatile ( "nop\n nop\n" );
#endif

//gruende fuer den toten bueffel
typedef enum { SYS_NOTASK, SYS_NULLPTR, SYS_NULLTIMER, SYS_OVERLOAD, SYS_VERIFY, SYS_UNKNOWN } tsys; 
void deadbeef( tsys reason );          //allgemeine fehler routine

void __attribute__ ((naked)) eRTK_scheduler( void ); /* start der hoechstprioren ready task, notfalls idle */
uint8_t eRTK_GetTimer8( void );        //systemzeit 1000Hz in 8 Bit
uint16_t eRTK_GetTimer16( void );      //systemzeit 1000Hz in 16 Bit
uint8_t eRTK_GetTid( void );           //holen der eigenen task id
void eRTK_SetReady( uint8_t tid );     //task fuer bereit erklaeren
void eRTK_SetSuspended( uint8_t tid ); //task suspendieren
void eRTK_WaitUntil( uint8_t then );   //warte auf den zeitpunkt
void eRTK_Sleep_ms( uint16_t ms );     //warte eine gewisse zeit
void eRTK_get_sema( uint8_t semaid );  /* Warten bis Semaphore frei ist und danach besetzen */
void eRTK_wefet( uint8_t timeout );    //Task suspendieren fuer eine gewisse zeit
void eRTK_init( void );                //initialisieren der datenstrukturen
void eRTK_timer_init( void );          //system timer initialisieren
void eRTK_go( void );                  //start der hoechstprioren ready task, notfalls idle

typedef struct {                       //der task control block
  void ( *task )( uint16_t param0, void *param1 );
  uint8_t prio;
  uint16_t param0;
  void * param1;
 } t_eRTK_tcb;

extern const t_eRTK_tcb rom_tcb[VANZTASK];

#endif /* ERTK_H_ */
