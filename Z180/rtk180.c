/**************************************************************************
 *     PROJEKT:         RTK180 V2.0                                   *
 **************************************************************************
 *                                                                        *
 *     FILENAME:  TASKCTRL.C                                              *
 *                                                                        *
 *     VARIANTE:  waage                                                   *
 *                                                                        *
 *     VERSION:   2.1                                                     *
 *                                                                        *
 *     AUFGABE:   sheduler mit dynamischen prioritaeten                   *
 *                                                                        *
 *     SPRACHE:   C mit Assemblereinschueben                              *
 *                                                                        *
 *     DATUM:     18-20. Mai  1992                                        *
 *                                                                        *
 *     PROGRAMMIERER:  /te                                                *
 *                     2.1 mit stack test                                 *
 **************************************************************************/

/************************** SYSTEM -CALL's ********************************

    - x_init_scheduler()    Routine zur Systeminitialisierung

    - x_start()             Aufruf des Betriebssystems zum starten
                            der hoechstprioren bereiten Anwendertask. 
                            die reihenfolge der deklaration ist unwichtig, 
                            die pri und der state sind entscheidend !

    - x_scheduler()         muss per Timerinterrupt zyklisch
                            aufgerufen werden.

    - x_sleep( word time )  argument ist anzahl an system timer ticks
                            waehrend die aufrufende task suspendiert bleibt.

    - x_pause()             compatibility function == x_sleep( 1 )

    - x_wef()               task geht in zustand waiting bis sie wieder per
                            x_start_task oder ablauf des systimers ready wird.
                            falls zum aufrufzeitpunkt der systimer auf null 
                            steht, gibt es einen dead lock !

    - x_wef_timer()         task geht in zustand waiting bis sie wieder per
                            ablauf des systimers ready wird. falls der timer
                            schon auf null steht, bleibt sie im zustand ready.

    - x_start_task( task )  angegebene task geht in zustand ready, back call
                            der aufrufenden task erfolgt umgehend.

    - x_suspend_task( task )angegebene task wird suspendiert, back call
                            der aufgerufenen task erfolgt umgehend.

    - x_cpri( task, pri )   die pri der angegebenen task wird auf den
                            wert pri eingestellt.

    - x_sefet( task, time ) der status der angegebenen task wird nach
                            ablauf der zeit auf ready gesetzt. es werden die
                            systimer benutzt. die aufrufende task laeuft 
                            normal weiter. 
                            achtung: verschachtelung mit einem seriellen
                            einleseauftrag ist nicht moeglich, da dieser
                            ebenfalls die systimer benutzt !

    - x_wef()               die task wartet bis sie wieder durch ablauf der
                            systimer oder durch andere task system aufrufe
                            in den state ready geht.

 **************************************************************************/
#include "types.h"  
#include "..\telegram.h"
#include "io.h"
#include "funcs.h"
#include "main.h"
#include "task3964.h"
#include "..\\chipcode.h"
#include "reader.h"
#include "memtest.h"  
#include "iomod.h"
#include "prt.h"
#include "dcf77.h" 
#include "stg.h"
#include "funk.h"
#include "rtk180.h"
                    
#define RTKOK 0x1234 /* merker das das rtk initialisiert ist */
static unsigned initialized;

static void Tidle( void ) { /* idle task ist immer ready mit prio 0 */
  #ifndef _MSC_VER
  #asm
  ei
  slp; energiespar modus sleep statt halt im idle zustand
  jr Tidle
  #endasm         
  #endif
 }


/************************ SYSTEM KONFIGURATION *****************************/


/* achtung !!!
   im modul data die task stacks fuer taskanzahl anlegen
   bei ungleichheit gibt es fehlermeldung: stack overflow
*/


#define VANZTASK 11                /* Vorgabewert Zahl der Tasks */

s_tcd tcd[VANZTASK];

