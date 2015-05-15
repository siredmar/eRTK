/*
 * uart.c
 *
 * Created: 04.05.2015 15:52:33
 *  Author: er
 */ 
#include <avr/interrupt.h>
#include "eRTK.h"
#include "uart.h"

#ifdef UART0_BAUD
static volatile uint8_t rx_buf0[RX0_SIZE];
static volatile uint8_t rx_in0, rx_out0;
static uint8_t tx_buff0[TX0_SIZE];
static volatile uint8_t tx_in0, tx_out0;
static uint8_t rxtid0; //task id die hier lesend wartet
static uint8_t nrx0;   //anzahl an zeichen auf die gewartet wird
static uint8_t txtid0; //task id die hier schreibend wartet
#endif

#ifdef UART1_BAUD
static volatile uint8_t rx_buf1[RX1_SIZE];
static volatile uint8_t rx_in1, rx_out1;
static uint8_t tx_buff1[TX1_SIZE];
static volatile uint8_t tx_in1, tx_out1;
static uint8_t rxtid1; //task id die hier lesend wartet
static uint8_t nrx1;   //anzahl an zeichen auf die gewartet wird
static uint8_t txtid1; //task id die hier schreibend wartet
#endif

#ifdef UART2_BAUD
static volatile uint8_t rx_buf2[RX2_SIZE];
static volatile uint8_t rx_in2, rx_out2;
static uint8_t tx_buff2[TX2_SIZE];
static volatile uint8_t tx_in2, tx_out2;
static uint8_t rxtid2; //task id die hier lesend wartet
static uint8_t nrx2;   //anzahl an zeichen auf die gewartet wird
static uint8_t txtid2; //task id die hier schreibend wartet
#endif

#ifdef UART3_BAUD
static volatile uint8_t rx_buf3[RX0_SIZE];
static volatile uint8_t rx_in3, rx_out3;
static uint8_t tx_buff3[TX3_SIZE];
static volatile uint8_t tx_in3, tx_out3;
static uint8_t rxtid3; //task id die hier lesend wartet
static uint8_t nrx3;   //anzahl an zeichen auf die gewartet wird
static uint8_t txtid3; //task id die hier schreibend wartet
#endif

tUART open( tUART port ) {
  eRTK_get_sema( port );
  switch( port ) {
#ifdef UART0_BAUD
    case UART0: {
      UBRR0H = ( UBRR0_VAL>>8 );
      UBRR0L = UBRR0_VAL;	  //set baud rate
      UCSR0A = 0;				    //no U2X, MPCM
      UCSR0C = ( 1<<UCSZ01 )^( 1<<UCSZ00 )^( 1<<USBS0 )^( 0<<UPM01 )^( 0<<UPM00 );	// 8 Bit, no parity
      UCSR0B = ( 1<<RXEN0 )^( 1<<TXEN0 )^( 1<<RXCIE0 );		// enable RX, TX, RX interrupt
      rx_in0=rx_out0=0;	    // set rx buffer empty
      break;
     }
#endif
#ifdef UART1_BAUD
    case UART1: {
      UBRR1H = ( UBRR1_VAL>>8 );
      UBRR1L = UBRR1_VAL;	  //set baud rate
      UCSR1A = 0;				    //no U2X, MPCM
      UCSR1C = ( 1<<UCSZ11 )^( 1<<UCSZ10 )^( 1<<USBS1 )^( 0<<UPM11 )^( 0<<UPM10 );	// 8 Bit, no parity
      UCSR1B = ( 1<<RXEN1 )^( 1<<TXEN1 )^( 1<<RXCIE1 );		// enable RX, TX, RX interrupt
      rx_in1=rx_out1=0;	    // set rx buffer empty
      break;
     }
#endif
#ifdef UART2_BAUD
    case UART2: {
      UBRR2H = ( UBRR2_VAL>>8 );
      UBRR2L = UBRR2_VAL;	  //set baud rate
      UCSR2A = 0;				    //no U2X, MPCM
      UCSR2C = ( 1<<UCSZ21 )^( 1<<UCSZ20 )^( 1<<USBS2 )^( 0<<UPM21 )^( 0<<UPM20 );	// 8 Bit, no parity
      UCSR2B = ( 1<<RXEN2 )^( 1<<TXEN2 )^( 1<<RXCIE2 );		// enable RX, TX, RX interrupt
      rx_in2=rx_out2=0;	    // set rx buffer empty
      break;
     }
#endif
#ifdef UART3_BAUD
    case UART3: {
      UBRR3H = ( UBRR3_VAL>>8 );
      UBRR3L = UBRR3_VAL;	  //set baud rate
      UCSR3A = 0;				    //no U2X, MPCM
      UCSR3C = ( 1<<UCSZ31 )^( 1<<UCSZ30 )^( 1<<USBS3 )^( 0<<UPM31 )^( 0<<UPM30 );	// 8 Bit, no parity
      UCSR3B = ( 1<<RXEN3 )^( 1<<TXEN3 )^( 1<<RXCIE3 );		// enable RX, TX, RX interrupt
      rx_in3=rx_out3=0;	    // set rx buffer empty
      break;
     }
#endif
    default: deadbeef( SYS_UNKNOWN );
   }
  return port;
 }

