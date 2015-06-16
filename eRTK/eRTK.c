/*
 * eRTK.cpp
 *
 * Created: 13.04.2015 07:52:46
 *  Author: er
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "eRTK.h"
#include "adc.h"

#ifdef ERTKDEBUG
uint8_t stack[VANZTASK+1][ERTK_STACKSIZE]  __attribute__ ((aligned (256)));  /* Jede Task hat Stack a' STACKSIZE Byte */
#else
uint8_t stack[VANZTASK+1][ERTK_STACKSIZE];  /* Jede Task hat Stack a' STACKSIZE Byte */
#endif

void * stackptr[VANZTASK+1]; /* Ablage fuer Stackpointer der Tasks */

volatile uint8_t akttask;    /* Nummer der aktuellen Task, 0=idle */

/* struktur task control block */
typedef struct s_tcd {    /* queue mit ordnungskriterium task prioritaet 255..0 */
  struct s_tcd * pnext;   /* 1. element immer verkettungszeiger !!! */
  struct s_tcd * pbefore; /* vorgaenger in liste */
  uint8_t tid;            /* index aus der reihenfolge der statischen deklaration */
  uint8_t prio;           /* 0..255 */
  uint8_t timer;          /* timeout counter mit system tick aufloesung */
#ifdef ERTK_DEBUG
  uint16_t param0;
  void * param1;
#endif
 } s_tcd;
s_tcd tcd[VANZTASK+1];    /* indizierung ueber task index */

s_tcd * pTaskRdy;         /* einfache verkettung aller ready tasks ueber tcd.next in reihenfolge der prioritaet */

/*
  die perfcounter: 
  stellen in einem schieberegister die letzten 256 messergebnisse dar.
  bei system im leerlauf stehen die zaehler auf 1000-overhead.
  eine schleife dauert 16 zyklen, alle 1ms wird abgefragt und da sind 16000 zyklen ausgefuehrt worden.
  je mehr systemlast um so kleiner der zaehlerstand.
  1000 -> leerlauf
   900 -> 100*16=1600 zyklen wurden anderweitig verbraucht (in diesem 1ms intervall)
  ! z.b. lcd_clear_area() braucht mehr als 10ms !
*/
volatile uint8_t  eRTK_up;               //wird gesetzt wenn das system gestartet ist
volatile uint16_t eRTK_perfcount;        //aktueller counter
volatile uint16_t eRTK_perfcounter[256]; //counter array der letzen 256ms
volatile uint8_t  eRTK_iperf;            //index im array 0..255
volatile uint16_t eRTK_ticks;            //wie spaet ist es nach dem urknall in ms
volatile uint8_t  eRTK_cnt_overload;     //zaehlt die aufeinanderfolgenden overload phasen

void  __attribute__ ((optimize("O2"))) eRTK_Idle( void ) {
#ifdef ERTKDEBUG
  while( 1 ) {           //14+2=16 cycles pro loop -> 16MHz -> 1000 inc/ms
    cli();               //cli=1clock
    ++eRTK_perfcount;    //lds,lds,adiw,sts,sts=5x2clocks
    sei();               //sei=1clock
    oIDLEfast( 1 );      //2 cycles fuer 2xnop oder ein output bit setzen
   }                     //rjmp=2clocks
#else
  while( 1 ) {
    set_sleep_mode( SLEEP_MODE_IDLE );
    sleep_enable();
    sei();
    oIDLE( 1 );
    sleep_cpu();
    sleep_disable();
   }
#endif
 }


__attribute__ ((noinline)) void deadbeef( tsys reason ) {
  while( 1 );
 }


