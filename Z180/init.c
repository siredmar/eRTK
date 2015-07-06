#ifndef _MSC_VER
#asm
;hd64180
        
         extern system_tos
         extern init_asci,init_prt, scc_init
         extern x_init_scheduler, x_start, adu_init
         extern stack, stackptr, akttask
         extern asci0_txd_wpos, asci0_txd_rpos, asci0_rxd_wpos, asci0_rxd_rpos
         extern asci1_txd_wpos, asci1_txd_rpos, asci1_rxd_wpos, asci1_rxd_rpos
         extern mmu_init, sema_init
         extern vm_mem_test
         extern system_stop, adu_identify
         extern jahr
         extern init_d
         extern io_init
         extern logslcd; anzeige hochlaufmeldungen
         extern logprintf
         extern never              
         extern InitRomCs
start:
  org 0               ;cold boot
  di
  jr boot
  ;
  org 8
  jp start
  ;
  org 10h
  jp start
  ;
  org 18h
  jp start
  ;
  org 20h
  jp start
  ;
  org 28h
  jp start
  ;
  org 30h
  jp start
  ;
  org 38h
  jp never  ;interner check fail trap als provisorium
  ;       
  org 66h ;nmi vector u. int1 einsprung
nmivect:
  di
  ld sp, system_tos
  ld c, 40h; zeitschleife
  ld b, 0
l1: 
  djnz l1
  dec c
  jr nz, l1
  ld hl, T_NMI
  push hl
  call logslcd
  pop hl
  ld hl, 0
  push hl
  retn;     jp 0 mit retn befehl zum nmi flip flop clear
  ;
boot:         
   extern init_mmu
   jp init_mmu
   public init_mmu_return    
init_mmu_return:          
  ;
  ;ram ist nun vorhanden !
  ;
  ld a,0
  out0 (RCR),a    ;kein refresh
  ;
MHZ6
  IFDEF MHZ6
  title module init for 6,144 MHz
  ld a, 00000000b;  keine wait's im memory, 1 wait im I/O
  ENDIF
  IFDEF MHZ9
  title module init for 9,216 MHz
  ld a, 00010001b;  1 wait im memory, 2 wait's im I/O
  ENDIF
  out0 (DCNTL),a  
  ;
  ld sp, system_tos
  in0 a, ( 34h )
  bit 7, a  
  jp z, init; normaler reset
  ;
  ;andernfalls: undefined fetch object
  xor a
  ld hl, T_UOF
  push hl
  call logslcd
  pop hl
  call system_stop
stop:
  di
  halt               ;undefined fetch object
  ;
  ;
testbyte:
  db 55h
  ;
#endasm         
#endif


#include "types.h"
#include "init.h"
#include "data.h"
#include "text.h"
#include "ssn180.h"
#include "ident.h"
#include "funcs.h"
#include "io.h"

#ifndef _MSC_VER
#asm
		 org 00e0h;		Interruptvektoren
  extern asci1,asci0,prt1,prt0,int2,csi,dma1,dma0,int1
  public ints,i_int1,i_int2,i_prt0,i_prt1,i_dma0,i_dma1,i_csi,i_asci0,i_asci1
ints:
i_int1:     dw int1
i_int2:     dw int2
i_prt0:     dw prt0
i_prt1:     dw prt1
i_dma0:     dw dma0
i_dma1:     dw dma1
i_csi:      dw csi
i_asci0:    dw asci0
i_asci1:    dw asci1
		 
	     org 0ffh
		 db 0;			cs korrekturbyte fur quersumme=0 ueber prom bereich 0..afff

         org 100h