#ifdef UART0_BAUD
ISR( USART0_RX_vect ) { //uart hat ein neues zeichen
  oIDLE( 0 );
  if( nrx0 ) { //es soll ein event erzeugt werden nach nrx0 zeichen
    if( !--nrx0 ) {
      if( rxtid0 ) {
        eRTK_SetReady( rxtid0 ); //task hat genug input
        rxtid0=0;
       }
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
    if( txtid0 ) {
      eRTK_SetReady( txtid0 ); //set task ready
      txtid0=0;
     }
    UCSR0B&=~( 1<<UDRIE0 ); //disable TX interrupt
    return;
   }
  UDR0 = tx_buff0[tx_out0]; //zeichen aus ringbuffer auslesen
  ROLLOVER( tx_out0, TX0_SIZE );
 }
#endif

#ifdef UART1_BAUD
ISR( USART1_RX_vect ) { //uart hat ein neues zeichen
  oIDLE( 0 );
  if( nrx1 ) { //es soll ein event erzeugt werden nach nrx1 zeichen
    if( !--nrx1 ) {
      if( rxtid1 ) {
        eRTK_SetReady( rxtid1 ); //task hat genug input
        rxtid1=0;
       }
     }
   }
  uint8_t i = rx_in1;
  ROLLOVER( i, RX1_SIZE );
  if( i == rx_out1 ) {      //buffer overflow
    UCSR1B&=~( 1<<RXCIE1 ); //disable RX interrupt
    return;                 //discard char
   }
  rx_buf1[rx_in1] = UDR1;   //speichere im ringbuffer
  rx_in1 = i;
 }

ISR( USART1_UDRE_vect ) {   //uart will naechstes zeichen senden
  oIDLE( 0 );
  if( tx_in1==tx_out1 ) {   //nothing to send
    if( txtid1 ) {
      eRTK_SetReady( txtid1 ); //set task ready
      txtid1=0;
     }
    UCSR1B&=~( 1<<UDRIE1 ); //disable TX interrupt
    return;
   }
  UDR1 = tx_buff1[tx_out1]; //zeichen aus ringbuffer auslesen
  ROLLOVER( tx_out1, TX1_SIZE );
 }
#endif

#ifdef UART2_BAUD
ISR( USART2_RX_vect ) { //uart hat ein neues zeichen
  oIDLE( 0 );
  if( nrx2 ) { //es soll ein event erzeugt werden nach nrx2 zeichen
    if( !--nrx2 ) {
      if( rxtid2 ) {
        eRTK_SetReady( rxtid2 ); //task hat genug input
        rxtid2=0;
	   }
     }
   }
  uint8_t i = rx_in2;
  ROLLOVER( i, RX2_SIZE );
  if( i == rx_out2 ) {      //buffer overflow
    UCSR2B&=~( 1<<RXCIE2 ); //disable RX interrupt
    return;                 //discard char
   }
  rx_buf2[rx_in2] = UDR2;   //speichere im ringbuffer
  rx_in2 = i;
 }

ISR( USART2_UDRE_vect ) {   //uart will naechstes zeichen senden
  oIDLE( 0 );
  if( tx_in2==tx_out2 ) {   //nothing to send
    if( txtid2 ) {
      eRTK_SetReady( txtid2 ); //set task ready
      txtid2=0;
     }
    UCSR2B&=~( 1<<UDRIE2 ); //disable TX interrupt
    return;
   }
  UDR2 = tx_buff2[tx_out2]; //zeichen aus ringbuffer auslesen
  ROLLOVER( tx_out2, TX2_SIZE );
 }
#endif

#ifdef UART3_BAUD
ISR( USART3_RX_vect ) { //uart hat ein neues zeichen
  oIDLE( 0 );
  if( nrx3 ) { //es soll ein event erzeugt werden nach nrx3 zeichen
    if( !--nrx3 ) {
      if( rxtid3 ) {
        eRTK_SetReady( rxtid3 ); //task hat genug input
        rxtid3=0;
	   }
     }
   }
  uint8_t i = rx_in3;
  ROLLOVER( i, RX3_SIZE );
  if( i == rx_out3 ) {      //buffer overflow
    UCSR3B&=~( 1<<RXCIE3 ); //disable RX interrupt
    return;                 //discard char
   }
  rx_buf3[rx_in3] = UDR3;   //speichere im ringbuffer
  rx_in3 = i;
 }

ISR( USART3_UDRE_vect ) {   //uart will naechstes zeichen senden
  oIDLE( 0 );
  if( tx_in3==tx_out3 ) {   //nothing to send
    if( txtid3 ) {
      eRTK_SetReady( txtid3 ); //set task ready
      txtid3=0;
     }
    UCSR3B&=~( 1<<UDRIE3 ); //disable TX interrupt
    return;
   }
  UDR3 = tx_buff3[tx_out3]; //zeichen aus ringbuffer auslesen
  ROLLOVER( tx_out3, TX3_SIZE );
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
            ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
              rxtid0=eRTK_GetTid();
              nrx0=nbytes-anz;
              eRTK_wefet( timeout );
              rxtid0=0;
             }
            timeout=0;
           }
          else break;
         }
       }
      break;
     }
