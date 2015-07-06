/**************************************************************************
 *     PROJEKT:          RTK180 V2.0                                      *
 **************************************************************************
 *                                                                        *
 *     FILENAME:  Seriell.c                                               *
 *                                                                        *
 *     VARIANTE:                                                          *
 *                                                                        *
 *     VERSION:   3.1                                                     *
 *                                                                        *
 *     AUFGABE:   Shell fuer die seriellen Schnittstellen                 *
 *                                                                        *
 *     SPRACHE:   C u. Z180 Assembler                                     *
 *                                                                        *
 *     DATUM:     25. Mai 1992 / 2.5.94                                   *
 *                                                                        *
 *     PROGRAMMIERER:         /te                                         *
 *                                                                        *
 **************************************************************************/
#include "types.h"
#include "io.h"
#include "rtk180.h"      
#include "scc.h"
#include "seriell.h"
#include "data.h"
#include "funcs.h"

#define CNTLA0 0
#define CNTLA1 1
#define CNTLB0 2
#define CNTLB1 3

byte asci0_rxd_nmax;
byte * asci0_rxd_tcd_state;
byte * asci0_txd_tcd_state;

byte asci1_rxd_nmax;
byte * asci1_rxd_tcd_state;
byte * asci1_txd_tcd_state;


static byte asci0_minzeich=0;
static byte asci0_maxzeich=0xff;
static byte asci1_minzeich=0;
static byte asci1_maxzeich=0xff;


#undef SET_PARAMETER_FUER_6MHZ
#ifdef SET_PARAMETER_FUER_6MHZ

static const word tc_scc[8] = { 
                      318, /*   600 */
                      158, /*  1200 */
                       78, /*  2400 */
                       38, /*  4800 */
                       18, /*  9600 */
                        8, /* 19200 */
                        3, /* 38400 */
                        0  /* 76800 (SCC only) */
                   };

static const byte tc_asci[7] = {   
                      bin(0,0,0,0,0,1,1,0), /*   600 */
                      bin(0,0,0,0,0,1,0,1), /*  1200 */
                      bin(0,0,0,0,0,1,0,0), /*  2400 */
                      bin(0,0,0,0,0,0,1,1), /*  4800 */
                      bin(0,0,0,0,0,0,1,0), /*  9600 */
                      bin(0,0,0,0,0,0,0,1), /* 19200 */
                      bin(0,0,0,0,0,0,0,0)  /* 38400 */
                    };