init:
         out0 ( PWATCHDOG ), a
         xor a
         ld a, 0
         out0 ( 0a0h ), a
         out0 ( 0e0h ), a    ;alle ausgaenge inaktiv
         ;
         ; cpu testen
         xor a
         ld b, a
         ld c, a
         ld d, a
         ld e, a
         ld h, a
         ld l, a
         or b
         or c
         or d
         or e
         or h
         or l
         cp 0
         jp nz, cpudefekt
         ;
         dec a
         jp p, cpudefekt
         ;
         dec b
         jp p, cpudefekt
         ;
         dec c
         jp p, cpudefekt
         ;
         dec d
         jp p, cpudefekt
         ;
         dec e
         jp p, cpudefekt
         ;
         dec h
         jp p, cpudefekt
         ;
         dec l
         jp p, cpudefekt
         ;
         ld ix, testbyte
         ld iy, testbyte
         ld a, ( testbyte )
         cp ( ix+0 )
         jp nz, cpudefekt
         ;
         cp ( iy+0 )
         jp nz, cpudefekt
         ;
         ex af, af'
         exx
         xor a
         ld b, a
         ld c, a
         ld d, a
         ld e, a
         ld h, a
         ld l, a
         or b
         or c
         or d
         or e
         or h
         or l
         cp 0
         jp nz, cpudefekt
         ;
         dec a
         jp p, cpudefekt
         ;
         dec b
         jp p, cpudefekt
         ;
         dec c
         jp p, cpudefekt
         ;
         dec d
         jp p, cpudefekt
         ;
         dec e
         jp p, cpudefekt
         ;
         dec h
         jp p, cpudefekt
         ;
         dec l
         jp p, cpudefekt
         ;
         ld ix, testbyte
         ld iy, testbyte
         ld a, ( testbyte )
         cp ( ix+0 )
         jp nz, cpudefekt
         ;
         cp ( iy+0 )
         jp nz, cpudefekt
         ;
         ex af, af'
         exx
         xor a
         push af
         pop bc; test ob af wert im ram steht, oder ram defekt ist
         push bc; b = a
         sub b
         jp nz, ramerr; anzeige ram error bei inkonsistentem stack ohne call
         ;
         inc sp
         pop af
         jp z, cpudefekt
         ;
         jp c, cpudefekt
         ;
         jp pe, cpudefekt
         ;
         jp m, cpudefekt
         ;
         dec sp
         ld a, 0ffh
         push af
         inc sp
         pop af
         jp nz, cpudefekt
         ;
         jp nc, cpudefekt
         ;
         jp po, cpudefekt
         ;
         jp p, cpudefekt
         ;
         dec sp
         ;
         ld hl, T_CPUOK
         push hl
         call logslcd
         pop hl
         out0 ( PWATCHDOG ), a
         ;
         ;
         call io_init
         ;
         call mmu_init; speichertest wird es ueberschreiben
         ;
         ld hl, 0
         ld ( kaltstart ), hl
         ;mit fehlertaste bei anlauf immer kaltstart
         INFEHLER1
         jr nz, mem_test
         ;
         INFEHLER2
         jr nz, mem_test
         ;   
         jp no_mem_test
         ;
         public mem_test
mem_test:    
         ld hl, -1
         ld ( kaltstart ), hl
         ;
         ld hl, T_MEMTEST
         push hl
         call logslcd
         pop hl
_55:
         out0 ( PWATCHDOG ), a
         ;kaltstartmerker in alternativem a register sichern
         ex af, af'
         ld a, ( kaltstart )
         ex af, af'
         ;
         ;rambeginn durch schreibtest finden
find_ram_start:         
         ld hl, 0
next_page:
         ld a, ( hl )
         xor 0ffh;     komplement bilden
         ld ( hl ), a; schreiben
         cp ( hl );    und vergleichen 
         jr z, hl_points_to_ram
         ;
         ld a, 10h
         add a, h
         ld h, a
         jr next_page
         ;        
hl_points_to_ram:         
         ld d, h
         ld e, l
         ;
; hl=startadresse, de=ist merker fuer startadresse

fill55:  ;fill from de up to efff with 55h  
         ld a, 55h
         ld b, 0;    256 bytes
loop55:  ld ( hl ), a
         inc hl
         djnz loop55
         out ( PWATCHDOG ), a; watchdog
         ld a, h
         cp 0f0h
         jr nz, fill55

test55:  ;test from de up to ffff if byte is 55h ( run time is 90 msec )
         ld h, d
         ld l, e; startadresse in hl
         ld a, d
         add a, 10h
         xor 0ffh
         ld b, a
         ld c, 0ffh; bc ist byte count bis ffff
         ld a, 55h