#define RELEASE
#ifdef RELEASE
static const struct {
  void * task;
  byte pri;
  byte state;
  word param0, param1; /* task parameter auf stack */
  /*               tid   adresse     pri  state    p0 p1  */
 } rom_tcb[VANZTASK]={
                  /*0*/  Tidle,       0,  READY,   0, 0,  /* idle ist immer ready */
                  /*1*/  &Tmain,     250, READY,   0, 0,  /* launch steuerungs task */
                  /*2*/  &T3964,      50, WAITING, 0, 0,  /* 3964 task */
                  /*3*/  &Tmemtest,    1, WAITING, 0, 0,  /* speicher test */
                  /*4*/  &Treader,   150, WAITING, 0, 0,  /* chip reader */
                  /*5*/  &Tdisplay,   60, WAITING, 0, 0,  /* display */ 
                  /*6*/  &Tdcf77,     40, WAITING, 0, 0,  /* dcf77 */
                  /*7*/  &Tio,       255, WAITING, 0, 0,  /* io und zeitzellen */
                  /*8*/  &Tstg,      240, WAITING, 0, 0,  /* steuerung 1 */
                  /*9*/  &Tstg,      240, WAITING, 1, 1,  /* steuerung 2 */
                 /*10*/  &Tfunk,      30, WAITING, 0, 0 };/* funk verbindung */
#endif

#ifdef SAMEPRI_FOR_ALL_TASKS
static const struct {
  void * task;
  byte pri;
  byte state;
  word param0, param1; /* task parameter auf stack */
  /*               tid   adresse     pri  state    p0 p1  */
 } rom_tcb[VANZTASK]={
                  /*0*/  &Tidle,     255, READY,   0, 0,  /* idle ist immer ready */
                  /*1*/  &Tmain,     255, READY,   0, 0,  /* launch steuerungs task */
                  /*2*/  &T3964,     255, WAITING, 0, 0,  /* 3964 task */
                  /*3*/  &Tmemtest,  255, WAITING, 0, 0,  /* speicher test */
                  /*4*/  &Treader,   255, WAITING, 0, 0,  /* chip reader */
                  /*5*/  &Tdisplay,  255, WAITING, 0, 0,  /* display */ 
                  /*6*/  &Tdcf77,    255, WAITING, 0, 0,  /* dcf77 */
                  /*7*/  &Tio,       255, WAITING, 0, 0,  /* io und zeitzellen */
                  /*8*/  &Tstg,      255, WAITING, 0, 0,  /* steuerung 1 */
                  /*9*/  &Tstg,      255, WAITING, 1, 1,  /* steuerung 2 */
                 /*10*/  &Tfunk,     255, WAITING, 0, 0 };/* funk verbindung */
#endif

byte TASK_IDLE=0; /* immer ready */
byte TASK_MAIN=1; /* init task */
byte TASK_3964=2;
byte TASK_MEMTEST=3;
byte TASK_READER=4;
byte TASK_DISPLAY=5;
byte TASK_DCF77=6;
byte TASK_IO=7;
byte TASK_STG0=8;
byte TASK_STG1=9;                    
byte TASK_FUNK=10; 

s_tcd * const tcd_adr[]={	/* pointer auf die tcds indiziert ueber taskindex ! */
  &tcd[0], &tcd[1], &tcd[2], &tcd[3],
  &tcd[4], &tcd[5], &tcd[6], &tcd[7],
  &tcd[8], &tcd[9], &tcd[10] };

/*********************** ende konfiguration *******************************/


/* ***************** prinzip taskverwaltung: **************************
   der pointer taskliste zeigt auf die einfach verkettete liste der tcd's.
   in dieser liste sind die tcd's in reihenfolge absteigender prioritaet
   angeordnet. anhand des task state kann der scheduler erkennen ob die
   task ablaufbereit ist. die task id ist der index in der tabelle der
   tcd's unabhaengig von der prio verkettung.
   Die CPU -Register werden vor dem Taskwechsel auf dem jeweiligen
   Taskstack abgelegt.


                   +--------------+
             254___I    param1    I 2. parameter fuer task
                   +--------------+
             252___I    param0    I 1. parameter fuer task
                   +--------------+
             250___I   dummy ret  I == adresse 0 bei ruecksprung aus task
                   +--------------+
             248___I taskx_start  I
                   +--------------+
             246___I      IX      I
                   +--------------+
             244___I      IY      I
                   +--------------+
             242___I      HL      I
                   +--------------+
             240___I      DE      I
                   +--------------+
             238___I      BC      I
                   +--------------+
             236___I      AF      I
                   +--------------+
             234___I      ??      I
                  ==================
                   I      ??      I
                   +--------------+
Stack_base +   0___I      ??      I
                   +--------------+

  Der verbleibende Stackbereich fuer die Anwendertask ist 236 Bytes lang.
*/

