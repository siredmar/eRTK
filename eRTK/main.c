﻿/*
 * main.c
 *
 * Created: 04.05.2015 07:54:15
 *  Author: er
 */ 
#include <avr/io.h>
#include "eRTK.h"
#include "uart.h"

void tskHighPrio( uint16_t param0, void *param1 ) { //prio ist 20
  while( 1 ) { //kurze aktivitaet auf prio 20 muss alles auf prio 10 sofort unterbrechen
    eRTK_Sleep_ms( 10 );
   }
 }

//tmain2 wird viermal gestartet, param0 ist 0..3
void tskUART( uint16_t param0, void *param1 ) { //prio ist 10
  while( 1 ) { //com test
    char buffer[50];
    static uint8_t rec;
    tUART h=open( UART0+param0 ); //das klappt weil UART0+1=UART2, usw.
    read( h, NULL, 0, 0 ); //clear rx buffer
    while( h ) { //bei einer loop back verbindung RX mit TX verbunden lauft es ohne time out
      write( h, "1", 1 );
      rec=read( h, buffer, 1, 100 );
      write( h, "abcdef", 6 );
      rec=read( h, buffer, 6, 100 );
      write( h, "0123456789ABCDEFGHIJ", 20 );
      rec=read( h, buffer, 20, 100 );
     }
   }
 }

const t_eRTK_tcb rom_tcb[VANZTASK]={
  //tid    adresse    prio  p0 p1
  /*1*/  { tskUART,     10, 0, "UART1" },
  /*2*/  { tskUART,     10, 1, "UART2" },
  /*3*/  { tskUART,     10, 2, "UART3" },
  /*4*/  { tskUART,     10, 3, "UART4" },
  /*5*/  { tskHighPrio, 20, 1, "highp" },
 };

int main( void ) {
  eRTK_init();
  eRTK_timer_init();
  eRTK_go();
 }