t55:     out ( PWATCHDOG ), a
         cpi
         jp po, test55ok
         jp nz, ramerr; abbruch vor ablauf vom byte count zaehler
         jr t55

test55ok:
         ld h, d
         ld l, e
fillaa:  ;fill from de up to ffff with aah  
         ld a, 0aah
         ld b, 0;    256 bytes
loopaa:  ld ( hl ), a
         inc hl
         djnz loopaa
         out ( PWATCHDOG ), a; watchdog
         ld a, h
         cp 0f0h
         jr nz, fillaa

testaa:  ;test from de up to ffff if byte is aah ( run time is 90 msec )
         ld h, d
         ld l, e; startadresse in hl
         ld a, d
         add a, 10h
         xor 0ffh
         ld b, a
         ld c, 0ffh; bc ist byte count bis ffff
         ld a, 0aah
taa:     out ( PWATCHDOG ), a
         cpi 
         jp po, testaaok
         jp nz, ramerr
         jr taa
testaaok:
    public ramok
ramok:
         ld h, d
         ld l, e
fill00:  ;fill from de up to ffff with 00h  
         ld a, 0
         ld b, 0;    256 bytes
loop00:  ld ( hl ), a
         inc hl
         djnz loop00  
         ;
         out ( PWATCHDOG ), a; watchdog
         ld a, h
         cp 0f0h
         jr nz, fill00
         ;
         ; kaltstartmerker aus a' restaurieren
         ex af, af'
         ld ( kaltstart ), a
         ex af, af'
         ;
         call mmu_init; speichertest hat die seg_tab zerstoert
         ;call vm_mem_test
         ;
         ;
no_mem_test: ; wenn kein speichertest erfolgt
  public go
go:
         ld hl, T_MEMOK
         push hl
         call logslcd
         pop hl
         ;
         extern lcd_hochlauf
         ld a, -1
         ld ( lcd_hochlauf ), a
         ;
         ld a,00000100b  
         out0 (itc),a    	;interrupt INT2 freigeben
         ;
         ld a, high ints
         ld i, a          	;I -Vektor MSB
         ld a, low ints
         and 11100000b  	;I -Vektor LSB ist e0, e2, e4, e6, e8, ea, ec, ee, f0 
         out0 ( IL ), a
         im 2         
         ;
         ;ld a, 10
         ;ld ( jahr ), a;  anfangsjahr ist 1980+10=1990
         ;                                             
         ;pruefe ob adu installiert ist, wenn nicht dann nur mit zaehlung arbeiten
         call adu_identify
         ;
         ld hl, T_INITASCI
         push hl
         call logslcd
         pop hl 
         out0 ( PWATCHDOG ), a
         call init_asci       ;ASCI initialisieren
         call scc_init
         out0 ( PWATCHDOG ), a
         ;
         ld c, 80h
@1:      djnz @1
         out0 ( PWATCHDOG ), a
         dec c
         jr nz, @1
         ;
         ld hl, T_INITPRT
         push hl
         call logslcd
         pop hl
         call init_prt        ;timer initialisieren
         out0 ( PWATCHDOG ), a
         ;
         ld hl, T_INITSYS
         push hl
         call logslcd
         pop hl
         call x_init_scheduler  ;Taskverwaltung initialisieren
         out0 ( PWATCHDOG ), a
         ;
         ;testen ob speicher ok
         ld a, ( kaltstart )
         or a
         jr z, no_pool_init
         ;
;         ld hl, T_INITMB
;         push hl
;         call logslcd
;         pop hl
;         call mb_init         ;mailboxen
;         out0 ( PWATCHDOG ), a
         ;