#endif
#ifdef UART1_BAUD
    case UART1: {
      if( puffer==NULL ) { //clear buffer
        rx_in1=rx_out1;
        return 0;
       }
      UCSR1B|=( 1<<RXCIE1 );		//enable RX interrupt
      while( anz<nbytes ) {     //bis genug im puffer steht
        if( rx_in1!=rx_out1 ) { //puffer auslesen wenn zeichen vorhanden sind
          *( uint8_t * )puffer++=rx_buf1[rx_out1];
          ROLLOVER( rx_out1, RX1_SIZE );
          ++anz;
         }
        else { //auf neue zeichen warten
          if( timeout ) { //wenn ein timeout angegeben wurde
            ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
              rxtid1=eRTK_GetTid();
              nrx1=nbytes-anz;
              eRTK_wefet( timeout );
              rxtid1=0;
             }
            timeout=0;
           }
          else break;
         }
       }
      break;
     }
#endif
#ifdef UART2_BAUD
    case UART2: {
      if( puffer==NULL ) { //clear buffer
        rx_in2=rx_out2;
        return 0;
       }
      UCSR2B|=( 1<<RXCIE2 );		//enable RX interrupt
      while( anz<nbytes ) {     //bis genug im puffer steht
        if( rx_in2!=rx_out2 ) { //puffer auslesen wenn zeichen vorhanden sind
          *( uint8_t * )puffer++=rx_buf2[rx_out2];
          ROLLOVER( rx_out2, RX2_SIZE );
          ++anz;
         }
        else { //auf neue zeichen warten
          if( timeout ) { //wenn ein timeout angegeben wurde
            ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
              rxtid2=eRTK_GetTid();
              nrx2=nbytes-anz;
              eRTK_wefet( timeout );
              rxtid2=0;
             }
            timeout=0;
           }
          else break;
         }
       }
      break;
     }
