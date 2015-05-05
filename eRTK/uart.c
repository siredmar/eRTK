/*
 * uart.c
 *
 * Created: 04.05.2015 15:52:33
 *  Author: er
 */ 
#include <avr/interrupt.h>
#include "uart.h"

#ifdef UART0_BAUD
static volatile uint8_t rx_buf0[RX0_SIZE];
static volatile uint8_t rx_in0, rx_out0;
static uint8_t tx_buff0[TX0_SIZE];
static volatile uint8_t tx_in0, tx_out0;
#endif

#ifdef UART1_BAUD
static volatile uint8_t rx_buf1[RX1_SIZE];
static volatile uint8_t rx_in1, rx_out1;
static uint8_t tx_buff1[TX1_SIZE];
static volatile uint8_t tx_in1, tx_out1;
#endif

#ifdef UART2_BAUD
static volatile uint8_t rx_buf2[RX2_SIZE];
static volatile uint8_t rx_in2, rx_out2;
static uint8_t tx_buff2[TX2_SIZE];
static volatile uint8_t tx_in2, tx_out2;
#endif

#ifdef UART3_BAUD
static volatile uint8_t rx_buf3[RX0_SIZE];
static volatile uint8_t rx_in3, rx_out3;
static uint8_t tx_buff3[TX3_SIZE];
static volatile uint8_t tx_in3, tx_out3;
#endif

tUART open( tUART port ) {
  eRTK_get_sema( port );
  switch( port ) {
#ifdef UART0_BAUD
    case UART0: {
      UBRR0H = ( UBRR0_VAL>>8 );
      UBRR0L = UBRR0_VAL;	  //set baud rate
      UCSR0A = 0;				    //no U2X, MPCM
      UCSR0C = ( 1<<UCSZ01 )^( 1<<UCSZ00 )^( 1<<USBS0 )^( 1<<UPM01 );	// 8 Bit, even parity
      UCSR0B = ( 1<<RXEN0 )^( 1<<TXEN0 )^( 1<<RXCIE0 );		// enable RX, TX, RX interrupt
      rx_in0=rx_out0=0;	    // set rx buffer empty
      break;
     }
#endif
    default: deadbeef( SYS_UNKNOWN );
   }
  return port;
 }

#ifdef UART0_BAUD
static volatile struct {
  uint8_t tid; //task id die hier wartet
  uint8_t nrx; //anzahl an zeichen auf die gewartet wird
 } eRTK_uart_data0;

ISR( USART0_RX_vect ) { //uart hat ein neues zeichen
  oIDLE( 0 );
  if( !--eRTK_uart_data0.nrx ) {
    if( eRTK_uart_data0.tid ) {
      eRTK_SetReady( eRTK_uart_data0.tid ); //task hat genug input
      eRTK_uart_data0.nrx=0;
      eRTK_uart_data0.tid=0;
     }
   }
  uint8_t i = rx_in0;
  ROLLOVER( i, RX0_SIZE );
  if( i == rx_out0 ) {      //buffer overflow
    UCSR0B&=~( 1<<RXCIE0 ); //disable RX interrupt
    return;                 //discard char
   }
  rx_buf0[rx_in0] = UDR0;   //speichere im ringbuffer
  rx_in0 = i;
 }

ISR( USART0_UDRE_vect ) {   //uart will naechstes zeichen senden
  oIDLE( 0 );
  if( tx_in0==tx_out0 ) {   //nothing to send
    UCSR0B&=~( 1<<UDRIE0 ); //disable TX interrupt
    return;
   }
  UDR0 = tx_buff0[tx_out0]; //zeichen aus ringbuffer auslesen
  ROLLOVER( tx_out0, TX0_SIZE );
 }

#endif

//read liest am port maximal nbytes zeichen in den puffer ein und wartet maximal timeout ms
//read gibt die anzahl eingelesener zeicehn im puffer zurueck
uint8_t read( tUART port, void * puffer, uint8_t nbytes, uint8_t timeout ) {
  register uint8_t anz=0;
  switch( port ) {
#ifdef UART0_BAUD
    case UART0: {
      if( puffer==NULL ) { //clear buffer
        rx_in0=rx_out0;
        return 0;
       }
      UCSR0B|=( 1<<RXCIE0 );		//enable RX interrupt
      while( anz<nbytes ) {     //bis genug im puffer steht
        if( rx_in0!=rx_out0 ) { //puffer auslesen wenn zeichen vorhanden sind
          *( uint8_t * )puffer++=rx_buf0[rx_out0];
          ROLLOVER( rx_out0, RX0_SIZE );
          ++anz;
         }
        else { //auf neue zeichen warten
          if( timeout ) { //wenn ein timeout angegeben wurde
            eRTK_uart_data0.tid=eRTK_GetTid();
            eRTK_uart_data0.nrx=nbytes-anz;
            eRTK_wefet( timeout );
            timeout=0;
           }
          else break;
         }
       }
      break;
     }
#endif
    default: deadbeef( SYS_UNKNOWN );
   }
  return anz;
 }

void write( tUART port, void * puffer, uint8_t nbytes ) {
  switch( port ) {
#ifdef UART0_BAUD
    case( UART0 ): {
      while( nbytes-- ) {
        while( 1 ) { //notfalls warte bis der sendepuffer ein zeichen fassen kann
          register uint8_t i=tx_in0;
          ROLLOVER( i, TX0_SIZE );
          if( i!=tx_out0 ) break;     //noch platz im sendepuffer
          else UCSR0B|=( 1<<UDRIE0 ); //sonst sende interrupt wieder einschalten
          //eRTK_wefet( 1 );            //damit wir nicht aktiv warten
         }
        tx_buff0[tx_in0] = *( uint8_t * )puffer++;
        ROLLOVER( tx_in0, TX0_SIZE );
        UCSR0B|=( 1<<UDRIE0 );        //sende interrupt einschalten
       }
      break;
     }
#endif
    default: deadbeef( SYS_UNKNOWN );
   }
 }