set_parameter( kanal, baud, parity, e_o, nbits, sbits, min, max )
  byte kanal;  /* 0=SCC/A, 1=SCC/B, 2=ASCI0, 3=ASCI1 */
  byte baud;   /* Fq=12MHz: 0=600, 1=1200, 2=2400, 3=4800, 4=9600... */
               /* Fq= 6MHz: 0=300, 1= 600, 2=1200, 3=2400, 4=4800... */
               /* Fq= 3MHz: 0=150, 1= 300, 2= 600, 3=1200, 4=2400... */
  byte parity; /* 0=no parity, 1=parity */
  byte e_o;    /* 0=odd, 1=even */
  byte nbits;  /* 0=7, 1=8 Datenbits */
  byte sbits;  /* 0=1, 1=2 Stopbits */
  byte min;    /* min. gueltiges Zeichen */
  byte max; {
  word temp;
  byte temp_h, temp_l;

  switch( kanal ) {

    case 0: { /* SCC_A */
      #ifdef SCC_VARIABLE
      if( baud<8 ) {
        disable();
        temp=tc_scc[baud];
        temp_l = temp;
        temp_h = temp>>8;
        outbyte( CA, 12 );
        outbyte( CA, temp_l );
        outbyte( CA, 13 );
        outbyte( CA, temp_h );
        temp_l=bin(0,1,0,0,1,1,1,0); /* Wert aus scc.asm */
        if( parity )
          temp_l |= bin(0,0,0,0,0,0,0,1);
        else
          temp_l &= bin(1,1,1,1,1,1,1,0);
        if( e_o )
          temp_l |= bin(0,0,0,0,0,0,1,0);
        else
          temp_l &= bin(1,1,1,1,1,1,0,1);
        if( sbits )
          temp_l |= bin(0,0,0,0,1,1,0,0);
        else {
          temp_l |= bin(0,0,0,0,0,1,0,0);
          temp_l &= bin(1,1,1,1,0,1,1,1);
         }
        outbyte( CA, 4 );
        outbyte( CA, temp_l );
        temp_l=bin(0,1,1,0,1,0,0,0);
        if( nbits )
          temp_l |= bin(0,1,1,0,0,0,0,0);
        else {
          temp_l |= bin(0,0,1,0,0,0,0,0);
          temp_l &= bin(1,0,1,1,1,1,1,1);
         }
        outbyte( CA, 5 );
        outbyte( CA, temp_l );
        scc_a_minzeich=min;
        scc_a_maxzeich=max;
        enable();
       }
      else
        return -1;
      #endif
      break;
     }

    case 1: { /* SCC_B */
      #ifdef SCC_VARIABLE
      if( baud<8 ) {
        disable();
        temp=tc_scc[baud];
        temp_l = temp;
        temp_h = temp>>8;
        outbyte( CB, 12 );
        outbyte( CB, temp_l );
        outbyte( CB, 13 );
        outbyte( CB, temp_h );
        temp_l=bin(0,1,0,0,1,1,1,0);
        if( parity )
          temp_l |= bin(0,0,0,0,0,0,0,1);
        else
          temp_l &= bin(1,1,1,1,1,1,1,0);
        if( e_o )
          temp_l |= bin(0,0,0,0,0,0,1,0);
        else
          temp_l &= bin(1,1,1,1,1,1,0,1);
        if( sbits )
          temp_l |= bin(0,0,0,0,1,1,0,0);
        else {
          temp_l |= bin(0,0,0,0,0,1,0,0);
          temp_l &= bin(1,1,1,1,0,1,1,1);
         }
        outbyte( CB, 4 );
        outbyte( CB, temp_l );
        temp_l=bin(0,1,1,0,1,0,0,0);
        if( nbits )
          temp_l |= bin(0,1,1,0,0,0,0,0);
        else {
          temp_l |= bin(0,0,1,0,0,0,0,0);
          temp_l &= bin(1,0,1,1,1,1,1,1);
         }
        outbyte( CB, 5 );
        outbyte( CB, temp_l );
        scc_b_minzeich=min;
        scc_b_maxzeich=max;
        enable();
       }                 
       
      else
        return -1;
      #endif  
      break;
     }

    case 0: { /* SCC_A */
      break;
     }

    case 1: { /* SCC_B */
      break;
     }

    case 2: { /* ASCI0 */
      if( baud<7 ) {
        temp_l=tc_asci[baud];
        if( e_o )
          temp_l &= bin(1,1,1,0,1,1,1,1);
        else
          temp_l |= bin(0,0,0,1,0,0,0,0);
        outbyte( CNTLB0, temp_l );
        temp_l=inbyte( CNTLA0 );
        if( parity )
          temp_l |= bin(0,0,0,0,0,0,1,0);
        else
          temp_l &= bin(1,1,1,1,1,1,0,1);
        if( nbits )
          temp_l |= bin(0,0,0,0,0,1,0,0);
        else
          temp_l &= bin(1,1,1,1,1,0,1,1);
        if( sbits )
          temp_l |= bin(0,0,0,0,0,0,0,1);
        else
          temp_l &= bin(1,1,1,1,1,1,1,0);
        outbyte( CNTLA0, temp_l );
        asci0_minzeich=min;
        asci0_maxzeich=max;
        asci0_txd_rpos=asci0_txd_wpos=0;
        asci0_rxd_rpos=asci0_rxd_wpos=0;
        asci0_rxd_anz=0;
       }
      else
        return -1;
      break;
     }

    case 3: { /* ASCI1 */
      if( baud<7 ) {
        temp_l=tc_asci[baud];
        if( e_o )
          temp_l &= bin(1,1,1,0,1,1,1,1);
        else
          temp_l |= bin(0,0,0,1,0,0,0,0);
        outbyte( CNTLB1, temp_l );
        temp_l=inbyte( CNTLA1 );
        if( parity )
          temp_l |= bin(0,0,0,0,0,0,1,0);
        else
          temp_l &= bin(1,1,1,1,1,1,0,1);
        if( nbits )
          temp_l |= bin(0,0,0,0,0,1,0,0);
        else
          temp_l &= bin(1,1,1,1,1,0,1,1);
        if( sbits )
          temp_l |= bin(0,0,0,0,0,0,0,1);
        else
          temp_l &= bin(1,1,1,1,1,1,1,0);
        outbyte( CNTLA1, temp_l );
        asci1_minzeich=min;
        asci1_maxzeich=max;
        asci1_txd_rpos=asci1_txd_wpos=0;
        asci1_rxd_rpos=asci1_rxd_wpos=0;
        asci1_rxd_anz=0;
       }
      else
        return -1;
      break;
     }
   } /* switch */
  return 0;
 }