void * pp_stack; //speicher f�r stackpointer w�hrend push/pop
//push pc, push r0, r0<-sreg, push r0..r31
#define push() { \
  asm volatile ( \
  "push	r0\n"/*push r0*/ \
  "in	r0, __SREG__\n" \
  "cli\n" /*push sreg, push r1..r31 */\ 
  "push	 r0\npush  r1\npush	 r2\npush  r3\npush	 r4\npush  r5\npush	 r6\npush  r7\npush	 r8\npush  r9\npush	r10\npush	r11\npush	r12\npush	r13\npush	r14\npush	r15\npush	r16\npush	r17\npush	r18\npush	r19\npush	r20\npush	r21\npush	r22\npush	r23\npush	r24\npush	r25\npush	r26\npush	r27\npush	r28\npush	r29\npush	r30\npush	r31\n" \
  "in		r0, __SP_L__\n" \
  "sts  pp_stack, r0\n" \
  "in		r0, __SP_H__\n" \
  "sts	pp_stack+1, r0\n" \
  ); \
 }

//pop r31 .. r0, sreg<-r0, pop r0
#define pop() { \
  asm volatile ( \
  "lds r0, pp_stack \n" \
  "out __SP_L__, r0 \n" \
  "lds r0, pp_stack+1\n" \
  "out __SP_H__, r0\n" \
  /*pop r31..sreg*/ \
	"pop	r31\npop	r30\npop	r29\npop	r28\npop	r27\npop	r26\npop	r25\npop	r24\npop	r23\npop	r22\npop	r21\npop	r20\npop	r19\npop	r18\npop	r17\npop	r16\npop	r15\npop	r14\npop	r13\npop	r12\npop	r11\npop	r10\npop	 r9\npop	 r8\npop	 r7\npop	 r6\npop	 r5\npop	 r4\npop	 r3\npop	 r2\npop	 r1\npop	 r0\n" \
	"out	__SREG__, r0\n" \
	"pop	r0\n"/*pop r0*/ \
  "sei\n" \
  ); \
 }

void __attribute__ ((naked)) eRTK_scheduler( void ) { /* start der hoechstprioren ready task, notfalls idle */
  push();
  stackptr[akttask]=pp_stack;
  //
  if( pTaskRdy ) { //da muss natuerlich immer was drinstehen ;)
    //do round robin bei mehreren mit gleicher prio
    s_tcd *p=pTaskRdy;
    while( p->tid != akttask ) {
      p=p->pnext; //finde aktuelle task
      if( !p ) break;
     }
    if( p ) { //task stand noch in der ready liste
      //teste pri des nachfolgers
      if( p->pnext!=NULL ) { //wenn es nachfolger gibt
        if( p->prio == p->pnext->prio ) { //schalte weiter wenn nachfolger prio genauso ist
          akttask=p->pnext->tid;
         }
        else {
          akttask=pTaskRdy->tid;
         }
       }
      else { //sonst nimm den ersten in der liste, der muss per definition die gleiche prio haben da wir bei kleineren prios gar nicht suchen !
        akttask=pTaskRdy->tid;
       }
     }
    else akttask=pTaskRdy->tid; //nimm das erstbeste aus der ready liste ;)
   }
  else deadbeef( SYS_NOTASK );
  //
  pp_stack=stackptr[akttask];
  pop();
  sei();
  asm volatile ( "ret" );
 }


void eRTK_go( void ) { /* start der hoechstprioren ready task, notfalls idle */
  if( pTaskRdy ) akttask=pTaskRdy->tid;
  else deadbeef( SYS_NOTASK );
  //
  eRTK_up=1;
  pp_stack=stackptr[akttask];
  pop();
  asm volatile( "ret" );
 }

