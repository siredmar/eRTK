#include "types.h"
#include "ASCII.h"
#include "funcs.h"
#include "seriell.h"
#include "t3964.h"


static byte sbcc, rbcc;

static void send( byte b ) {
  asci1_ausgabe( &b, 1 );
  sbcc ^= b;
 }


static word rec( byte time ) {
  static word zei, lesab;
  ;
  lesab=asci1_einlesen( ( byte * )&zei, 1, time );
  if( lesab>=0xff00 ) return 0xffff; /* timeout */
  if( lesab==0x0001 ) return zei&0x00ff;
  return lesab&0x00ff; /* abschlusszeichen, minx, maxx falsch angegeben */
 }


static byte sendeblock( byte * puffer, unsigned anz, byte abschluss ) { /* ein Block im 3964 -Protokoll */
  word n, count;
  ;
  count = 0;
  while( count < 3 ) {
    send( STX );
    if( rec( TQ ) != DLE ) /* kein DLE */
      ++count;
    else break;
    if( count == 3 ) return 11;
   }
  sbcc = 0;
  for( n=0; n<anz; n++ ) {
    if( puffer[n] == DLE ) /* DLE doubling */
      send( DLE );
    send( puffer[n] );
   }
  send( DLE );
  send( abschluss );
  send( sbcc );
  if( rec( TQ ) == DLE )
    return 0;
  return 3;
 }



byte ausgab_3964( byte * puffer, unsigned anz ) { /* Senderoutine 3964 */
  byte *zeiger, status, abschluss;
  word gesendet, help;

  asci1_clear();
  gesendet = 0;
  while( gesendet < anz ) { /* bis alle Zeichen gesendet oder Protokollfehler */
    help = ( anz-gesendet );
    if( help>MAX_BLOCK_LEN )
      help=MAX_BLOCK_LEN;
    zeiger = &puffer[gesendet];
    if( ( gesendet+help ) >= anz ) abschluss=ETX;
    else abschluss=ETB;
    status=sendeblock( zeiger, help, abschluss );
    if( !status ) {
      gesendet += help;
      zeiger = &puffer[gesendet];
     }
    else { /* Fehler in Sendeblock */
      return status;
     }
   }
  return 0;
 }





byte einles_3964( byte * puffer, unsigned anz, byte U_zeit ) { /* Empfangsroutine 3964 */
  word empfangszaehler, hilfszaehler;
  word zei, help;

  asci1_clear();
  memset( puffer, 0, anz ); /* Puffer loeschen */
  empfangszaehler = 0;
  hilfszaehler = 0;

  next_block:
  zei=rec( U_zeit );
  if( zei != STX ) { /* kein Sendaufruf in Ueberwachungszeit */
    memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
    if( zei<0x100 ) { /* zeichen aber nicht STX */
      send( NAK ); /* protokollfehler */
      return 3; /* ev. falscher code fuer protokollfehler !!!! */
     }
    return 11;
   }
  send( DLE ); /* quittieren */
  rbcc = 0;
  hilfszaehler = empfangszaehler;
  while( 1 ) { /* Empfangen */
    zei = rec( TZ );
    if( zei != DLE ) {
      if( zei != 0xffff ) { /* Zeichen oder timeout */
        if( hilfszaehler<anz ) {
          puffer[hilfszaehler++] = zei;
         }
        else { /* zu viel Daten */
          memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
          return 5;
         }
       }
      else { /* timeout */
        memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
        return 1;
       }
     }
    else { /* ein DLE empfangen */
      zei = rec( TZ );
      switch( zei ) {
        case DLE: { /* DLE doubling */
          puffer[hilfszaehler++] = zei;
          break;
         }
        case ETX: { /* Endeblock */
          help = rbcc;
          zei = rec( TZ );
          if( zei == help ) { /* Pruefsumme ok */
            send( DLE );
            empfangszaehler = hilfszaehler;
            return 0;
           }
          else { /* Pruefsumme falsch */
            send( NAK );
            memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
            return 6;
           }
         }
        case ETB: { /* weiterer Block folgt */
          help = rbcc;
          zei = rec( TZ );
          if( zei == help ) {
            send( DLE );
           }
          else { /* Protokollfehler */
            send( NAK );
            memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
            return 6;
           }
          empfangszaehler = hilfszaehler;
          goto next_block; /* Folgeblock einlesen */
         }
        default: { /* Protokollfehler */
          send( NAK );
          memset( &puffer[empfangszaehler], 0, anz-empfangszaehler );
          return 6;
         }
       } /* switch zei */
     } /* else DLE */
   } /* while 1 */
 } /* empfangen */

