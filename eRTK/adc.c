/*
 * adc.c
 *
 * Created: 27.05.2015 12:57:21
 *  Author: er
 */ 
#include "eRTK.h"

/*
  Der ADC wird von einer Sequencer Liste gesteuert.
  Die Sequenz wird vom System Timer angestossen und lauft bis zum Ende durch.
  In der Liste stehen Bloecke vom Typ <tadu>, jeder steuert einen separaten ADC Kanal.
  In jedem Block wird ein Zaehler erhoeht und falls dieser den Scalerwert erreicht startet dieser Kanal.
  Im anderen Falle wird weitergeschaltet bis zum Ende der Liste.
  Wird ein arbeitsbereiter Block gefunden wird der zugehoerige Kanal zur Wandlung gestartet.
  Nach Ablauf jeder Wandlung startet die "End of Conversion" Interrupt Prozedur und schaltet auf den naechsten Block weiter.
  Am Ende eines Durchlaufes wird dann im Timerinterrupt die Liste neu gestartet.

  Bei 125k adc Takt dauert eine Wandlung etwa 104us.
  Wenn der Sequencer also alle 1000us==1/1000Hz gestartet wird so sind 9 Wandlungen moeglich.    

  Der erlaubte Takt des ADC liegt max. bei 200kHz (2Bit) oder bis 1MHz bei verringerter Genauigkeit (3Bit)
  bei F_CPU von 16MHz gibt es folgenden Zusammenhang:
  Scaler  ADC-Clock   Conversion Time   ADPS2 ADPS1 ADPS0
  /128    125k        104us             1     1     1
  /64     250k         52us             1     1     0
  /32     500k         26us             1     0     1
  /16     1M           13us             1     0     0

*/

typedef struct {
  uint8_t mux;       //0..15 beim atmega256x
  uint8_t ref;       //wert fuer referenz 
  uint8_t scaler;    //0=nie, 1=jeden lauf, 2=jeden 2ten lauf, usw
  uint8_t cnt;       //aufwaertszaehler
  uint16_t value;    //adc messwert
  uint8_t tid;       //wenn eine task gestartet werden soll
 } tadc;


//sequenzer liste mit adc mux selektor und scaler fuer die messrate
tadc adc_cntrl[]={
  { .mux=0, 
    .ref=( 1<<REFS0 ),
    .scaler=10  //bei jedem 10ten lauf messen
   },
  { .mux=8,
    .ref=( 1<<REFS0 ),
    .scaler=1   //bei jedem lauf messen
   }
 };

/*
messzeitpunkte bei verschiedenen scaler einstellungen und anzahl messungen pro zeitpunkt:

time[ms]  scaler=1  scaler=2    scaler=3  scaler=4  scaler=5  scaler=6  scaler=7  scaler=8  scaler=9  scaler=10  cnt  gesamtdauer
0         1         0           0         0         0         0         0         0         0         0           1     104us
1         1         1           0         0         0         0         0         0         0         0           2     208us
2         1         0           1         0         0         0         0         0         0         0           2     208us
3         1         1           0         1         0         0         0         0         0         0           3     312us
4         1         0           0         0         1         0         0         0         0         0           2     208us
5         1         1           1         0         0         1         0         0         0         0           4     416us
6         1         0           0         0         0         0         1         0         0         0           2     208us
7         1         1           0         1         0         0         0         1         0         0           4     416us
8         1         0           1         0         0         0         0         0         1         0           3     312us
9         1         1           0         0         1         0         0         0         0         1           4     416us
10        1         0           0         0         0         0         0         0         0         0           1     104us
11        1         1           1         1         0         1         0         0         0         0           5     520us
12        1         0           0         0         0         0         0         0         0         0           1     104us
13        1         1           0         0         0         0         1         0         0         0           3     312us
14        1         0           1         0         1         0         0         0         0         0           3     312us
15        1         1           0         1         0         0         0         1         0         0           4     416us
16        1         0           0         0         0         0         0         0         0         0           1     104us
17        1         1           1         0         0         1         0         0         1         0           5     520us
18        1         0           0         0         0         0         0         0         0         0           1     104us
19        1         1           0         1         1         0         0         0         0         1           5     520us
*/

