/*
 * eRTK.cpp
 *
 * Created: 13.04.2015 07:52:46
 *  Author: er
 */ 
#include <string.h>
#include <stdio.h>


#if defined (__AVR_ATmega2560__)|(__AVR_ATxmega384C3__)
  #include <avr/io.h>
  #include <avr/interrupt.h>
  #include <avr/sleep.h>
  #include <avr/wdt.h>
  #include "AVR/adc.h"
#elif defined (__SAMD21J18A__)
  //arm cortex nvic systimer register on SAMD21
  #define NVIC_SYSTICK_CTRL		( ( volatile uint32_t *) 0xe000e010 )
  #define NVIC_SYSTICK_LOAD		( ( volatile uint32_t *) 0xe000e014 )
  #define NVIC_SYSTICK_CLK		4
  #define NVIC_SYSTICK_INT		2
  #define NVIC_SYSTICK_ENABLE	1
#endif

#include "eRTK_config.h"
#include "eRTK.h"

#if defined (__AVR_ATmega2560__) ||(__AVR_ATxmega384C3__)
  #ifdef ERTK_DEBUG
    uint8_t stack[VANZTASK+1][ERTK_STACKSIZE]  __attribute__ ((aligned (256)));  /* Jede Task hat einen Stack a' STACKSIZE Byte */
  #else
    uint8_t stack[VANZTASK+1][ERTK_STACKSIZE];  /* Jede Task hat Stack a' STACKSIZE Byte */
  #endif
#elif defined (__SAMD21J18A__)
  #ifdef ERTK_DEBUG
    uint32_t stack[VANZTASK+1][ERTK_STACKSIZE]  __attribute__ ((aligned (4096)));  /* Jede Task hat einen Stack a' STACKSIZE words */
  #else
    uint32_t stack[VANZTASK+1][ERTK_STACKSIZE];  /* Jede Task hat Stack a' STACKSIZE words */
  #endif
#endif

void * stackptr[VANZTASK+1]; /* Ablage fuer Stackpointer der Tasks */

volatile eRTK_TYPE akttask;    /* Nummer der aktuellen Task, 0=idle */