/* Prinzip der Ready Liste:
 * pTskRdy ist nie 0 und zeigt zumindest auf den tcd[0]==idle task.
 * wenn eine task bereit wird geschieht dies durch einhaengen in die liste
 * und zwar vor der ersten task mit niedrigerer prio.
 * zunaechst also pTskRdy->tcd[0]->0 wird pTskRdy->tcd[x]->tcd[0]->0 ( falls tcd[x].prio>0 )
 * wenn tcd[y].prio > tcd[x].prio dann pTskRdy->tcd[y]->tcd[x]->tcd[0]->0
 * sonst pTskRdy->tcd[x]->tcd[y]->0
 * usw.
 * Der scheduler sucht immer ob tcd[akttask] noch in der liste steht.
 *   wenn nein, nimmt er den ersten eintrag der liste zum start ( highest prio task always runs )
 *   wenn ja, schaut er nach ob der nachfolger gleiche pri hat. ( round robin )
 *     wenn ja nimmt er diesen ( weiterschaltung in prio ebene )
 *     wenn nein nimmt er den ersten eintrag der liste ( das ist ja immer der mit der hoechsten prio )
 *
 *  Ausgangszustand nur mit idle task:
 *
 *  pTsk-->tcd[0]
 *         pnext=0
 *         pbefore=0
 *
 *  task 2 dazu mit insertat( this=&tcd[0], new=&tcd[2] )
 *
 *         new          this
 *  pTsk-->tcd[2]       tcd[0]
 *         pnext------->pnext=0
 *         pbefore=0 <--pbefore  
 *              
 *  task 1 dazu mit insertat( this=&tcd[0], new=&tcd[1] )
 *
 *                      new          this
 *  pTsk-->tcd[2]       tcd[1]       tcd[0]
 *         pnext------->pnext------->pnext=0
 *         pbefore=0 <--pbefore<-----pbefore
 *                   
 */
static __inline__ void insertat( s_tcd *pthis, s_tcd *newone ) { //newone vor pthis eintragen
  if( !pthis || !newone ) deadbeef( SYS_NULLPTR );
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    if( pthis->pbefore == NULL ) { //der erste tcd
      pTaskRdy=newone;
      newone->pbefore=NULL;
     }
    else { //tcd in der kette
      pthis->pbefore->pnext=newone;
      newone->pbefore=pthis->pbefore;
     }
    newone->pnext=pthis;
    pthis->pbefore=newone;
   }
 }

static __inline__ void removeat( s_tcd *pthis ) { //den node pthis austragen
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    if( pthis==NULL || pthis==&tcd[0] ) { //0 oder idle task
      deadbeef( SYS_NULLPTR );
     }
    if( pthis->pbefore==NULL ) { //es ist der erste block auf den pTskReady zeigt
      if( pthis->pnext==NULL ) { //und er hat keinen nachfolger den man vorziehen kann
        deadbeef( SYS_NULLPTR );
       }
      pTaskRdy=pthis->pnext; //nachfolger einhaengen
      pthis->pnext->pbefore=NULL;
     }
    else { //mittendrin in der kette
      pthis->pbefore->pnext=pthis->pnext;
      pthis->pnext->pbefore=pthis->pbefore;
     } 
    pthis->pnext=NULL; //diesen block aus verkettung nehmen
    pthis->pbefore=NULL;
   }
 }  

uint8_t eRTK_GetTid( void ) { //holen der eigenen task id
  return akttask;
 }

//Verwaltung der ready list mit einfuegen/ausfuegen in der reihenfolge der prio
void eRTK_SetReady( uint8_t tid ) {
  if( !tid ) deadbeef( SYS_NULLPTR ); //idle task ist immer ready
  if( tcd[tid].pnext || tcd[tid].pbefore ) deadbeef( SYS_VERIFY ); //war gar nicht suspendiert
  //
  s_tcd *pthis=pTaskRdy;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    tcd[tid].timer=0; //timer loeschen damit er nicht spaeter nochmal startet
    do { //pthis->hoechstprioren tcd oder idle
      if( tcd[tid].prio > pthis->prio || pthis->pnext==NULL ) { //neuer eintrag hat hoehere prio oder es gibt keinen nachfolger
        insertat( pthis, &tcd[tid] );
        break;
       }
     } while( ( pthis=pthis->pnext ) );
   }
 }

//pTskRdy->tcdx->tcdy->0 es muss immer mindestens ein tcd (idle) in der liste bleiben
void eRTK_SetSuspended( uint8_t tid ) { //tcd[tid] aus der ready list austragen
  if( !tid ) deadbeef( SYS_NULLPTR ); //idle task darf nicht suspendiert werden
  if( !tcd[tid].pbefore && !tcd[tid].pnext ) deadbeef( SYS_VERIFY ); //war nicht in ready list
  s_tcd *pthis=pTaskRdy;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    do {
      if( pthis->tid==tid ) {
        removeat( pthis );
        break;
       }
     } while( ( pthis=pthis->pnext ) );
   }
 }

