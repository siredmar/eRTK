/*
 * eRTK.h
 *
 * Created: 27.04.2015 13:38:51
 *  Author: er
 */ 
#ifndef ERTK_H_
#define ERTK_H_

#include <stddef.h>
#include <stdint.h>

//der task control block
typedef struct { 
  void ( *task )( uint16_t param0, void *param1 );
  uint8_t prio;
  uint16_t param0;
  void * param1;
 } t_eRTK_tcb;

extern const t_eRTK_tcb rom_tcb[VANZTASK]; //das soll in der anwendung definiert sein

void __attribute__ ((naked)) eRTK_scheduler( void ); // start der hoechstprioren ready task, notfalls idle

//eRTK funktionsinterface
void     eRTK_init( void );                //initialisieren der datenstrukturen
void     eRTK_timer_init( void );          //system timer initialisieren
void     eRTK_go( void );                  //start der hoechstprioren ready task, notfalls idle
uint8_t  eRTK_GetTimer8( void );           //systemzeit 1000Hz in 8 Bit
uint16_t eRTK_GetTimer16( void );          //systemzeit 1000Hz in 16 Bit
uint8_t  eRTK_GetTid( void );              //holen der eigenen task id
void     eRTK_SetReady( uint8_t tid );     //task fuer bereit erklaeren
void     eRTK_SetSuspended( uint8_t tid ); //task suspendieren
void     eRTK_WaitUntil( uint8_t then );   //warte auf den zeitpunkt
void     eRTK_Sleep_ms( uint16_t ms );     //warte eine gewisse zeit
void     eRTK_get_sema( uint8_t semaid );  //Warten bis Semaphore frei ist und danach besetzen
void     eRTK_wefet( uint8_t timeout );    //Task suspendieren fuer eine gewisse zeit
//eRTK_cpri() setzen der prioritaet fuer eine task
//tid=0 setze die eigene prioritaet oder sonst die einer anderen task
//prio=neue prioritaet
//schedule_immediately wenn true dann wird sofort eine neue Prozesstabelle ermittelt und der hoechstpriore prozess gestartet
void eRTK_cpri( uint8_t tid, uint8_t prio, uint8_t schedule_immediately );

//gruende fuer einen toten bueffel
typedef enum { SYS_NOTASK, SYS_NULLPTR, SYS_NULLTIMER, SYS_OVERLOAD, SYS_VERIFY, SYS_STACKOVERFLOW, SYS_UNKNOWN } tsys;
	
//allgemeine fehlerbehandlung, kann je nach bedarf umgelenkt werden
void deadbeef( tsys reason );              //allgemeine fehler routine

#endif /* ERTK_H_ */