#endif


/************************* asci0 **************************************/

void asci0_clear( void ) {
  disable();
  asci0_rxd_wpos=asci0_rxd_rpos=asci0_rxd_anz=0;
  enable();
 }

static word asci0_holzeich( void ) {
  byte zei;
  ;
  if( !asci0_rxd_anz ) { /* kein Zeichen im Puffer */
    return 0xffff;
   }
  else { /* Zeichen im Puffer */
    disable();
    --asci0_rxd_anz;
    zei=asci0_rxd_buf[asci0_rxd_rpos++];
    enable();
    return zei;
   }
 }


/*	einlesefunktion: rueckgabewert vom typ word
	high byte: 00=normale zeichen, <>00 leseabschluss, ff=timeout
	low byte:  anzahl zeichen im puffer ohne leseabschluss
	1. fall: timeout kein zeichen -> ff00
	2. fall: timeout normale zeichen -> ff03
	3. fall: 3 normale zeichen -> 0003
	4. fall: 3 normale zeichen+lesab 41 -> 4103
*/

word asci0_einlesen( byte * adresse, byte nmax, byte timeout ) { /* adresse = Speicherbereich zum Einlesen */
  static char n;                  /* nmax = max. Zeichenzahl z. Einlesen */
  static word temp;               /* timeout = max. Wartezeit beim Einlesen */
  ;
  disable();
  if( asci0_rxd_anz>=nmax ) { /* schon genug zeichen im puffer */
    enable();
	for( n=0; n<nmax; n++ ) { /* zeichen kopieren */
	  adresse[n]=asci0_holzeich();
     }
	return nmax;
   }
  asci0_rxd_nmax=nmax;                     /* fuer int routine */
  asci0_rxd_tcd_state=&tcd[akttask].state; /* pointer auf tcd.state */

  for( n=0; n<nmax; n++ ) adresse[n]=0;    /* Puffer loeschen */
  n=0;
  x_sefet_disabled( akttask, timeout );
  x_wef(); /* rueckkehr durch timeout oder start durch asci0_rxd */
  systimer[akttask]=0; /* wenn kein timeout auftrat, verhindern dass spaeter
						  durch timerablauf evtl. task unerwuenscht gestartet 
						  wird.
						*/
  while( 1 ) { /* max. nmax Zeichen einlesen */
    temp=asci0_holzeich(); /* ein Zeichen holen */
    /* Zeichen auswerten */
    if( temp>=asci0_minzeich && temp<=asci0_maxzeich ) { /* Wertebereich testen */
      adresse[n++]=temp;
      if( n>=nmax ) return n; /* zahl ist erreicht, kein timeout */
     }
    else {
      if( temp==0xffff ) return 0xff00+n; /* timeout+anzahl */
      else return ( temp<<8 )+n; /* leseabschluss+anzahl */
     }
   }
 }

