#include "types.h"
#include "..\telegram.h"
#include "vm.h"
#include "seriell.h"
#include "funk.h"
#include "funcs.h"  
#include "rtk180.h"
#include "scc.h"
#include "ssn180.h" 
#include "iomod.h"
#include "logger.h"

/* read only ankopplung der wiegeeinheit an ein modacom funkmodem:
   sobald eine connection hergestellt ist, wird ein string gesendet
               "chip:............,tt.mm.jjjj,ss:mm:ss"
   danach wird die verbindung von hier wieder getrennt.
   weitere eingriffe erfolgen nicht !
   zugriff auf eichpflichtige daten erfolgt nicht       
                                       
   die wiegeeinheit geht mit der at-sequenz ato/connect ständig auf
   abfrage bis eine verbindung steht.
   danach kommen daten mit denen die struktur moda aufgefüllt wird.
   die dort enthaltene abskennung ist für die antwort dann die empkennung !
   
*/               


typedef struct {
  char mg[2];
  char abskennung[14]; /* modem kennung wird eingetragen */
  char empkennung[14];
  char cr;
  char flags[2];
  char date[5];
  char cr2;
  char stx;
  char command; /* 0x5c */
  char ssn[12]; /* seriennummer waage */
  s_last_chip last_chip; /* cipnummer, date, time */
  char etx;
 } s_modacom; 

static s_modacom moda;
 
static void ClearModaData( void ) {
  memset( ( byte * )&moda, 0, sizeof( moda ) );
 }

static void PrepareAnswerData( void ) { /* aufbereiten der daten für die antwort */
  static char z;
  static char swap[14];
  /* modacom part */
  memcpy( ( byte * )&moda.mg, ( byte * )"MG", 2 );
  memcpy( ( byte * )&swap, ( byte * )moda.abskennung, 14 ); /* merke absenderkennung */
  memcpy( ( byte * )&moda.abskennung, ( byte * )&moda.empkennung, 14 );
  memcpy( ( byte * )&moda.empkennung, ( byte * )swap, 14 );
  /* daten part */
  for( z=0; z<6; z++ ) { /* silicon serial number der waage */
    moda.ssn[z+z]=ASCII[code[z+1]>>4];
    moda.ssn[z+z+1]=ASCII[code[z+1]&0xf];
   }
  vm_last_chip( &moda.last_chip ); /* daten der letzten wiegung holen */
 }
  

