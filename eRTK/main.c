/*
 * main.c
 *
 * Created: 04.05.2015 07:54:15
 *  Author: er
 */ 

/*
  Dies ist ein Beispiel der Anwendung und ein Test des Betriebssystems.
  Es werden 5 Tasks definiert, eine mit hoher Prioritaet und 4 mit niedrigerer Prioritaet.
  Die Task mit hoher Prio macht nichts anderes als zyklisch alle 10ms wieder zu starten, 
  in der Zwischenzeit suspendiert sie sich.
  Wenn man damit z.B. einen Portpin toggelt kann man den Jitter des erreichbaren Echtzeitverhaltens prüfen.
  Es sollten weniger als etwa 100 CPU Zyklen also bei 16MHz Takt 100/16 us Jitter erreichbar sein.
  Die anderen 4 Tasks nutzen den selben Code (code sharing) lediglich eine Variable indiziert die Instanz der Task
  welche aktuell laeuft, dies wird gleichzeitig in den Datenfeldern als Index genutzt.
  Initial wird ueber den Startparameter param0 dieser Index uebergeben.
  Zusammengefasst: 
    tskHighPrio wird alle 10ms kurz aktiv und beendet sich gleich wieder.   
	tskUART startet viermal, jede Instanz nimmt sich einen seriellen Port (UART)
	und sendet 1..n Zeichen, wenn eine Verbindung Rx<->Tx existiert wird auch etwas empfangen.
	Ansonsten entstehen time outs beim Warten auf die Zeichen.
	Diese Prozesse bestehen aus kurzen Phasen von CPU Aktivitaet und relativ langen Wartezeiten auf die Peripherie.
	Wenn das alles abgearbeitet ist wird die restliche CPU Zeit in der Idle Task verbracht.
	Hier kommen die perfcounter ins Spiel.
	Die Idle Task enthaelt eine Anzeige fuer die CPU Restzeit, 
	also wieviel Anteil die Summe aller Tasks von der verfügbaren Rechenzeit nimmt.
	Im Leerlauf  -> 1000
	Bei 50% Last ->  500
	Überlast     ->    0
	
	Mit diesem Beispiel erreiche ich um 930 Zaehler, also etwa 7% der Zeit ist die CPU in den Tasks unterwegs, 
	der Rest ist Leerlauf.
	Das bei einer Interruptbelastung von 1000Hz vom Systemtimer und 8 seriellen Sende- und Empfangsinterrupts
	mit jeweils 2000Hz bei 19200Bd und 3x1000Hz bei 9600Bd ein gutes Ergebnis !
	
	14.05.2005  T. Erdmann t-erdmann@web.de 
*/

#include <avr/io.h>
#include <stdio.h>
#include "eRTK.h"
#include "uart.h"
#include "adc.h"

void tskHighPrio( uint16_t param0, void *param1 ) { //prio ist 20
  while( 1 ) { //kurze aktivitaet auf prio 20 muss alles auf prio 10 sofort unterbrechen
    eRTK_Sleep_ms( 10 );
   }
 }

#if defined (__AVR_ATmega2560__)
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
    uint8_t rec;
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
#endif

#if defined (__AVR_ATxmega384C3__)
void tskUART( uint16_t param0, void *param1 ) { //prio ist 10
  while( 1 ) {
    eRTK_Sleep_ms( 100 );
  }
}
#endif

#if defined (__AVR_ATmega2560__)

//sequenzer liste mit adc mux selektor und scaler fuer die messrate
tadc adc_cntrl[ANZ_ADC]={
  { .mux=0, .ref=( 1<<REFS0 ), .scaler=10 }, //bei jedem 10ten lauf messen
  { .mux=8, .ref=( 1<<REFS0 ), .scaler=1  }  //bei jedem lauf messen
 };

void tskADC( uint16_t param0, void *param1 ) { //prio ist 15
  while( 1 ) { //task wartet bis neue adc messung vorliegt, kanal 0 hat teiler 10 -> 1000/10=100ms datenrate
    static char txt[10];
    snprintf( txt, sizeof txt, "%u", adc_wait( 0 ) );
    eRTK_Sleep_ms( 10 );
   }
 }
#endif

#if defined (__AVR_ATxmega384C3__)
void tskADC( uint16_t param0, void *param1 ) { //prio ist 15
  while( 1 ) { 
    eRTK_Sleep_ms( 100 );
   }
 }
#endif

const t_eRTK_tcb rom_tcb[VANZTASK]={
  //tid    adresse    prio  p0 p1
  /*1*/  { tskUART,     10, 0, "UART1" },
  /*2*/  { tskUART,     10, 1, "UART2" },
  /*3*/  { tskUART,     10, 2, "UART3" },
  /*4*/  { tskUART,     10, 3, "UART4" },
  /*5*/  { tskHighPrio, 20, 1, "highp" },
  /*6*/  { tskADC,      15, 0, "adc"   }
 };

int main( void ) {
  eRTK_init();
  eRTK_timer_init();
#if defined (__AVR_ATmega2560__)
  adc_init();
#endif
  eRTK_go();
 }