void asci0_ausgabe( byte * buf, byte n );
#ifndef _MSC_VER
#asm
    public asci0_ausgabe
    extern asci0_txd_rpos
    extern asci0_txd_wpos
    extern asci0_txd_buf
    extern xstop

asci0_warten:
      extern x_pause
      call x_pause;		1 timertakt warten
asci0_ausgabe:;			sp -> return_adress, sp+2 -> string, sp+4=nmax
      ld a,(asci0_txd_rpos)
      ld hl,asci0_txd_wpos
      cp (hl)
      jp nz,asci0_warten
      in0 a,(stat0)
      bit 1,a
      jr z,asci0_warten

      ;preemptive teil
      ;tcd.state pointer fuer irq routine setzen
      ld a, ( akttask )
      ld d, 0
      add a, a
      ld e, a
      ld hl, tcd_adr
      add hl, de;		hl -> tcd_adr[akttask]
      ld a, ( hl );		hl = ( hl )
      inc hl
      ld h, ( hl )
      ld l, a
      ld d, 0
      ld e, OFFSET_STATE
      add hl, de;		hl -> tcd[akttask].state
      ld ( asci0_txd_tcd_state ), hl            

      pop ix       ;returnadresse
      pop hl       ;stringadresse
      pop bc       ; c=nmax

      ld de,asci0_txd_buf
      ld b,0       ;zaehler [0..255]
      ld a,c
      cp 0
      jp z,nullstring0       ;0 zeichen im buffer
      ldir
      ld (asci0_txd_wpos),a  ;zeichenzaehler nach eintragszeiger
      ld a,0
      ld (asci0_txd_rpos),a  ;0 zeichen ausgegeben v. d. string
      di;					damit die irq routine nicht den wef ueberholt
      in0 a,(stat0)
      or 00000001b
      out0 (stat0),a         ;sendeinterrupt freigeben

      extern x_wef
      call x_wef
nullstring0:
      push bc
      push hl
      push ix
      ret
#endasm
#endif

/************************* asci1 **************************************/

void asci1_clear( void ) {
  disable();
  asci1_rxd_wpos=asci1_rxd_rpos=asci1_rxd_anz=0;
  enable();
 }

static word asci1_holzeich( void ) {
  byte zei;
  ;
  if( !asci1_rxd_anz ) { /* kein Zeichen im Puffer */
    return 0xffff;
   }
  else { /* Zeichen im Puffer */
    disable();
    --asci1_rxd_anz;
    zei=asci1_rxd_buf[asci1_rxd_rpos++];
    enable();
    return zei;
   }
 }