#define ANZ_ADC ( sizeof adc_cntrl / sizeof adc_cntrl[0] )

static uint8_t adc_index;  //aktuell wandelnder adc kanal

#if defined (__AVR_ATmega2560__)
ISR( ADC_vect ) { //adc interrupt
  uint8_t m_ready=0;
  register tadc * padc=&adc_cntrl[adc_index];
  if( adc_index<ANZ_ADC ) {
    padc->value=ADCW;
    if( padc->tid ) { //falls ein task suspendiert wartet
      m_ready=1; 
      eRTK_SetReady( padc->tid ); //dann aktivieren 
     }
    //weiterschalten bis zum naechsten bereiten block oder ende der liste
    while( 1 ) {
      if( ++adc_index>=ANZ_ADC ) { //listenende erreicht
        adc_index=0;
        break;
       }
      else {
        ++padc;
        if( ++( padc->cnt ) >= padc->scaler ) { //adc kanal starten
          padc->cnt=0;
          ADMUX=( padc->mux&0x07 ) | padc->ref; 
          if( padc->mux<=7 ) ADCSRB&=~( 1<<MUX5 );
          else ADCSRB|=( 1<<MUX5 );
          ADCSRA|=( 1<<ADSC );
          break;
         }
       }
     }
   }
  else adc_index=0;
  if( m_ready ) eRTK_scheduler(); //scheduling durchfuehren falls eine task aktiviert wurde
 }

uint8_t adc_sequencer( void ) { //soll im system timer interrupt aufgerufen werden
  if( !adc_index ) {
    register tadc * padc=adc_cntrl;
    //finde startklaren adc kanal
    while( padc<&adc_cntrl[ANZ_ADC] ) {
      if( ++( padc->cnt ) >= padc->scaler ) { //adc kanal starten
        padc->cnt=0;
        ADMUX=( padc->mux&0x07 ) | padc->ref;
        if( padc->mux<=7 ) ADCSRB&=~( 1<<MUX5 );
        else ADCSRB|=( 1<<MUX5 );
        ADCSRA|=( 1<<ADSC );
        break;
       }
      ++padc;
     }
    return !0;
   }
  else return 0;
 }

void adc_init( void ) { //beim hochlauf aufzurufen
  ADMUX = 0;  //Kanal waehlen 0-7
  ADMUX |= /*(1<<REFS1) |*/ (1<<REFS0); // avcc Referenzspannung nutzen
  ADCSRA = (1<<ADIE)|(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Frequenzvorteiler setzen auf %128, ADC aktivieren, int aktivieren
 }
#endif

uint16_t adc_get( uint8_t mux ) { //holen des aktuellen wandlungswertes
  uint16_t val=-1;
  register tadc * padc=adc_cntrl;
  while( padc<adc_cntrl+ANZ_ADC ) {
    if( padc->mux==mux ) {
      ATOMIC_BLOCK( ATOMIC_RESTORESTATE	) {
        val=padc->value;
       }
     }
    ++padc;
   }
  return val;
 }

uint16_t adc_wait( uint8_t mux ) { //warten bis auf diesem kanal eine neue messung vorliegt und dann liefern
  register tadc * padc=adc_cntrl;
  while( padc<adc_cntrl+ANZ_ADC ) {
    if( padc->mux==mux ) { //dieser kanal
      uint8_t tid=eRTK_GetTid();
      ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
        padc->tid=tid;
        eRTK_SetSuspended( tid );
        eRTK_scheduler();
       }
      return adc_get( mux );
     }
    ++padc;
   }
  deadbeef( SYS_UNKNOWN );
  return 0;
 }