extern byte stack[VANZTASK][256];  /* Jede Task hat Stack a' 256 Byte */

extern byte stacktop; 		/* ende des stack segmentes */

byte * stackptr[VANZTASK];  	/* Ablage fuer Stackpointer der Tasks */

byte akttask;             	/* Nummer der aktuellen Task */

byte anztask=VANZTASK;      /* const fuer Zahl der Tasks */

static byte rr_count;  /* wenn mehrere tasks mit gleicher prio vorhanden sind,
                           passiert folgendes:
                           der sheduler prueft immer, ob
                           tcd[akttask+1].pri==tcd[akttask].pri ist.
                           es wird time sharing im systemtakt fuer alle
                           tasks gleicher prio im state ready durchgefuehrt.
                        */


s_tcd * taskliste; 			/* zeiger auf tcd schlange */
byte systimer[VANZTASK]; 	/* zeitzellen fuer task wartezeiten */
                            

#ifdef SUSPEND                            
x_suspend_task( taskid ) byte taskid; {
  if( taskid>=anztask ) return -1;
  tcd_adr[taskid]->state=WAITING;
  return 0;
 }
#endif

byte x_start_task( byte taskid ) {
  if( taskid>=anztask ) return 0xff;
  tcd_adr[taskid]->state=READY;
  return 0;
 }

void x_short_sleep( byte time ) { /* task fuer intervall suspendieren */
  ;
  disable();
  if( !time ) time=1;
  systimer[akttask]=time;
  tcd_adr[akttask]->state=WAITING;
  x_scheduler();
 }

#define PAUSE
#ifdef PAUSE
void x_pause( void ) { /* Weiterschalten zur naechsten Task per Programmbefehl */
  if( initialized!=RTKOK ) return;
  x_short_sleep( 1 );
 }
#endif

/* suspendierung einer tasks fuer die zahl von longtime systemtakten */
void x_sleep( word longtime ) { /* 16 bit zeiten realisieren */
  if( initialized!=RTKOK ) return;
  while( longtime ) {
    if( longtime>255 ) {
      x_short_sleep( 255 );
      longtime-=255;
     }
    else {
      if( longtime ) x_short_sleep( ( byte )longtime );
      else x_short_sleep( 1 );
      longtime=0;
     }
   }
  return;
 }

/* nach ablauf von time system takten wird zelle gesetzt */
void x_sefet( byte task, byte time ) {                  
  ;
  disable();
  systimer[task]=time;
  enable();
  return; /* task arbeitet normal weiter */
 }

/* treiber im disabled zustand */
void x_sefet_disabled( byte task, byte time ) {
  ;
  disable(); /* zur sicherheit */
  systimer[task]=time;
  return; /* task arbeitet normal weiter */
 }

/* der scheduler setzt beim nullwerden des systimers die task auf ready.
   falls eine ueberschneidung in der art vorkommt, dass das dekrement zeitlich
   vor der ausfuehrung des wef calls erfolgt, bleibt die task endlos
   im zustand waiting !!!
   lediglich ein x_start_task call einer anderen task bringt sie wieder 
   in den zustand ready.
*/
void x_wef( void ) {
  disable();
  tcd_adr[akttask]->state=WAITING;
  x_scheduler();
 }

/* warten auf die zeit zelle */
/* der scheduler setzt beim nullwerden des systimers die task auf ready.
   falls der timer schon auf null steht, bleibt die task ready !
*/
void x_wef_timer( void ) {
  disable();
  if( systimer[akttask] ) { /* timer laeuft noch */
    tcd_adr[akttask]->state=WAITING;
    x_scheduler();
   }
  else enable();
 }
                                  