/*	einlesefunktion: rueckgabewert vom typ word
	high byte: 00=normale zeichen, <>00 leseabschluss, ff=timeout
	low byte:  anzahl zeichen im puffer ohne leseabschluss
	1. fall: timeout kein zeichen -> ff00
	2. fall: timeout normale zeichen -> ff03
	3. fall: 3 normale zeichen -> 0003
	4. fall: 3 normale zeichen+lesab 41 -> 4103
*/
word asci1_einlesen( byte * adresse, byte nmax, byte timeout ) { /* adresse = Speicherbereich zum Einlesen */
  static char n;                  /* nmax = max. Zeichenzahl z. Einlesen */
  static word temp;               /* timeout = max. Wartezeit beim Einlesen */

  disable();
  if( asci1_rxd_anz>=nmax ) { /* schon genug zeichen im puffer */
    enable();
	for( n=0; n<nmax; n++ ) { /* zeichen kopieren */
      adresse[n]=asci1_holzeich();
     }
	return nmax;
   }
  asci1_rxd_nmax=nmax;                     /* fuer int routine */
  asci1_rxd_tcd_state=&tcd[akttask].state; /* pointer auf tcd.state */

  for( n=0; n<nmax; n++ ) adresse[n]=0;    /* Puffer loeschen */
  n=0;
  x_sefet_disabled( akttask, timeout );
  x_wef(); /* rueckkehr durch timeout oder start durch asci1_rxd */
  systimer[akttask]=0; /* wenn kein timeout auftrat, verhindern dass spaeter
						  durch timerablauf evtl. task unerwuenscht gestartet 
						  wird.
						*/
  while( 1 ) { /* max. nmax Zeichen einlesen */
    temp=asci1_holzeich(); /* ein Zeichen holen */
    /* Zeichen auswerten */
    if( temp>=asci1_minzeich && temp<=asci1_maxzeich ) { /* Wertebereich testen */
      adresse[n++]=temp;
      if( n>=nmax ) return n; /* zahl ist erreicht, kein timeout */
     }
    else {
      if( temp==0xffff ) return 0xff00+n; /* timeout+anzahl */
      else return ( temp<<8 )+n; /* leseabschluss+anzahl */
     }
   }
 }

void asci1_ausgabe( byte * buf, byte n );
#ifndef _MSC_VER
#asm
    public asci1_ausgabe
    extern asci1_txd_rpos
    extern asci1_txd_wpos
    extern asci1_txd_buf

asci1_warten:
      extern x_pause
      call x_pause;		1 timertakt warten
asci1_ausgabe:     ;sp -> return_adress, sp+2 -> string, sp+4=nmax
      ld a,(asci1_txd_rpos)
      ld hl,asci1_txd_wpos
      cp (hl)
      jp nz,asci1_warten
      in0 a,(stat1)
      bit 1,a
      jr z,asci1_warten

      ;preemptive teil
      ;tcd.state pointer fuer irq routine setzen
      ld a, ( akttask )
      ld d, 0
      add a, a
      ld e, a
      ld hl, tcd_adr
      add hl, de;		hl -> tcd_adr[akttask]
      ld a, ( hl );		hl = ( hl )
      inc hl
      ld h, ( hl )
      ld l, a
      ld d, 0
      ld e, OFFSET_STATE
      add hl, de;		hl -> tcd[akttask].state
      ld ( asci1_txd_tcd_state ), hl            

      pop ix       ;returnadresse
      pop hl       ;stringadresse
      pop bc       ;c=nmax

      ld de,asci1_txd_buf
      ld b,0       ;zaehler [0..255]
      ld a,c
      cp 0
      jp z,nullstring1       ;0 zeichen im buffer
      ldir
      ld (asci1_txd_wpos),a  ;zeichenzaehler nach eintragszeiger
      ld a,0
      ld (asci1_txd_rpos),a  ;0 zeichen ausgegeben v. d. string
      di;					damit die irq routine nicht den wef ueberholt
      in0 a,(stat1)
      or 00000001b
      out0 (stat1),a         ;sendeinterrupt freigeben

      extern x_wef
      call x_wef
nullstring1:
      push bc
      push hl
      push ix
      ret

#endasm     
#endif



/********************************* scc part ***************************************/

/*
  die scc i/o erfolgt voll eingebunden im rtk180
  1. senden
  die task sendet das erste zeichen, setzt sich im tcd auf waiting und mit dem letzten tx interupt
  wird sie wieder ready. hierbei ist das letzte zeichen im scc noch nicht vollstaendig aus
  dem tx schieberegister auf die leitung gegangen !
  die rs485 transceiver werden ueber rts angesteuert. dafuer ist voraussetzung, dass cts/dcd dauerhaft
  auf low liegen und auto enables eingeschaltet sind !
  2. empfangen
  die task setzt sich auf waiting und die zeitueberwachung oder das entsprechende empfanszeichen
  setzt sie wieder auf ready
*/