void eRTK_wefet( uint8_t timeout ) {
  if( timeout ) { //sonst klinkt sich die task in einem wait_until() fuer immer aus
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) { //14+2=16 cycles pro loop -> 16MHz -> 1000 inc/ms
      if( tcd[akttask].timer ) deadbeef( SYS_UNKNOWN );
      tcd[akttask].timer=timeout;
      eRTK_SetSuspended( akttask );
      eRTK_scheduler();
     }
   }
 }

void eRTK_Sleep_ms( uint16_t ms ) {
  while( ms ) {
    if( ms>255 ) { 
      eRTK_wefet( 255 ); 
      ms-=255; 
     }
    else {
      eRTK_wefet( ms );
      ms=0;
     }
   }
 }

//semaphoren
#define ANZSEMA 10
static uint8_t sema[ANZSEMA];

static inline __attribute__((__always_inline__)) uint8_t xch ( uint8_t volatile *p, uint8_t x ) {
  //wenn die cpu xch kennt:
  // __asm volatile ("xch %a1,%0" : "+r"(x) : "z"(p) : "memory");
  register uint8_t tmp;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    tmp=*p;
    *p=x;
   }
  return tmp;
  //return x;
 }

void eRTK_get_sema( uint8_t semaid ) { /* Warten bis Semaphore frei ist und danach besetzen */
  if( semaid>=ANZSEMA ) deadbeef( SYS_UNKNOWN );
  while( xch( &sema[semaid], 1 ) ) { /* >0 = sema blockiert */
    sei();
    eRTK_wefet( 1 );
   }
 }

void eRTK_free_sema( uint8_t semaid ) {
  if( semaid>=ANZSEMA ) deadbeef( SYS_UNKNOWN );
  xch( &sema[semaid], 0 ) ;
 }

void sema_init( void ) {
  static uint8_t n;
  for( n=0; n<ANZSEMA; n++ ) {
    sema[n]=0; /* ff=keine task wartend */
   }
 }