#endif
#ifdef UART3_BAUD
    case UART3: {
      if( puffer==NULL ) { //clear buffer
        rx_in3=rx_out3;
        return 0;
       }
      UCSR3B|=( 1<<RXCIE3 );		//enable RX interrupt
      while( anz<nbytes ) {     //bis genug im puffer steht
        if( rx_in3!=rx_out3 ) { //puffer auslesen wenn zeichen vorhanden sind
          *( uint8_t * )puffer++=rx_buf3[rx_out3];
          ROLLOVER( rx_out3, RX3_SIZE );
          ++anz;
         }
        else { //auf neue zeichen warten
          if( timeout ) { //wenn ein timeout angegeben wurde
            ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
              rxtid3=eRTK_GetTid();
              nrx3=nbytes-anz;
              eRTK_wefet( timeout );
              rxtid3=0;
             }
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
          ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            UCSR0B|=( 1<<UDRIE0 ); //sonst sende interrupt wieder einschalten
            txtid0=eRTK_GetTid(); //damit wir nicht aktiv warten
            eRTK_SetSuspended( txtid0 );
            eRTK_sheduler();
           }
         }
        tx_buff0[tx_in0] = *( uint8_t * )puffer++;
        ROLLOVER( tx_in0, TX0_SIZE );
        UCSR0B|=( 1<<UDRIE0 );        //sende interrupt einschalten
       }
      break;
     }
#endif
#ifdef UART1_BAUD
    case( UART1 ): {
      while( nbytes-- ) {
        while( 1 ) { //notfalls warte bis der sendepuffer ein zeichen fassen kann
          register uint8_t i=tx_in1;
          ROLLOVER( i, TX1_SIZE );
          if( i!=tx_out1 ) break;     //noch platz im sendepuffer
          ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            UCSR1B|=( 1<<UDRIE1 ); //sonst sende interrupt wieder einschalten
            txtid1=eRTK_GetTid(); //damit wir nicht aktiv warten
            eRTK_SetSuspended( txtid1 );
            eRTK_sheduler();
           }
         }
        tx_buff1[tx_in1] = *( uint8_t * )puffer++;
        ROLLOVER( tx_in1, TX1_SIZE );
        UCSR1B|=( 1<<UDRIE1 );        //sende interrupt einschalten
       }
      break;
     }
#endif
#ifdef UART2_BAUD
    case( UART2 ): {
      while( nbytes-- ) {
        while( 1 ) { //notfalls warte bis der sendepuffer ein zeichen fassen kann
          register uint8_t i=tx_in2;
          ROLLOVER( i, TX2_SIZE );
          if( i!=tx_out2 ) break;     //noch platz im sendepuffer
          ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            UCSR2B|=( 1<<UDRIE2 ); //sonst sende interrupt wieder einschalten
            txtid2=eRTK_GetTid(); //damit wir nicht aktiv warten
            eRTK_SetSuspended( txtid2 );
            eRTK_sheduler();
           }
         }
        tx_buff2[tx_in2] = *( uint8_t * )puffer++;
        ROLLOVER( tx_in2, TX2_SIZE );
        UCSR2B|=( 1<<UDRIE2 );        //sende interrupt einschalten
       }
      break;
     }
#endif
#ifdef UART3_BAUD
    case( UART3 ): {
      while( nbytes-- ) {
        while( 1 ) { //notfalls warte bis der sendepuffer ein zeichen fassen kann
          register uint8_t i=tx_in3;
          ROLLOVER( i, TX3_SIZE );
          if( i!=tx_out3 ) break;     //noch platz im sendepuffer
          ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
            UCSR3B|=( 1<<UDRIE3 ); //sonst sende interrupt wieder einschalten
            txtid3=eRTK_GetTid(); //damit wir nicht aktiv warten
            eRTK_SetSuspended( txtid3 );
            eRTK_sheduler();
           }
         }
        tx_buff3[tx_in3] = *( uint8_t * )puffer++;
        ROLLOVER( tx_in3, TX3_SIZE );
        UCSR3B|=( 1<<UDRIE3 );        //sende interrupt einschalten
       }
      break;
     }
#endif
    default: deadbeef( SYS_UNKNOWN );
   }
 }