void scc_clear_a( void ) {
  disable();
  scc_a_rxd_wpos=scc_a_rxd_rpos=0;
  enable();
 }
   
void scc_clear_b( void ) {
  disable();
  scc_b_rxd_wpos=scc_b_rxd_rpos=0;
  enable();
 }

static byte scc_a_rx_zeichenanz( void ) {
  return scc_a_rxd_wpos-scc_a_rxd_rpos;
 }
  
static byte scc_b_rx_zeichenanz( void ) {
  return scc_b_rxd_wpos-scc_b_rxd_rpos;
 }

#ifdef RINGCPY_SLOW
ringcpy_slow( dst, ringsrc, nmax ) /* reference funktion ringcpy */
  char * dst; /* empfangspuffer */
  char * ringsrc; /* empfangsringpuffer */
  byte nmax; /* anzahl an zeichen */
  {
  if( ringsrc==scc_a_rxd_buf ) {
    while( nmax-- ) {
      *( dst++ )=scc_a_rxd_buf[scc_a_rxd_rpos++];
     }
   }
  else if( ringsrc==scc_b_rxd_buf ) {
    while( nmax-- ) {
      *( dst++ )=scc_b_rxd_buf[scc_b_rxd_rpos++];
     }
   }
  else message( "ringcpy buffer invalid", 100, 0 );
 }
#endif

static void scc_a_ringcpy( byte * dst, byte nmax ) {
  #ifndef _MSC_VER
  #asm
  pop hl; return
  pop de; dst
  pop bc; nmax
  push bc
  push de
  push hl
  ;
  ld b, c
  xor a
  or b
  ret z;	0 bytes
  ;
  ld h, high scc_a_rxd_buf;		hl->rxd buffer 0..255, page alignment !
  ld a, ( scc_a_rxd_rpos )                
  ld l, a;						hl->rxd_buf[rxd_rpos]
@a:  
  ld a, ( hl )
  ld ( de ), a
  inc l;						inc rpos
  inc de;						inc destination
  djnz @a
  ;
  ld a, l
  ld ( scc_a_rxd_rpos ), a;		store rpos in memory
  #endasm   
  #endif
 }

static void scc_b_ringcpy( byte * dst, byte nmax ) {
  #ifndef _MSC_VER
  #asm
  pop hl; return
  pop de; dst
  pop bc; nmax
  push bc
  push de
  push hl
  ;
  ld b, c
  xor a
  or b
  ret z;	0 bytes
  ;
  ld h, high scc_b_rxd_buf;		hl->rxd buffer 0..255, page alignment !
  ld a, ( scc_b_rxd_rpos )                
  ld l, a;						hl->rxd_buf[rxd_rpos]
@b:  
  ld a, ( hl )
  ld ( de ), a
  inc l;						inc rpos
  inc de;						inc destination
  djnz @b
  ;
  ld a, l
  ld ( scc_b_rxd_rpos ), a;		store rpos in memory
  #endasm   
  #endif
 }  
          
word scc_a_einlesen( byte * adresse, byte nmax, byte timeout ) { /* liefert anzahl eingelesener bytes, bei time out 0! */
  ;       
  memset( adresse, 0, nmax );
  disable();
  if( scc_a_rx_zeichenanz()>=nmax ) { /* genug zeichen da */
    enable();
    scc_a_ringcpy( adresse, nmax );
    return nmax;
   }
  ;
  /* warte bis genug zeichen da sind oder time out */  
  disable();
  /* Timeout Zeitzelle laden */
  systimer[akttask]=timeout;                                              
  /* int2 daten mitteilen */
  scc_a_rxd_anz=nmax;
  scc_a_tcd_ptr=tcd_adr[akttask]; /* zeiger auf den tcd der caller task */
  /* warte auf event, zeichen eingelesen oder time out */
  x_wef(); 
  /* sperre timer und rx event machine */  
  disable();        
  systimer[akttask]=0;  
  scc_a_rxd_anz=0;
  scc_a_tcd_ptr=NULL;
  enable();
  ;        
  /* sind genug zeichen da ? */
  if( scc_a_rx_zeichenanz()>=nmax ) { /* genug zeichen da */
    scc_a_ringcpy( adresse, nmax );
    return nmax;
   }
  else {
    nmax=scc_a_rx_zeichenanz();
    scc_a_ringcpy( adresse, nmax );
    return nmax;
   } 
 }
 
