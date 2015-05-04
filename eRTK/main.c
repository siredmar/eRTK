/*
 * main.c
 *
 * Created: 04.05.2015 07:54:15
 *  Author: er
 */ 
#include <avr/io.h>
#include "eRTK.h"

void TMain1( uint16_t param0, void *param1 ) { //prio ist 10
  uint32_t loops=0l;
  while( 1 ) {
    ++loops;
   }
 }

void TMain2( uint16_t param0, void *param1 ) { //prio ist 20
  while( 1 ) {
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