/* struktur task control block */
typedef struct s_tcd {    /* queue mit ordnungskriterium task prioritaet 255..0 */
  struct s_tcd * pnext;   /* 1. element immer verkettungszeiger !!! */
  struct s_tcd * pbefore; /* vorgaenger in liste */
  eRTK_TYPE tid;          /* index aus der reihenfolge der statischen deklaration */
  eRTK_TYPE prio;         /* 0..255 */
  eRTK_TYPE timer;        /* timeout counter mit system tick aufloesung */
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
*/
#ifdef ERTK_DEBUG
  #if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
    volatile uint16_t eRTK_perfcount;        //aktueller counter
    volatile uint16_t eRTK_perfcounter[256]; //counter array der letzen 256ms
    volatile uint16_t eRTK_ticks;            //wie spaet ist es nach dem urknall in ms
    volatile uint8_t  eRTK_cnt_overload;     //zaehlt die aufeinanderfolgenden overload phasen
  #elif defined (__SAMD21J18A__)
    volatile uint32_t eRTK_perfcount;        //aktueller counter
    volatile uint32_t eRTK_perfcounter[256]; //counter array der letzen 256ms
    volatile uint32_t eRTK_ticks;            //wie spaet ist es nach dem urknall in ms
    volatile uint32_t  eRTK_cnt_overload;    //zaehlt die aufeinanderfolgenden overload phasen
  #endif
  volatile uint8_t  eRTK_up;                 //wird gesetzt wenn das system gestartet ist
  volatile uint8_t  eRTK_iperf;              //index im array 0..255
#endif

#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
void  __attribute__ ((optimize("O2"))) eRTK_Idle( void ) {
  #ifdef ERTK_DEBUG
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
#elif defined (__SAMD21J18A__)
void  __attribute__ ((optimize("O2"))) eRTK_Idle( void ) {
  #ifdef ERTK_DEBUG
    while( 1 ) {           //S=14T
    cli();               //cpsid i 1T
    ++eRTK_perfcount;    //ldr r2, [r3] / adds r2, #1 / str	r2, [r3]  2T+1T+2T
    sei();               //cpsie i 1T
    oIDLEfast( 1 );      //ldr	r1, [r3] / orrs	r1, r0 / str	r1, [r3]   2T+1T+2T
   }                     //b	#-18 2T
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
#endif


__attribute__ ((noinline)) void deadbeef( tsys reason ) {
  oIDLE( 0 ); 
  while( 1 ); //this is the end...
 }


void * pp_stack; //speicher fuer stackpointer waehrend push/pop

#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
//push pc, push r0, r0<-sreg, push r0..r31
#define push() { \
  asm volatile ( \
  "push	r0 \n"/*push r0*/ \
  "in r0, __SREG__ \n" \
  "cli\n" /*push sreg, push r1..r31 */\ 
  "push	r0\npush  r1\npush	 r2\npush  r3\npush	 r4\npush  r5\npush	 r6\npush  r7\npush	 r8\npush  r9\npush	r10\npush	r11\npush	r12\npush	r13\npush	r14\npush	r15\npush	r16\npush	r17\npush	r18\npush	r19\npush	r20\npush	r21\npush	r22\npush	r23\npush	r24\npush	r25\npush	r26\npush	r27\npush	r28\npush	r29\npush	r30\npush	r31\n" \
  "in r0, __SP_L__ \n" \
  "sts pp_stack, r0 \n" \
  "in r0, __SP_H__ \n" \
  "sts pp_stack+1, r0 \n" \
  ); \
 }

//pop r31 .. r0, sreg<-r0, pop r0
#define pop() { \
  asm volatile ( \
  "lds r0, pp_stack \n" \
  "out __SP_L__, r0 \n" \
  "lds r0, pp_stack+1 \n" \
  "out __SP_H__, r0 \n" \
  /*pop r31..sreg*/ \
  "pop r31\npop	r30\npop	r29\npop	r28\npop	r27\npop	r26\npop	r25\npop	r24\npop	r23\npop	r22\npop	r21\npop	r20\npop	r19\npop	r18\npop	r17\npop	r16\npop	r15\npop	r14\npop	r13\npop	r12\npop	r11\npop	r10\npop	 r9\npop	 r8\npop	 r7\npop	 r6\npop	 r5\npop	 r4\npop	 r3\npop	 r2\npop	 r1\npop	 r0\n" \
  "out __SREG__, r0 \n" \
  "pop r0 \n"/*pop r0*/ \
  "sei \n" \
  ); \
 }
#endif

#if defined (__SAMD21J18A__)
//r8,r9,r10,r11,r12,r0,r1,r2,r3,r4,r5,r6,r7,lr
#define push() { \
  asm volatile ( \
  /*direkt kann man nur die lo regs pushen*/ \
  "cpsid i \n" \
  "push { r0-r7, lr } \n"   /*r0-r7 und linkregister auf stack*/ \
  "mov r0, r8 \n" \
  "mov r1, r9 \n" \
  "mov r2, r10 \n" \
  "mov r3, r11 \n" \
  "mov r4, r12 \n" \
  "push { r0-r4 } \n"       /*r8-r12 auf stack*/ \
  "mrs r0, msp \n"          /*msp nach r0*/ \
  "ldr r1, =pp_stack \n" \
  "str r0, [ r1 ] \n" \
  ); \
 }
/*nun sind alle regs gesichert auf stack, stackpointer in pp_stack gesichert*/