word scc_b_einlesen( byte * adresse, byte nmax, byte timeout ) { /* liefert anzahl eingelesener bytes, bei time out 0! */
  ;       
  memset( adresse, 0, nmax );
  disable();
  if( scc_b_rx_zeichenanz()>=nmax ) { /* genug zeichen da */
    enable();
    scc_b_ringcpy( adresse, nmax );
    return nmax;
   }
  ;
  /* warte bis genug zeichen da sind oder time out */  
  disable();
  /* Timeout Zeitzelle laden */
  systimer[akttask]=timeout;                                              
  /* int2 daten mitteilen */
  scc_b_rxd_anz=nmax;
  scc_b_tcd_ptr=tcd_adr[akttask]; /* zeiger auf den tcd der caller task */
  x_wef(); 
  disable();          
  systimer[akttask]=0;
  scc_b_rxd_anz=0;
  scc_b_tcd_ptr=NULL;
  enable();
  ;
  if( scc_b_rx_zeichenanz()>=nmax ) { /* genug zeichen da */
    scc_b_ringcpy( adresse, nmax );
    return nmax;
   }
  else {
    nmax=scc_b_rx_zeichenanz();
    scc_b_ringcpy( adresse, nmax );
    return nmax;
   } 
 }
 
void scc_a_ausgabe( byte * bufr, byte nmax ); /* es werden alle Zeichen incl. 0H gesendet */
#ifndef _MSC_VER
#asm
    public scc_a_ausgabe
    extern scc_a_txd_rpos
    extern scc_a_txd_wpos
    extern scc_a_txd_buf
    public scc_b_ausgabe
    extern scc_b_txd_rpos
    extern scc_b_txd_wpos
    extern scc_b_txd_buf

scc_a_warten:
      call x_pause
scc_a_ausgabe:     ;sp -> return_adress, sp+2 -> string, sp+4 = nmax
      ld a, ( scc_a_txd_rpos )
      ld hl, scc_a_txd_wpos
      cp ( hl )
      jp nz, scc_a_warten
      ld a, 0
      di
      out ( CA ), a
      nop
      in a, ( CA )
      ei
      bit 2, a
      jr z, scc_a_warten    ;warte bis altes zeichen ausgetaktet
      ;
      pop ix       ;returnadresse
      pop hl       ;stringadresse
      pop bc       ;c=nmax
      push bc
      push hl
      push ix
      ;
      ld de, scc_a_txd_buf
      ld b, 0       ;zaehler=nmax [0..256]
      ld a, c       ;nmax testen
      cp 0
      ret z        ;0 Zeichen im buffer
      ;
      di
      ldir
      ld ( scc_a_txd_wpos ), a  ;nmax=zeichenzaehler nach eintragszeiger
      ld a, 1
      ld ( scc_a_txd_rpos ), a  ;erstes zeichen wird gleich ausgegeben
      ;
      ld a, 1
      out ( CA ), a
      ld a, WR1A
      or 00010010b;		setze int on tx
      out ( CA ), a
      ;
      ld a, 5
      out ( CA ), a
      ld a, WR5A
      or 00000010b;		setze rts aktiv = low pegel am chip
      out ( CA ), a
      ;ei
      ;erst hier write ins daten register sonst geht nichts raus !                 
      ;
      ; -> damit task nicht verdraengt wird bis alles initialisiert ist wird disabled ! 
      ;di
      ld a, ( scc_a_txd_buf )
      out ( DA ), a
      ;                     
      ;setze tcd pointer fuer int2 
      ld a, ( akttask )
      ld l, a
      ld h, 0
      add hl, hl;		akttask*2 da tcd_adr aus word elementen besteht !
      ld de, tcd_adr;	array start addieren
      add hl, de;		hl->tcd_adr[akttask] 
      ld a, ( hl )
      inc hl      
      ld h, ( hl )
      ld l, a;			hl->tcd[akttask]
      ld ( scc_a_tcd_ptr ), hl
      ;setze tcd[akttask].state auf waiting
      ld e, OFFSET_STATE
      ld d, 0
      add hl, de
	  ld a, WAITING
	  ld ( hl ), a; tcd[akttask].state=WAITING
	  ;hier disabled weitergehen, da bei txanz=1 sofort int erfolgt !
      call x_wef; int2 setzt mit letztem zeichen tcd.state auf ready
      ;
      ret
