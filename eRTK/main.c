/*
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

//tskUART wird viermal gestartet, param0 ist 0..3
//es wird derselbe programm code genutzt, anhand des param0 wird UART0..UART3 in jeder task genutzt
//und 1 byte, 16 byte == sendepuffergroesse oder 20 byte gesendet und wieder zurueckgelesen
//also eine sog. loop back gemacht.
//wird nichts empfangen so gibt es time outs.
//wird alles empfangen so geht es unverzueglich weiter in der schleife.
//ist gedacht als belastungstest der taskwechselmechanismen und datenstrukturen.
void tskUART( uint16_t param0, void *param1 ) { //prio ist 10
  while( 1 ) { //com test
    char buffer[50];
    static uint8_t rec;
    tUART h=open( UART0+param0 ); //das klappt weil UART0+1=UART1, usw.
    while( h ) { //bei einer loop back verbindung wird RX mit TX verbunden und es laeuft ohne time out
      read( h, NULL, 0, 0 ); //clear rx buffer
      write( h, "1", 1 ); //schreibe ein zeichen auf die leitung
      rec=read( h, buffer, 1, 100 ); //lies ein zeichen mit 100ms time out
      write( h, "abcdef", 6 );
      rec=read( h, buffer, 6, 100 );
      write( h, "0123456789ABCDEFGHIJ", 20 ); //hier ist der auszugebende string laenger als der interne puffer, es kommt ein spezieller mechanismus zum tragen, der abwartet bis der sendepuffer leer ist
      rec=read( h, buffer, 16, 100 ); //hier muss timeout entstehen da der empfangspuffer nur auf 16 zeichen eingestellt ist und wir nicht rechtzeitig auslesen koennen bevor ein overflow entsteht
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