/*der stackpointer in pp_stack liegt vor, alle register werden restauriert und der link adressiert*/
#define pop() {  \
  asm volatile ( \
  "ldr r0, =pp_stack \n"    /*zeiger auf pp_stack */     \
  "ldr r0, [ r0 ] \n"       /*inhalt pp_stack */         \
  "msr msp, r0 \n"          /*speichere pp_stack in msp*/\
  "cpsie i \n"              /*sperre interrupts */       \
  "pop { r0-r4 } \n"        /*hole r8-r12*/              \
  "mov r8, r0 \n" \
  "mov r9, r1 \n" \
  "mov r10, r2 \n" \
  "mov r11, r3 \n" \
  "mov r12, r4 \n" \
  "pop { r0-r7, pc } \n"   /*restauriere r0-r7, springe nach link*/ \
  ); \
 }
 
#endif


void __attribute__ ((naked)) eRTK_scheduler( void ) { /* start der hoechstprioren ready task, oder idle task, oder der bueffel wenn alles scheitert ;) */
  push();
#ifdef ERTK_DEBUG 
  //stack overflow check, stack pointer in pp_stack uebergeben
  if( pp_stack < ( void * )&stack[akttask][ERTK_STACKSIZE-ERTK_STACKLOWMARK] ) deadbeef( SYS_STACKOVERFLOW );
#endif
  stackptr[akttask]=pp_stack;
  //
  if( pTaskRdy ) { //da muss natuerlich immer was drinstehen ;)
    //do round robin bei mehreren mit gleicher prio
    register s_tcd *p=pTaskRdy;
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
    else {
      akttask=pTaskRdy->tid; //nimm das erstbeste aus der ready liste ;)
     }
   }
  else deadbeef( SYS_NOTASK );
  //
  pp_stack=stackptr[akttask];
  pop();
#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)  
  sei();
  asm volatile ( "ret" );
#elif defined (__SAMD21J18A__)
#endif  
 }