#endasm
#endif


void scc_b_ausgabe( byte * bufr, byte nmax ); /* es werden alle Zeichen incl. 0H gesendet */
#ifndef _MSC_VER
#asm
scc_b_warten:
      call x_pause
scc_b_ausgabe:     ;sp -> return_adress, sp+2 -> string, sp+4 = nmax
      ld a, ( scc_b_txd_rpos )
      ld hl, scc_b_txd_wpos
      cp ( hl )
      jp nz, scc_b_warten
      ld a, 0
      di
      out ( CB ), a
      nop
      in a, ( CB )
      ei
      bit 2, a
      jr z, scc_b_warten    ;warte bis altes zeichen ausgetaktet
      ;
      pop ix       ;returnadresse
      pop hl       ;stringadresse
      pop bc       ;c=nmax
      push bc
      push hl
      push ix
      ;
      ld de, scc_b_txd_buf
      ld b, 0       ;zaehler=nmax [0..256]
      ld a, c       ;nmax testen
      cp 0
      ret z        ;0 Zeichen im buffer
      ;                    
      di
      ldir
      ld ( scc_b_txd_wpos ), a  ;nmax=zeichenzaehler nach eintragszeiger
      ld a, 1
      ld ( scc_b_txd_rpos ), a  ;erstes zeichen wird gleich ausgegeben
      ;
      ld a, 1
      out ( CB ), a
      ld a, WR1B
      or 00010010b;		setze int on tx
      out ( CB ), a
      ;
      ld a, 5
      out ( CB ), a
      ld a, WR5B
      or 00000010b;		setze rts aktiv = low pegel am chip
      out ( CB ), a
      ;ei
      ;erst hier write ins daten register sonst geht nichts raus !                 
      ;
      ; -> damit task nicht verdraengt wird bis alles initialisiert ist wird disabled ! 
      ;di
      ld a, ( scc_b_txd_buf )
      out ( DB ), a
      ;                     
      ;setze tcd pointer fuer int2
      ld a, ( akttask )
      ld l, a
      ld h, 0
      add hl, hl;		akttask*2 da tcd_adr aus word elementen besteht !
      ld de, tcd_adr;	array start addieren
      add hl, de;		hl->tcd_adr[akttask] 
      ld a, ( hl )
      inc hl      
      ld h, ( hl )
      ld l, a;			hl->tcd[akttask]
      ld ( scc_b_tcd_ptr ), hl
      ;setze tcd[akttask].state auf waiting
      ld e, OFFSET_STATE
      ld d, 0
      add hl, de
	  ld a, WAITING
	  ld ( hl ), a; tcd[akttask].state=WAITING
	  ;hier disabled weitergehen, da bei txanz=1 sofort int erfolgt !
      call x_wef; int2 setzt mit letztem zeichen tcd.state auf ready
      ;
      ret

#endasm     
#endif