#undef CPRI_USED
#ifdef CPRI_USED                                  
x_cpri( taskid, newpri ) byte taskid, newpri; { /* change task prio */
  static s_tcd * p, * pnext, * plast;
  static byte task;
  ;
  if( taskid>=anztask ) return -1; /* fehler */
  ;
  disable();
  /* 1. tcd aus kette entfernen */
  p=tcd_adr[taskid]; /*&tcd[taskid];*/
  pnext=p->next; /*tcd[taskid].next;*/
  p->next=NULL; /*tcd[taskid].next=NULL;*/
  if( taskliste==p/*tcd_adr[taskid]*//*&tcd[taskid]*/ ) taskliste=pnext;
  else for( task=0; task<anztask; task++ ) {
    plast=tcd_adr[task];
    if( plast->next/*tcd[task].next*/==p ) {
      plast->next/*tcd[task].next*/=pnext;
      break;
     }
   }
  ; 
  /* 2. tcd in kette an durch pri gegebener stelle einfuegen */
  p->pri/*tcd[taskid].pri*/=newpri;
  plast=&taskliste;
  p=taskliste; /* 1.tcd */
  pnext=p->next; /* 2. tcd */
  while( pnext!=NULL && p->pri>newpri ) { /* tcd.pnext<>0, tcd.pri>newpri */
    plast=p;
    p=pnext;
    pnext=p->next;
   }             
  ; 
  p=tcd_adr[taskid];
  p->next/*tcd[taskid].next*/=plast->next;
  plast->next=tcd_adr[taskid];/*&tcd[taskid];*/
  enable();
  return 0; /* ok */
 }
#endif       
       
       
void x_init_scheduler( void ) { /* Initialisierung des Echtzeitsystems */
  static byte n, pri, index;
  static byte * p;
  static byte task;
  static word * nextp;
  ;      
  /* pruefen ob stack bereich ausreicht */
  #ifndef _MSC_VER
  #asm                                    
  ld hl, stacktop
  ld de, stack
  and a
  sbc hl, de;		hl=gesamtgroesse aller stacks in bytes, h=anzahl pages
  ld a, ( anztask )
  sub h
  jp nz, stck_overflow    
  ;
  #endasm       
  #endif
  ;
  /* test ob tcd_adr alle tcd adressen enthaelt */
  if( sizeof( tcd_adr )!=( VANZTASK*sizeof( s_tcd * ) ) ) stck_overflow();
  ;
  rr_count=0;
  for( n=0; n<VANZTASK; n++ ) {
    tcd_adr[n]->next=NULL; /* verkettung der tcd's in unsortierten grundzustand */
    tcd_adr[n]->id=n;
    tcd_adr[n]->pri=rom_tcb[n].pri;
    tcd_adr[n]->state=rom_tcb[n].state;
    systimer[n]=0;
   }
  ;
  /* sortieren der task verkettung nach absteigender prioritaet */
  taskliste=NULL;
  for( task=0; task<VANZTASK; task++ ) {
    pri=0;
    index=0;
    for( n=0; n<VANZTASK; n++ ) {
      if( ( tcd_adr[n]->pri>=pri )
        &&( tcd_adr[n]->next==NULL ) ) { /* pri>pri und noch nicht in liste */
        pri=tcd_adr[n]->pri;
        index=n;
       }
     }
    if( task==0 ) taskliste=tcd_adr[index];
    else *nextp=( word )tcd_adr[index];
    tcd_adr[index]->next=( word * )0xffff; /* merker, dass tcd in liste steht */
    nextp=( word * )&tcd_adr[index]->next; /* eintrag verkettung naechster durchlauf */
    if( task==VANZTASK-1 ) *nextp=0;
   }
  ;
  for( n=0; n<VANZTASK; n++ ) { /* SP der Tasks auf jeweiliges Stackende setzen */
    stackptr[n] = &stack[n][236];
   }
  ;
  /* startadressen und parameter auf den stack */
  for( task=0; task<VANZTASK; task++ ) {
    nextp=( word * )&stack[task][248];
    nextp[0]=( word )rom_tcb[task].task; 	/* [248..249]=task adresse */
    nextp[1]=0; 					/* [250..251]=restart adresse bei return von task */
    nextp[2]=rom_tcb[task].param0;  /* [252..253]=parameter 0 */
    nextp[3]=rom_tcb[task].param1;  /* [254..255]=parameter 1 */
   }   
  initialized=RTKOK; 
 }