/*
stck[n][STACKSIZE]
- 1-+
- 2 |deadbeef() bei return aus der task zurueck
- 3-+
- 4-+
- 5 |task()
- 6-+
- 7 r0
- 8 SREG
- 9 r1
-10 r2
-11 r3
-12 r4
-13 r5
-14 r6
-15 r7
-16 r8
-17 r9
-18 r10
-19 r11
-20 r12
-21 r13
-22 r14
-23 r15
-24 r16
-25 r17
-26 r18
-27 r19
-28 r20
-29 r21
-30 r22-+
-31 r23-+param1
-32 r24-+
-33 r25-+param0
-34 r26
-35 r27
-36 r28
-37 r29
-38 r30
-39 r31     
*/
void eRTK_init( void ) { /* Initialisierung der Daten des Echtzeitsystems */
  uint8_t n, prio, index;
  uint8_t task;
  //
  for( n=0; n<VANZTASK+1; n++ ) {
    tcd[n].pnext=NULL; /* verkettung der tcd's in unsortiertem grundzustand */
    tcd[n].pbefore=NULL;
    tcd[n].tid=n;
    tcd[n].prio=n ? rom_tcb[n-1].prio : 0; //idle task ist immer da mit pri 0
    tcd[n].timer=0;
    //die parameter merken zum debuggen
#ifdef ERTK_DEBUG
    tcd[n].param0= n ? 0 : rom_tcb[n-1].param0;
    tcd[n].param1= n ? 0 : rom_tcb[n-1].param1;
#endif
   }
  //einsetzen der idle task, die muss immer da sein !
  pTaskRdy=( s_tcd * )&tcd[0];
  pTaskRdy->pnext=NULL;
  pTaskRdy->pbefore=NULL;
  /* einsortieren der tasks mit verkettung nach absteigender prioritaet */
  char hit[VANZTASK+1];
  memset( hit, 0, sizeof hit );
  for( task=1; task<VANZTASK+1; task++ ) {
    prio=0;
    index=0;
    for( n=1; n<VANZTASK+1; n++ ) { //hoechstpriore ready und noch nicht verkettete task finden
      if( tcd[n].prio>=prio && !hit[n] ) { /* prio>prio und noch nicht in liste */
        prio=tcd[n].prio;
        index=n;
       }
     }
    eRTK_SetReady( index ); //pTaskReady -> tcdx -> txdy -> 0
    hit[index]=1;
   }
  //
  for( n=0; n<VANZTASK+1; n++ ) { /* SP der Tasks auf jeweiliges Stackende setzen */
    for( uint16_t f=0; f<ERTK_STACKSIZE/4; f++ ) memcpy( stack[n]+4*f, "\xde\xad\xbe\xef", 4 );
    //memcpy( &stack[n][214], "0123456789abcdef0123456789ABCDEF", 32 );
    /* startadressen und parameter auf den stack */
    stack[n][ERTK_STACKSIZE-9]=0; //r1=0
    //
    union {
      uint32_t ui32;
      uint8_t ui8[4];
      void ( *task )( uint16_t param0, void *param1 );
      void ( *tvoid )( void );
      void ( *tbeef )( tsys );
     } taddr;
    memset( &taddr, 0, sizeof taddr );
    taddr.tbeef=deadbeef;
    stack[n][ERTK_STACKSIZE-1]=taddr.ui8[0];
    stack[n][ERTK_STACKSIZE-2]=taddr.ui8[1];
    stack[n][ERTK_STACKSIZE-3]=taddr.ui8[2];
    if( n ) taddr.task=rom_tcb[n-1].task;
    else taddr.tvoid=eRTK_Idle;
    stack[n][ERTK_STACKSIZE-4]=taddr.ui8[0];
    stack[n][ERTK_STACKSIZE-5]=taddr.ui8[1];
    stack[n][ERTK_STACKSIZE-6]=taddr.ui8[2];
    //
    //                      hi  lo         hi  lo
    //zwei parameter in p0=r25:r24 und p1=r23:r22
    if( n ) {
      stack[n][ERTK_STACKSIZE-32]=rom_tcb[n-1].param0&0xff;
      stack[n][ERTK_STACKSIZE-33]=rom_tcb[n-1].param0>>8;
      stack[n][ERTK_STACKSIZE-30]=( uint16_t )( rom_tcb[n-1].param1 )&0xff;
      stack[n][ERTK_STACKSIZE-31]=( uint16_t )( rom_tcb[n-1].param1 )>>8;
     }
    //
    stackptr[n]=&stack[n][ERTK_STACKSIZE-40];
   }
  sema_init();
 }

/*
  Ein Prozess holt den aktuellen Zaehlerstand in ms mit eRTK_GetTimer().
  Wenn er in 10ms erneut starten will, kann er folgendes tun:
    uint8_t now=eRTK_GetTimer();
    ...do something
    eRTK_WaitUntil( now+10 );
*/
union {
  volatile uint8_t timer8[2];
  volatile uint16_t timer16;
 } eRTK_m_timer;
 
uint8_t eRTK_GetTimer8( void ) { //256ms bis overflow
  return eRTK_m_timer.timer8[0];
 }

uint16_t eRTK_GetTimer16( void ) { //wenn 256ms nicht reichen
  register uint16_t val;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE	) { 	
    val=eRTK_m_timer.timer16;
   }
  return val;
 }

void eRTK_WaitUntil( uint8_t then ) {
  eRTK_wefet( then-eRTK_GetTimer8() );
 }

//setzen der prioritaet fuer eine task
//tid=0 setze die eigene prioritaet oder sonst die einer anderen
//prio=neue prioritaet
//schedule_immediately wenn true dann wird sofort eine neue Prozesstabelle ermittelt und der hoechstpriore prozess gestartet
void eRTK_cpri( uint8_t tid, uint8_t prio, uint8_t schedule_immediately ) {
  if( tid==0 ) tid=akttask;
  if( tid<VANZTASK ) {
    tcd[akttask].prio=prio;
    if( schedule_immediately ) eRTK_scheduler();	
   } else deadbeef( SYS_VERIFY ); //die task id ist unbekannt
 }

