
/* task states */
#define READY   1
#define WAITING 0
#ifndef _MSC_VER
#asm
READY 			equ 1
WAITING 		equ 0
#endasm
#endif

/* struktur task control block */
typedef struct tagTCD { /* queue mit ordnungskriterium task prioritaet 255..0 */
  word * next;   /* 1. element immer verkettungszeiger !!! */
  byte id;       /* aus der reihenfolge der statischen deklaration */
  byte pri;      /* 0..255 */
  byte state;    /* 0=ready, 1=waiting */
 } s_tcd;

#ifndef _MSC_VER
#asm
;tcd element offsets
OFFSET_ID		equ 2
OFFSET_PRI		equ 3
OFFSET_STATE	equ 4
SIZE_TCD		equ 5
#endasm	    
#endif

#define OFFSET_ID  		2
#define OFFSET_PRI 		3
#define OFFSET_STATE	4
#define SIZE_TCD		5

extern s_tcd tcd[]; /* indizierung ueber task index */
extern s_tcd * taskliste; /* einfache verkettung ueber tcd.next in reihenfolge
                            der prioritaet */

extern byte systimer[]; /* indizierung ueber task index */
                            
void x_scheduler( void );
void stck_overflow( void );

extern byte TASK_IDLE; /* immer ready */
extern byte TASK_MAIN; /* init task */
extern byte TASK_3964;
extern byte TASK_MEMTEST;
extern byte TASK_READER;
extern byte TASK_DISPLAY;
extern byte TASK_DCF77;
extern byte TASK_IO;
extern byte TASK_STG0;
extern byte TASK_STG1;                    
extern byte TASK_FUNK; /* muss noch gestartet werden */

extern s_tcd * const tcd_adr[];	/* pointer auf die tcds indiziert ueber taskindex ! */

extern byte * stackptr[];  	/* Ablage fuer Stackpointer der Tasks */

extern byte akttask;             	/* Nummer der aktuellen Task */

extern byte anztask;      /* const fuer Zahl der Tasks */

byte x_start_task( byte taskid );
void x_short_sleep( byte time ); /* task fuer intervall suspendieren */
void x_pause( void ); /* Weiterschalten zur naechsten Task per Programmbefehl */
void x_sleep( word longtime ); /* 16 bit zeiten realisieren */
void x_sefet( byte task, byte time );                  
void x_sefet_disabled( byte task, byte time );
void x_wef( void );
void x_wef_timer( void );
void x_init_scheduler( void ); /* Initialisierung des Echtzeitsystems */
void x_start( void ); /* start der hoechstprioren ready task */
void x_sysclock( void ); /* call mit system timer interrupt im task kontext */
void x_scheduler( void ); /* hardware taskswitch per Timer Interrupt */
