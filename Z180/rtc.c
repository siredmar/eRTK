/*****************************************************************************
 *                                                                           *
 *  FILENAME: RTC.C                                                          *
 *                                                                           *
 *  LANGUAGE: K&R C AND Z180 ASSEMBLER MIXED                                 *
 *                                                                           *
 *  FUNCTION: SHELL FOR RTC65271 CHIP                                        *
 *                                                                           *
 *  SPECIALS:                                                                *
 *  THE CHIP CONTAINS AN RTC MODULE AND AN 4096 BYTE PROTECTED CMOS RAM      *
 *  THE RTC IST CONTROLED BY TWO I/O ADRESSES, AN ADRESS AND A DATA REGISTER *
 *  THE XRAM IS ACCESSED BY 128 PAGES OF 32 BYTES IN I/O SPACE               *
 *  RTC IS IO MAPPED ON BASE D0H                                             *
 *  XRAM IS IO MAPPED ON BASE B0H PARALLEL TO WATCHDOG CIRCUIT               *
 *  CPU PIN                 RTC PIN                                          *
 *  A0----------------------A0                                               *
 *  A1----------------------A1                                               *
 *  A2----------------------A2                                               *
 *  A3----------------------A3                                               *
 *  A8----------------------A4                                               *
 *  A9----------------------A5                                               *
 *  I/O CS XXD0H------------/RTC                                             *
 *  I/O CS XXB0H------------/XRAM                                            *
 *  RTC ADRESS REG..........D0H                                              *
 *  RTC DATA REG............D1H                                              *
 *  XRAM PAGE ACCESS OFFSET 00H..0FH------I/O=00B0H..00BFH                   *
 *  XRAM PAGE ACCESS OFFSET 10H..1FH------I/O=01B0H..01BFH                   *
 *  XRAM PAGE REGISTER--------------------I/O=02B0H                          *
 *                                                                           *
 *  VERSION:  1.0                                                            *
 *                                                                           *
 *  DATE:     23. APRIL 1992     /te                                         *
 *                                                                           *
 *****************************************************************************/

#include "types.h"
#include "io.h"
#include "rtc.h"
#include "data.h"
#include "iomod.h"
#include "text.h"                      
#include "rtk180.h"    
#include "einrb.h"
#include "funcs.h"


#ifdef BCD_BIN
static byte byte_bcd( val )
  byte val; { /* byte nach bcd wandeln */
  static byte temp, ten;

  temp=val;
  ten=0;
  while( temp>=10 ) {
    ++ten;
    temp-=10;
   }
  return ( ten<<4 ) | temp;
 }

static byte bcd_byte( val )
  byte val; { /* bcd nach byte wandeln */
  static byte temp;
  temp=val&0xf0;
  temp>>=4;
  temp*=10;
  temp+=val&0x0f;
  return temp;
 }
#endif

byte rtc_detect( void ) { /* feststellen, ob rtc vorhanden ist -> 1 sonst 0 */
  static byte mask;
  ;
  disable();
  outbyte( RTC_ADRESS, 0xd ); /* register d */
  mask=inbyte( RTC_DATA );
  enable();
  if( mask==0x80 || mask==0x00 ) return 1;
  else return 0;
 }

void rtc_run( void ) { /* startet die rtc */
  outbyte( RTC_ADRESS, 0x0a ); /* register a */
  outbyte( RTC_DATA, 0x20 ); /* oszillator ein */
 }

static byte rtc_reg_rd( byte index ) { /* lese rtc register */
  outbyte( RTC_ADRESS, index );
  return inbyte( RTC_DATA );
 }

static void rtc_reg_wr( byte index, byte val ) { /* schreibe rtc register */
  outbyte( RTC_ADRESS, index );
  outbyte( RTC_DATA, val );
 }
  
void rtc_set( void ) { /* schreiben der aktuellen zeit in den rtc */
  disable();
  outbyte( RTC_ADRESS, 0xb ); /* register b adressieren */
  outbyte( RTC_DATA, 0x86 ); /* set, binaer, 24, keine sommerzeit */
  /* zeit schreiben */
  rtc_reg_wr( RTC_SEC, sekunde );
  rtc_reg_wr( RTC_MIN, minute );
  rtc_reg_wr( RTC_STD, stunde );
  rtc_reg_wr( RTC_TAG, tag );
  rtc_reg_wr( RTC_MON, monat );
  #ifdef UMSETZ
  if( jahr<20 ) { /* 1980-1999 */
    rtc_reg_wr( RTC_JAH, jahr+80 );
   }
  else rtc_reg_wr( RTC_JAH, jahr-20 ); /* 2000-... */
  #endif
  rtc_reg_wr( RTC_JAH, jahr );
  rtc_reg_wr( RTC_DAYCODE, MONDAY ); /* erst mal */
  /* zeit schreiben beenden */
  outbyte( RTC_ADRESS, 0xb ); /* register b */
  outbyte( RTC_DATA, 0x06 ); /* set bit auf 0 */
  enable();
 }

void rtc_get( void ) { /* auslesen des rtc ins ram */
  disable();
  /* uip pollen bis das bit 0 wird */
  while( rtc_reg_rd( 0xa )&0x80 );
  /* 244 usec zeit zum auslesen */
  sekunde=rtc_reg_rd( RTC_SEC );
  minute=rtc_reg_rd( RTC_MIN );
  stunde=rtc_reg_rd( RTC_STD );
  tag=rtc_reg_rd( RTC_TAG );
  monat=rtc_reg_rd( RTC_MON );
  jahr=rtc_reg_rd( RTC_JAH );
  #ifdef UMSETZ
  if( jahr>80 ) jahr-=80; /* 1980-1999 */
  else jahr+=20; /* 2000-... */ 
  #endif
  enable();  
 }

void time_info( void ) {
  TextCopy( display, T_ZEIT_STELLEN );
  TextCopy( display, T_RTC_UHRPRUEF );
  TextCopy( display+60, T_RTC_OK_STELL );
  while( ( !TasteWahl )&&( !TasteStell ) ) {  
    rtc_get();
    wtoa2( tag, display+20+6 );
    wtoa2( monat, display+20+9 );
    wtoa4( jahr+1980, display+20+12 );
    wtoa2( stunde, display+40+6 );
    wtoa2( minute, display+40+9 );
    wtoa2( sekunde, display+40+12 );
    x_sleep( 20 );
   }
  if( TasteStell ) einrb(); /* einstellen noetig */
 }