#ifndef _MSC_VER
#asm

POPALL macro
  pop af
  pop bc
  pop de
  pop hl
  pop iy
  pop ix
 endm
  
PUSHALL macro  
  push ix
  push iy
  push hl
  push de
  push bc
  push af           ;Arbeitsregister auf den Task Stack
 endm
    
#endasm
#endif

void x_start( void ) { /* start der hoechstprioren ready task */
  #ifndef _MSC_VER
  #asm
  ld ix, ( taskliste );         ix -> 1. tcd
s_next_tcd:;                    suche tcd im state ready
  ld a, READY
  cp ( ix+OFFSET_STATE )
  jr z, tcd_start
  ;
  ld l, ( ix )
  ld h, ( ix+1 )
  push hl
  pop ix
  jr s_next_tcd
  ;
tcd_start:;                     ix -> tcd in status ready
  ld a, ( ix+OFFSET_PRI )
  ld ( lastpri ), a;            pri merken fuer round robin programm
  ld a, ( ix+OFFSET_ID );       a=task id
  ld ( akttask ), a;            akttask setzen
  add a, a
  ld d, 0
  ld e, a;                      ( ix+de ) -> task stack
  ld ix, stackptr;              tabelle der sp nach ix
  add ix, de;                   ix -> hoechst prio task stack
  ld l, ( ix )
  ld h, ( ix+1 );               hl -> task stack
  ld sp,hl
  POPALL
  ret                  ; pop pc -> task starten ...
  ;pop param0
  ;pop param1
  #endasm
  #endif
 }


void x_sysclock( void ) { /* call mit system timer interrupt im task kontext */
  #ifndef _MSC_VER
  #asm   
  public prt1;	alias name  
prt1:  
  PUSHALL;		Arbeitsregister auf den Task Stack
  ;timer interrupt acknowledge durch input auf timer ports 
  in0 a, ( tcr )
  in0 a, ( tmdr1l )
  in0 a, ( tmdr1h )
  ;
  ;************** zeiten verwaltung ****************
  ld hl, systimer;      hl -> timer zellen
  ld a, ( anztask )
  ld b, a;              anztask in b
  ld c, 0;              task id in c
  ld ix, tcd;           ix -> tcd array
  ld de, SIZE_TCD;      de = array laenge
  ;
the_next:
  xor a
  cp ( hl )
  jr z, next_cell
  ;
  dec ( hl )
  jr nz, next_cell
  ;
set_task_ready:;         dekrement 1 -> 0, state auf ready setzen
  ld a, READY
  ld ( ix+OFFSET_STATE ), a;task ready setzen nach ablauf des intervalls
  ;
next_cell:
  inc c;                    ++ task id
  inc hl;                   hl -> timer[c]
  add ix, de;               ix -> next tcd
  djnz the_next
  ;
  jr ShRegsOnStack
  #endasm         
  #endif
 }
 
static byte lastpri; /* merker fuer round robin programm */