void eRTK_go( void ) { /* start der hoechstprioren ready task, notfalls idle */
  if( pTaskRdy ) akttask=pTaskRdy->tid;
  else deadbeef( SYS_NOTASK );
  //
  eRTK_up=1;
  pp_stack=stackptr[akttask];
#if defined (__SAMD21J18A__)
  asm volatile(	"movs r0, #0 \n" ); /* privilegierte ausfuehrung und nehme den msp stack. */
  asm volatile(	"msr CONTROL, r0 \n" );
#endif
  pop();
#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
  asm volatile( "ret" );
#elif defined (__SAMD21J18A__)
#endif
  deadbeef( SYS_UNKNOWN ); //hier duerfen wir nie wieder ankommen, wenn die verwaltungsstrukturen i.O. sind
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

eRTK_TYPE eRTK_GetTid( void ) { //holen der eigenen task id
  return akttask;
 }

//Verwaltung der ready list mit einfuegen/ausfuegen in der reihenfolge der prio
void eRTK_SetReady( eRTK_TYPE tid ) {
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
void eRTK_SetSuspended( eRTK_TYPE tid ) { //tcd[tid] aus der ready list austragen
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

void eRTK_wefet( eRTK_TYPE timeout ) {
  if( timeout ) { //sonst klinkt sich die task in einem wait_until() fuer immer aus
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) { //14+2=16 cycles pro loop -> 16MHz -> 1000 inc/ms
      if( tcd[akttask].timer ) deadbeef( SYS_UNKNOWN );
      tcd[akttask].timer=timeout;
      eRTK_SetSuspended( akttask );
      eRTK_scheduler();
     }
   }
#ifdef ERTK_DEBUG   
  else deadbeef( SYS_NULLTIMER ); 
#endif  
 }


void eRTK_Sleep_ms( uint16_t ms ) { //macht nur auf avr sinn
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
static eRTK_TYPE sema[ANZSEMA];

static inline __attribute__((__always_inline__)) uint8_t xch ( eRTK_TYPE volatile *p, eRTK_TYPE x ) {
  //wenn die (AVR) cpu xch kennt:
  // __asm volatile ("xch %a1,%0" : "+r"(x) : "z"(p) : "memory");
  register uint8_t tmp;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE ) {
    tmp=*p;
    *p=x;
   }
  return tmp;
  //return x;
 }

void eRTK_get_sema( eRTK_TYPE semaid ) { /* Warten bis Semaphore frei ist und danach besetzen */
  if( semaid>=ANZSEMA ) deadbeef( SYS_UNKNOWN );
  while( xch( &sema[semaid], 1 ) ) { /* >0 = sema blockiert */
    sei();
    eRTK_wefet( 1 );
   }
 }

void eRTK_free_sema( eRTK_TYPE semaid ) {
  if( semaid>=ANZSEMA ) deadbeef( SYS_UNKNOWN );
  xch( &sema[semaid], 0 ) ;
 }

void sema_init( void ) {
  static eRTK_TYPE n;
  for( n=0; n<ANZSEMA; n++ ) {
    sema[n]=0; /* ff=keine task wartend */
   }
 }

/* 
AVR Stackbelegung:
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
  eRTK_TYPE n, prio, index;
  eRTK_TYPE task;
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
#if defined (__AVR_ATmega2560__) ||(__AVR_ATxmega384C3__)
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
#elif defined (__SAMD21J18A__)
  for( n=0; n<VANZTASK+1; n++ ) { /* SP der Tasks auf jeweiliges Stackende setzen */
    for( int f=0; f<ERTK_STACKSIZE; f++ ) stack[n][f]=0xdeadbeef;
    //cortex m0+ arm thumb register set
    //r0-r12 general purpose registers
    //r13=MSP stack pointer
    //r14=LR link register
    //r15=PC program counter
    //PSR program status register
    //PRIMASK
    //CONTROL
	  //ARM Stackbelegung: r8,r9,r10,r11,r12,r0,r1,r2,r3,r4,r5,r6,r7,lr=pc
    stack[n][ERTK_STACKSIZE-14]=8;
    stack[n][ERTK_STACKSIZE-13]=9;
    stack[n][ERTK_STACKSIZE-12]=10;
    stack[n][ERTK_STACKSIZE-11]=11;
    stack[n][ERTK_STACKSIZE-10]=12;
    stack[n][ERTK_STACKSIZE-9]=0;
    stack[n][ERTK_STACKSIZE-8]=1;
    stack[n][ERTK_STACKSIZE-7]=2;
    stack[n][ERTK_STACKSIZE-6]=3;
    stack[n][ERTK_STACKSIZE-5]=4;
    stack[n][ERTK_STACKSIZE-4]=5;
    stack[n][ERTK_STACKSIZE-3]=6;
    stack[n][ERTK_STACKSIZE-2]=7;
    if( n ) stack[n][ERTK_STACKSIZE-1]=( unsigned )rom_tcb[n-1].task;
    else stack[n][ERTK_STACKSIZE-1]=( unsigned )eRTK_Idle;
    stackptr[n]=&stack[n][ERTK_STACKSIZE-14];
   }
#endif 
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
  volatile uint8_t timer8[4];
  volatile uint16_t timer16[2];
  volatile uint32_t timer32;
 } eRTK_m_timer;
 
uint8_t eRTK_GetTimer8( void ) { //256ms bis overflow
  return eRTK_m_timer.timer8[0];
 }

uint16_t eRTK_GetTimer16( void ) { //wenn 256ms nicht reichen
  register uint16_t val;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE	) { 	
    val=eRTK_m_timer.timer16[0];
   }
  return val;
 }

#if defined (__SAMD21J18A__)
uint32_t eRTK_GetTimer32( void ) { //wenn 65s nicht reichen
  register uint32_t val;
  ATOMIC_BLOCK( ATOMIC_RESTORESTATE	) {
    val=eRTK_m_timer.timer32;
   }
  return val;
 }
#endif

void eRTK_WaitUntil( eRTK_TYPE then ) {
#if defined (__AVR_ATmega2560__) ||(__AVR_ATxmega384C3__)
  eRTK_wefet( then-eRTK_GetTimer8() );
#elif defined (__SAMD21J18A__)
  eRTK_wefet( then-eRTK_GetTimer32() );
#endif  
 }