no_pool_init:;                 wenn speicher ok ist
         ld hl, T_INITSEMA
         push hl
         call logslcd
         pop hl
         out0 ( PWATCHDOG ), a
         call sema_init       ;semaphoren
         out0 ( PWATCHDOG ), a
         ;
         ld hl, T_INITADU
         push hl
         call logslcd
         pop hl
         call adu_init        ;adu steuerung init
         ;
         ;seriennummer pruefen
         ld hl, T_DSCHECK
         push hl
         call logslcd
         pop hl 
         out0 ( PWATCHDOG ), a
         call ds_check  
         ;<-----SSN FEST EINPROGRAMMIEREN ZUM TEST = milc500
         ;ld ix, code
         ;ld ( ix+0 ), 01h;
         ;ld ( ix+1 ), 3ch;
         ;ld ( ix+2 ), 08h;
         ;ld ( ix+3 ), 03h;
         ;ld ( ix+4 ), 0;
         ;ld ( ix+5 ), 0;
         ;ld ( ix+6 ), 0;
         ;ld ( ix+7 ), 1bh;
         ;
         ld hl, T_RTCRUN
         push hl
         call logslcd
         pop hl
         extern rtc_run
         out0 ( PWATCHDOG ), a
         call rtc_run;   oszillator einschalten
         ;
         out0 ( PWATCHDOG ), a
         call init_d
         ;
         call InitRomCs; kurze anzeige der rom crc ( -F -> crc passt nicht )
         ;
; Beginn Hauptprogramm
         ld hl, T_START
         push hl
         call logslcd
         pop hl
         jp x_start      ;Betriebssystem starten



cpudefekt:              ;cpu fehler festgestellt
        di
        halt

ramerr:                 ;fehler im RAM
; lcd initialisieren

wait0:
    in0 a,(0c0h)
    and 80h
    jr nz,wait0
    ld a,00111100b  ;function set: 8 bit, 2 lines, 5x10 dots
    out0 (0c0h),a
wait1:
    in0 a,(0c0h)
    and 80h
    jr nz,wait1
    ld a,00000001b  ;clear display
    out0 (0c0h),a
wait2:
    in0 a,(0c0h)
    and 80h
    jr nz,wait2
    ld a,00001100b  ;display on, cursor off, cursor blink off
    out0 (0c0h),a
wait3:
    in0 a,(0c0h)
    and 80h
    jr nz,wait3
    ld a,00000110b
    out0 (0c0h),a   ;entry mode set: increment, no shift
wait4:
    in0 a,(0c0h)
    and 80h
    jr nz,wait4
    ld a,10000000b
    out0 (0c0h),a

    ld hl,ERRTEXT
    ld b, 8
wait5:
    in0 a,(0c0h)
    and 80h
    jr nz,wait5
    ld a,(hl)
    out0 (d_disp),a
    inc hl
    djnz wait5
    call system_stop
    di
    halt

ERRTEXT: db 'RAMERROR', 0

#endasm         
#endif


static byte ds_compare( word index ) {/* nicht reentrant */   
  static byte * p;
  static byte failed;
  ;
  failed=0;
  p=( byte * )&SSN_MSW[index];
  if( ( p[1]^SN_MASKE )!=code[1] ) failed=0xff;
  if( ( p[0]^SN_MASKE )!=code[2] ) failed=0xff;
  p=( byte * )&SSN[index];
  if( ( p[3]^SN_MASKE )!=code[3] ) failed=0xff;
  if( ( p[2]^SN_MASKE )!=code[4] ) failed=0xff;
  if( ( p[1]^SN_MASKE )!=code[5] ) failed=0xff;
  if( ( p[0]^SN_MASKE )!=code[6] ) failed=0xff;
  return failed;
 }

void ds_check( void ) { /* nicht reentrant */
  static word index;
  static byte failed;
  static word nr, n;
  /*static byte * p;*/
  ;
  for( n=0; n<3; n++ ) { /* maximal 3 versuche ssn auszulesen */
    ds_get(); /* ssn in puffer code[] */
    for( nr=0; nr<VANZCODES; nr++ ) {
      failed=ds_compare( nr ); /* index der ssn in tabelle */
      if( !failed ) {
        return; /* code abgetestet und ok */
       }
     } 
   }
  /* kein gueltiger code vorhanden, meldung ausgeben */
  trigger_wd();
  init_d();
  trigger_wd();
  logslcd( T_SSN );
  while( 1 ) {
    disable();
    for( n=0; n<10000; n++ ) trigger_wd();
    #ifndef _MSC_VER
    #asm   
    rst 38h
    #endasm
    #endif
   } 
 }

