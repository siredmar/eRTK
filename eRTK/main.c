/*
 * main.c
 *
 * Created: 04.05.2015 07:54:15
 *  Author: er
 */ 
#include <avr/io.h>
#include "eRTK.h"
#include "uart.h"

void TMain1( uint16_t param0, void *param1 ) { //prio ist 10
  uint32_t loops=0l;
  while( 0 ) { //busy loop auf prio 10
    ++loops;
   }
  while( 1 ) { //com test
    char buffer[50];
    static uint8_t rec;
    tUART h=open( UART0 );
    read( h, NULL, 0, 0 ); //clear rx buffer
    while( h ) {
      write( h, "1", 1 );
      rec=read( h, buffer, 1, 100 );
      write( h, "abcdef", 6 );
      rec=read( h, buffer, 6, 100 );
      write( h, "0123456789ABCDEFGHIJ", 20 );
      rec=read( h, buffer, 20, 100 );
     }
   }
 }

void TMain2( uint16_t param0, void *param1 ) { //prio ist 20
  while( 1 ) { //kurze aktivitaet auf prio 20 muss prio 10 sofort unterbrechen
    eRTK_Sleep_ms( 10 );
   }
 }

const t_eRTK_tcb rom_tcb[VANZTASK]={
  //        adresse    prio p0 p1
  /*1*/  { TMain1,     10, 1, "task1" },
  /*2*/  { TMain2,     20, 2, "task2" },
 };

int main( void ) {
  eRTK_init();
  eRTK_timer_init();
  eRTK_go();
 }