//setzen der prioritaet fuer eine task
//tid=0 setze die eigene prioritaet oder sonst die einer anderen
//prio=neue prioritaet
//schedule_immediately wenn true dann wird sofort eine neue Prozesstabelle ermittelt und der hoechstpriore prozess gestartet
void eRTK_cpri( eRTK_TYPE tid, eRTK_TYPE prio, eRTK_TYPE schedule_immediately ) {
  if( tid==0 ) tid=akttask;
  if( tid<VANZTASK ) {
    tcd[akttask].prio=prio;
    if( schedule_immediately ) eRTK_scheduler();	
   } else deadbeef( SYS_VERIFY ); //die task id ist unbekannt
 }

#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
__inline__ void __attribute__ ( ( always_inline ) ) //damit im irq alle register gesichert werden
#elif defined (__SAMD21J18A__)
void 
#endif
eRTK_timertick( void ) { 
  oIDLE( 0 );
#if 0
//  //stack overflow check, stack pointer in pp_stack uebergeben
//  #if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
//    asm volatile( "in r0, __SP_L__ \n" );
//    asm volatile( "sts pp_stack, r0 \n" );
//    asm volatile( "in r0, __SP_H__ \n" );
//    asm volatile( "sts pp_stack+1, r0 \n" );
//  #elif defined (__SAMD21J18A__)
//    asm volatile ( "mrs r0, msp \n" );
//    asm volatile ( "ldr r1, =pp_stack \n" );
//    asm volatile ( "str r0, [ r1 ] \n" );
//  #endif
//  if( pp_stack < ( void * )&stack[akttask][ERTK_STACKSIZE-ERTK_STACKLOWMARK] ) deadbeef( SYS_STACKOVERFLOW );
#endif  
  if( eRTK_ticks<UINT_MAX ) ++eRTK_ticks;
#if defined (__AVR_ATmega2560__)||(__AVR_ATxmega384C3__)
  ++eRTK_m_timer.timer16;
#elif defined (__SAMD21J18A__)
  ++eRTK_m_timer.timer32;
#endif
  register s_tcd *p=tcd;
  //timer service
  for( register eRTK_TYPE n=0; n<VANZTASK+1; n++ ) {
    if( p->timer ) {
      if( !--p->timer ) {
        eRTK_SetReady( p->tid );
       }
     }
    ++p;
   }
#ifdef ERTK_DEBUG   
  if( eRTK_up && eRTK_ticks>eRTK_STARTUP_MS ) { //solange geben wir dem system zum hochlauf
    eRTK_perfcounter[ ( eRTK_iperf++ )/*&0x0f*/ ]=eRTK_perfcount;
    if( eRTK_perfcount<1 ) {
      ++eRTK_cnt_overload;
      if( eRTK_MAX_OVERLOAD && eRTK_cnt_overload>eRTK_MAX_OVERLOAD ) deadbeef( SYS_OVERLOAD ); 
     }
    else eRTK_cnt_overload=0;
    eRTK_perfcount=0;
   }
#endif   
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

#if defined (__SAMD21J18A__)
void SysTick_Handler( void ) {
  //asm volatile ( "mrs r0, primask\n" );
  //asm volatile ( "cpsid i" );
  eRTK_timertick();
  //asm volatile ( "cpsie i" );
  //asm volatile ( "msr primask, r0\n" );  
 }

void init_Systimer( void ) {
  *(NVIC_SYSTICK_LOAD) = ( F_CPU / eRTKHZ ) - 1UL;
  *(NVIC_SYSTICK_CTRL) = NVIC_SYSTICK_CLK | NVIC_SYSTICK_INT | NVIC_SYSTICK_ENABLE;
 }
#endif



void eRTK_timer_init( void ) {
#if defined (__AVR_ATmega2560__)
  timer0_init();
#elif defined (__AVR_ATxmega384C3__)
  timer0_init();
#elif defined (__SAMD21J18A__)
  init_Systimer();
#endif
 }