void Tfunk( void ) {  /* funkverbindung */
  static byte z, anz, bufpos, online;         
  static int channel; /* welcher kanal ist aktiv */                                             
  static int sts;
  static char buffer[26];
  static char OldText[80]; /* ascii dump repeat buffer */
  char * p;
  ;      
  /*  while( 1 ) {
      sts=vm_check();
      logprintf( "\r\nTfunk sts=%x", sts );
      x_sleep( 50 );
     }
  */    
  /* rueckgabedaten loeschen */
  ClearModaData();
  memset( ( byte * )&moda.last_chip, 0x20, sizeof( s_last_chip ) );
  memcpy( ( byte * )&moda.last_chip, ( byte * )"CHIP", 4 );
  sts=SCCFound();
  if( !sts ) { /* kein scc */
    logs( "NO SCC" );
    while( 1 ) {   
      x_sleep( 128 );
     }
   }   
  ; 
  memset( OldText, 0, sizeof OldText );
  scc_clear_a();       
  scc_clear_b();
  if( iotest() ) { /* mit iotest auch test des scc */
    anz=26;
    scc_a_ausgabe( "ABCDEFGHIJKLMNOPQRSTUVWXYZ", anz ); 
    scc_b_ausgabe( "ABCDEFGHIJKLMNOPQRSTUVWXYZ", anz ); 
    while( 1 ) { /* test echo */
      anz=scc_a_einlesen( buffer, anz, 250 );
      if( anz ) {
        scc_a_ausgabe( buffer, anz );
       }
      anz=scc_b_einlesen( buffer, anz, 250 );
      if( anz ) {
        scc_b_ausgabe( buffer, anz );
       }
     }  
   }	  	        
  ;          
neustart:
  logs( "MODEM NEUSTART" );
  while( 1 ) { /* do ascii dump to notebook pc */
    p=GetASCIIDump();
	if( p ) {
	  strncpy( OldText, p, sizeof OldText );
	 }
	else {
	  x_sleep( 2000/5 ); /* 2 s */
	 }
	if( OldText[0] ) {
	  scc_a_ausgabe( OldText, strlen( OldText ) ); /* raw data output */
	  /*scc_b_ausgabe( OldText, strlen( OldText ) );*/
	 }  
   }

  #ifdef MODACOM
  channel=-1;                       
  while( channel==-1 ) { /* hier wird auf ok von der modem line gewartet */
    scc_a_ausgabe( "AT\r", 3 );
    anz=scc_a_einlesen( buffer, 10, 100 );
    if( anz ) {
      for( z=0; z<anz-1; z++ ) {
        if( buffer[z]=='O' && buffer[z+1]=='K' ) channel=0;  /* ok vom modem */
       }
     }
    scc_b_ausgabe( "AT\r", 3 );
    anz=scc_b_einlesen( buffer, 10, 100 );
    if( anz ) {
      for( z=0; z<anz-1; z++ ) {
        if( buffer[z]=='O' && buffer[z+1]=='K' ) channel=1;  /* ok vom modem */
       }
     }
   }
  logs( "AT/OK PASSED" );
  while( 1 ) { /* sende ato bis antwort connect kommt */
    if( channel==0 ) { /* kanal a */
      scc_clear_a();
      scc_a_ausgabe( "ATO\r", 4 );
      online=bufpos=anz=0;
      for( z=0; z<60; z++ ) { /* 60 mal mit 1 sec timeout lesen */
        anz=scc_a_einlesen( &buffer[bufpos], 1, 1000/5 ); 
        if( anz ) ++bufpos;
        if( bufpos>=sizeof( buffer ) ) break;
        if( !memcmp( buffer, "CONNECT", 7 ) ) {
          online=1;
          break;
         }
        if( !anz ) goto neustart; /* nach time out neu beginnen */
       }
      if( !online ) continue;
      ;
      anz=scc_a_einlesen( ( byte * )&moda, sizeof( moda ), 0xff ); /* 1,25 sec */
      if( !anz ) continue;
      PrepareAnswerData();
      scc_a_ausgabe( ( byte * )&moda, sizeof( moda ) );
      for( z=0; z<3; z++ ) { /* breche verbindung ab */
        x_sleep( 1000/5 );
        scc_a_ausgabe( "+++", 3 );   
        x_sleep( 1000/5 );
        scc_a_ausgabe( "ATH\r", 4 );
       } 
      goto neustart;
     }
    else { /* kanal b */
      scc_clear_b();
      scc_b_ausgabe( "ATO\r", 4 );
      online=bufpos=anz=0;
      for( z=0; z<60; z++ ) { /* 60 mal mit 1 sec timeout lesen */
        anz=scc_b_einlesen( &buffer[bufpos], 1, 1000/5 ); 
        if( anz ) { logc( buffer[bufpos] ); ++bufpos; }
        if( bufpos>=sizeof( buffer ) ) break;
        if( !memcmp( buffer, "CONNECT", 7 ) ) {
          logs( "\r\nCONNECT PASS" );
          online=1;
          break;
         }
        if( !anz ) goto neustart; /* nach time out neu beginnen */
       }
      if( !online ) continue;
      ;
      anz=scc_b_einlesen( ( byte * )&moda, sizeof( moda ), 0xff ); /* 1,25 sec */
      if( !anz ) continue;
      logs( "\r\nMODA PASS" );
      PrepareAnswerData();
      scc_b_ausgabe( ( byte * )&moda, sizeof( moda ) );
      for( z=0; z<3; z++ ) { /* breche verbindung ab */
        x_sleep( 1000/5 );
        scc_b_ausgabe( "+++", 3 );   
        x_sleep( 1000/5 );
        scc_b_ausgabe( "ATH\r", 4 );
        logs( "\r\nHANG UP" );
       } 
      goto neustart;
     } 
   } 
  #endif
 }
