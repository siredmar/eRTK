/**************************************************************************
 *     PROJEKT:          RTK180 V2.0                                      *
 **************************************************************************
 *                                                                        *
 *     FILENAME:  Sema.c                                                  *
 *                                                                        *
 *     VARIANTE:                                                          *
 *                                                                        *
 *     VERSION:   2.0                                                     *
 *                                                                        *
 *     AUFGABE:   Realisierung von Semaphoren fuer RTK180                 *
 *                                                                        *
 *     SPRACHE:   C u. Z180 Assembler                                     *
 *                                                                        *
 *     DATUM:     25. Mai 1992                                            *
 *                                                                        *
 *     PROGRAMMIERER:         /te                                         *
 *                                                                        *
 **************************************************************************/
#include "types.h"
#include "logger.h"
#include "rtk180.h"
#include "funcs.h"
#include "sema.h"         


static byte test_and_set( byte * adr, byte wert ) { /* atomare funktion fuer byte zelle */
  #ifndef _MSC_VER
  #asm
    pop ix
    pop hl
    pop de
    di
    ld a,(hl)
    cp 0
    jr nz,_l1
    ld (hl),e
    ld hl,-1;           semaphore besetzt
    jr _l2
_l1:
    ld hl,0;            semaphore ist blockiert
_l2:
    ;ei
    push de
    push hl
    push ix 
    ret
  #endasm
  #endif
  return 0;
 }


static byte sema[ANZSEMA];
/*      0 = alloc
        1 = vm
        2 = dword
        3 = message
        4 = accustar
        5 = meldungslogger
    6.. 9 = frei
   10..19 = mailboxen
*/

static byte sema_tid[ANZSEMA];

static void get_sema( byte semaid, byte wert ) { /* Warten bis Semaphore frei ist und danach besetzen */
  ;
  disable();
  while( !test_and_set( &sema[semaid], wert ) ) { /* sema blockiert */
    /*assert( sema_tid[semaid]==0xff, "get semaphore with more than two tasks -> polling" );*/
    disable();
    if( sema_tid[semaid]==0xff ) { /* noch keine task wartend */
      sema_tid[semaid]=akttask; /* normaler wartevorgang */
      x_wef(); /* kommt im state enabled wieder */
     }
    else x_short_sleep( 2 ); /* alle weiteren tasks pollen semaphor */
   }
  disable();
  if( sema_tid[semaid]==akttask ) sema_tid[semaid]=0xff;
  enable();
 }

static void free_sema( byte semaid ) {
  byte tid;
  ;
  disable();
  sema[semaid]=0;
  if( sema_tid[semaid]!=0xff ) { /* andere task wartet */
    tid=sema_tid[semaid];
    sema_tid[semaid]=0xff;
    x_start_task( tid );
   }
  enable();
 }

void sema_init( void ) {
  static byte n;
  ;
  for( n=0; n<ANZSEMA; n++ ) {
    sema[n]=0;
    sema_tid[n]=0xff; /* ff=keine task wartend */
   }
 }

/* semaphor fuer alloc funktionen */
void ReserveAlloc( void ) {
  get_sema( 0, ( byte )( akttask+0x80 ) );
 }

void ReleaseAlloc( void ) {
  free_sema( 0 );
 }

/* semaphor virtueller speicher */
void ReserveVM( void ) {
  get_sema( 1, ( byte )( akttask+0x80 ) );
 }

void ReleaseVM( void ) {
  free_sema( 1 );
 }

/* semaphor dword arithmetik */
void ReserveDword( void ) {
  get_sema( 2, ( byte )( akttask+0x80 ) );
 }

void ReleaseDword( void ) {
  free_sema( 2 );
 }

/* semaphor message */
void ReserveMessage( void ) {
  get_sema( 3, ( byte )( akttask+0x80 ) );
 }

void ReleaseMessage( void ) {
  free_sema( 3 );
 }

/* sharing ressource "accustar" fuer beide kanaele */
/*void get_accustar( void ) {
  get_sema( 4, ( byte )( akttask+0x80 ) );
 }
*/
/*void free_accustar( void ) {
  free_sema( 4 );
 }
*/
/* logger message */
void ReserveLogger( void ) {
  get_sema( 5, ( byte )( akttask+0x80 ) );
 }

void ReleaseLogger( void ) {
  free_sema( 5 );
 }

void ReserveMB( byte mb_id ) {
  get_sema( ( byte )( 10+mb_id ), ( byte )( akttask+0x80 ) );
 }

void ReleaseMB( byte mb_id ) {
  free_sema( ( byte )( 10+mb_id ) );
 }