void x_scheduler( void ) { /* hardware taskswitch per Timer Interrupt */
  #ifndef _MSC_VER
  #asm
  di            
  PUSHALL;			Arbeitsregister auf den alten Stack
  ;
ShRegsOnStack:;		x_sysclock() hat register schon auf dem stack abgelegt
  ;
  ;*************** alten stackpointer ablegen *********************
sp_store:;              einsprung sw scheduler
  ld ix, stackptr
  ld d, 0
  ld a, ( akttask )
  sla a
  ld e, a
  add ix, de;		ix->platz fuer stackpointer
  ;
  ld hl, 0
  add hl, sp;		sp nach hl laden
  ;alten Stackpointer ablegen
  ld ( ix+0 ), l
  ld ( ix+1 ), h
  ;
  extern system_tos
  ld sp, system_tos;        systemstack ab hier aktiv
  ;
;************ naechste bereite task aus liste suchen *****************
search_next_task:
  ld ix, ( taskliste ); ix -> 1. task
  ld a, READY
  ;
search_highest_pri:
  cp ( ix+OFFSET_STATE )
  jr z, highest_pri_task
  ;
  ld l, ( ix )
  ld h, ( ix+1 )
  push hl
  pop ix
  jr search_highest_pri
  ;
  ;round robin sheduler realisieren
highest_pri_task:;                  ix -> tcd der hoechstprioren ready task
  ld c, ( ix+OFFSET_PRI );      c=pri der ersten hoechstprioren ready task
  ;bei prioritaetenwechsel round robin programm uebergehen
  ld a, ( lastpri )
  cp c
  jr z, do_round_robin
  ;
  xor a
  ld ( rr_count ), a;               neustart rr_count
  jr start_task;                    und neue task starten
  ;
do_round_robin:
  ld hl, rr_count
  inc ( hl );                       ++rr_count
  ld b, ( hl );                 nach b laden
  ;test ob tcd[akttask+( ++rr_count )].pri==tcd[akttask].pri
  push ix;                      ix auf stack sichern
tcd_same_pri:
  ld l, ( ix )
  ld h, ( ix+1 );                   pnext laden
  ld a, h
  or l
  jr z, tcd_pnext_is_null;      ende der tcd kette, ix=& ready tcd auf stack
  ;
  push hl
  pop ix;                           ix=pnext
  dec b
  jr nz, tcd_same_pri
  ;
  ;rr1:;ix -> tcd[akttask+rr_count]
  ld a, c;                      a=pri der ersten hi pri tcd
  cp ( ix+OFFSET_PRI )
  jr z, test_tcd_ready
  ;
tcd_pnext_is_null:
  ld a, 0
  ld ( rr_count ), a;               rr_count ruecksetzen
  pop ix;                           & ready tcd vom stack
  jr start_task
  ;
test_tcd_ready:;                    test ob die weitere task ready ist
  ld a, READY
  cp ( ix+OFFSET_STATE )
  jr nz, tcd_same_pri;                      wenn nicht
  ;
  ;weitere task ist ready
  pop bc;                           dummy fuer ix vom stack holen
  ;
start_task:;                        ix -> tcd der zu startenden task
; ld a, RUNNING
; ld ( ix+OFFSET_STATE ), a;    state is running
  ld a, ( ix+OFFSET_PRI )
  ld ( lastpri ), a;            merker fuer round robin counter
  ld a, ( ix+OFFSET_ID )
  ld ( akttask ), a;            neue Tasknummer ablegen
  ;neuen stack pointer nach sp laden
  ld ix, stackptr           ;Startadresse der Stackpointer laden
  ld d, 0
  sla a             ; x 2, da Wortwerte
  ld e, a
  add ix, de         ;ix -> neuen Stackpointer
  ld l, ( ix+0 )
  ld h, ( ix+1 )
  ld sp, hl
  ;
  ;stack overflow testen, lower byte sp <= 10h ist indikator wegen page alignment stack segmente
  ;ld hl, 0
  ;add hl, sp
  ld a, 10h;   mehr als 10h bytes stack sind immer noetig
  sub l
  jr nc, stck_overflow
  ;
  ;test ob richtiges stack segment benutzt wird
  ld a, ( akttask )
  add a, high stack
  sub h
  jr nz, stck_overflow
  ;                 
  POPALL
ExitScheduler:
  ei
  ret
  #endasm       
  #endif
 }


/* 
   kommt auch in hochlaufphase wenn sizeof( stack ) und vanztasks nicht konsistent
   sowie wenn tcd_adr nicht korrekt initialisiert ist
*/   
char stck_text[]="\
NUC: FATAL ERROR    \
STACK/TCD OVERFLOW  \
SYSTEM STOPPED      \
REBOOT SYSTEM       "; 


/*static*/void stck_overflow( void ) {
  #ifndef _MSC_VER
  #asm
  di  
  extern init_d
  call init_d
  ld hl, stck_text
sout:
  ld a, ( hl )
  cp 0
  jp z, stop
  ld c, a
  ld b, 0
  push hl
  push bc      
  extern putchar
  call putchar
  pop bc
  pop hl
  inc hl
  jr sout
stop:
  halt
  #endasm         
  #endif
 }