__inline__ void __attribute__ ( ( always_inline ) ) eRTK_timertick( void ) { //damit im irq alle register gesichert werden
  oIDLE( 0 );
  if( eRTK_ticks<65535u ) ++eRTK_ticks;
  ++eRTK_m_timer.timer16;
  s_tcd *p=tcd;
  //timer service
  for( uint8_t n=0; n<VANZTASK+1; n++ ) {
    if( p->timer ) {
      if( !--p->timer ) {
        eRTK_SetReady( p->tid );
       }
     }
    ++p;
   }
  if( eRTK_up && eRTK_ticks>eRTK_STARTUP_MS ) { //solange geben wir dem system zum hochlauf
    eRTK_perfcounter[ ( eRTK_iperf++ )/*&0x0f*/ ]=eRTK_perfcount;
    if( eRTK_perfcount<1 ) {
      ++eRTK_cnt_overload;
      if( eRTK_MAX_OVERLOAD && eRTK_cnt_overload>eRTK_MAX_OVERLOAD ) deadbeef( SYS_OVERLOAD ); 
     }
    else eRTK_cnt_overload=0;
    eRTK_perfcount=0;
   }
  if( eRTK_up ) eRTK_scheduler();
 }


#if defined (__AVR_ATmega2560__)
ISR( TIMER0_OVF_vect ) { //1kHz timer
  TCNT0=( uint8_t )( -1*TIMERPRELOAD );
  eRTK_timertick();
  adc_sequencer();
 }

static void timer0_init( void ) {
  TCCR0B=/*(1<<CS12)|*/(1<<CS01)|(1<<CS00); //prescaler CLK/64
  TIMSK0|=(1<<TOIE0);
  TCNT0=( uint8_t )( -1*TIMERPRELOAD );
 }
#endif

//#include <avr/iox384c3.h>
#if defined (__AVR_ATxmega384C3__)
ISR( TCC0_OVF_vect ) { //1kHz timer
  TCC0.CNT=( uint8_t )( -1*TIMERPRELOAD );
  eRTK_timertick();
  //adc_sequencer();
 }
 
static void timer0_init( void ) {
  TCC0.CTRLA=5; //prescaler CLK/64
  TCC0.INTFLAGS|=1; //overflow int flag
  TCC0.CNT=( uint8_t )( -1*TIMERPRELOAD );
 }
#endif

void eRTK_timer_init( void ) {
#if defined (__AVR_ATmega2560__)
  timer0_init();
#elif defined (__AVR_ATxmega384C3__)
  timer0_init();
#endif
 }




#if eRTKTEST
uint16_t loop1, loop2;
void TMain1( uint16_t param0, void *param1 ) {
  uint16_t p0=param0;
  uint16_t p1=param1;
  while( 1 ) {
    uint8_t now=eRTK_GetTimer();
    uint32_t cnt;
    for( cnt=0; cnt<1000; cnt++ );
    eRTK_WaitUntil( now+10 );
   }
  while( 0 ) {
    eRTK_get_sema( 0 );
    write0( ( uint8_t* )"1234567890", 10 );
    eRTK_free_sema( 0 );
   }
  while( 1 ) {
  	char tmp[100];
    strcpy( tmp, "12345678901234567890123456789012345678901234567890" );
   }
  while( 1 ) {
    eRTK_wefet( 1 );
    ++loop1;
    eRTK_wefet( 1 );
    --loop1;
   }
 }

void TMain2( uint16_t param0, void *param1 ) {
  uint16_t p0=param0;
  uint16_t p1=param1;
  while( 0 ) {
    eRTK_get_sema( 0 );
    write0( ( uint8_t* )"abcdefghij", 10 );
    eRTK_free_sema( 0 );
   }
  while( 1 ) {
	  char tmp[100];
	  strcpy( tmp, "abcdefghijklabcdefghijklabcdefghijklabcdefghijkl" );
   }
  while( 1 ) {
    eRTK_wefet( 1 );
    ++loop2;
    eRTK_wefet( 1 );
    --loop2;
   }
 }
#endif
