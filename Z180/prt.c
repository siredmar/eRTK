#include "types.h"
#include "io.h"
#define PRT_SELF
#include "prt.h"

void init_prt( void ) {
  #ifndef _MSC_VER
  #asm
MHZ6
  IFDEF MHZ6
  title prt module for 6,144 MHz
TC0L equ 065h
TC0H equ 002h; 2msec
TC1L equ 0ffh
TC1H equ 005h; 5msec
  ENDIF
  ;
  IFDEF MHZ9 
  title prt module for 9,216 MHz
TC0L equ 097h
TC0H equ 003h ;2msec
TC1L equ 0ffh
TC1H equ 008h; 5msec
  ENDIF
 
        ;prt0 u. 1 frequenz einstellungen
        ;6,144 mhz
        ; 1 msec, tau=0307=0150h+1, l=50 h=01
        ; 2 msec, tau=0614=0265h+1, l=65 h=02
        ; 5 msec, tau=1536=05ffh+1, l=ff h=05
        ;10 msec, tau=3072=0bffh+1, l=ff h=0b
        ;9,216 mhz
        ; 1 msec, tau=0460=01cbh+1, l=cb h=01
        ; 2 msec, tau=0920=0397h+1, l=97 h=03
        ; 5 msec, tau=2304=08ffh+1, l=ff h=08
        ;10 msec, tau=4608=11ffh+1, l=ff h=11


        ld a,0
        out0 (tcr),a ;timer gestoppt

        ld a, TC0L
        out0 (rldr0l),a
        ld a, TC0H       ;zeitkonstante=614
        out0 (rldr0h),a  ;timer0 mit 2 msec zyklus fuer max182

        ld a, TC1L   
        out0 (rldr1l),a
        ld a, TC1H
        out0 (rldr1h),a  ;timer1 mit 5 msec zyklus fuer taskswitch

        ;Kontrollwort fuer Timer, Timer 0 starten u. Timer 1 starten
        ld a,00110011b
        out0 (tcr),a
        out0 (tcr),a

        ld hl, 0
        ld ( indexbuf0 ), hl
        ld ( indexbuf1 ), hl
        ld ( readbuf0 ), hl
        ld ( readbuf1 ), hl
        
        ld a, 0
        ld ( st ), a
        out ( ADU+0 ), a
        ret
  #endasm
  #endif
 }



#ifndef _MSC_VER
#asm
  extern akttask
  extern x_sefet
  extern x_wef_timer
  extern logmw
  extern frame
  public indexbuf0, indexbuf1
  public readbuf0, readbuf1
  public readstart0, readstart1
  public filt0, filt1
  public kette0, kette1

  
  
  dseg
indexbuf0:  ds 2
readbuf0:   ds 2
readstart0: ds 2
indexbuf1:  ds 2
readbuf1:   ds 2
readstart1: ds 2
st:         ds 1
kette0:     ds 2*15; word[15]
kette1:     ds 2*15; word[15]
  cseg
;
;ablauf: der analog mux braucht nach dem kanal umschalten mind. 1 bis 2
;messungen bis er den echten wert misst. darum werden wechselweise die kanaele
;erst mit 2 dummy messungen und dann 6 echten messungen abgescannt. die
;abtastrate nach fourier reduziert sich nach dem abstand zweier pakete 
;entsprechend 32msec auf 1/0,032 = 31Hz. der tiefpass im messverstaerker muss
;damit eine Fg von < 15Hz besitzen was hier gegeben ist.
;
; 100ms: 0000000000000000000000000000000000000000000000000011111111111111
;  10ms: 0000011111222223333344444555556666677777888889999900000111112222
;   1ms: 0246802468024680246802468024680246802468024680246802468024680246
;kanal0: --000000--......--000000--......--000000--......--000000--......
;kanal1: --......--111111--......--111111--......--111111--......--111111
;
;bei zyklus von 2ms fallen pro kanal 500*( 6/16 ) == 187 messwerte an
;bei pufferlaenge 2048 ist max. zeit 2048/187 == 10,95 ( s )
;0000 dummy
;0001 dummy
;0010 0
;0011 0
;0100 0
;0101 0
;0110 0
;0111 0
;1000 dummy
;1001 dummy
;1010 1
;1011 1
;1100 1
;1101 1
;1110 1
;1111 1

#endasm
#endif




/* prt0 macht max182 service */
void prt0( void ) {  /* Timerinterrupt 1msec */
  #ifndef _MSC_VER
  #asm
  di
  push af
  push bc
  push hl
  ;irq acknowledge
  in0 a,(tcr)
  in0 a,(tmdr0l)
  in0 a,(tmdr0h)		 
  ;
  ld hl, st
  ld a, ( hl )
  and 00000110b
  jr nz, work
  ;
dummy:  
  in a, ( ADU+4 )
  in a, ( ADU )
  bit 3, ( hl )
  jr nz, strt1
  ;
strt0:  
  out ( ADU ), a
  inc ( hl )
  pop hl
  pop bc
  pop af
  ei
  ret
  ;
strt1:
  out ( ADU+1 ), a
  inc ( hl )
  pop hl
  pop bc
  pop af
  ei
  ret
  ;
work:
  ld a, ( st )
  bit 3, a
  jp nz, work1
  ;
work0:
  extern PrtVMGetADU0
  push hl
  call PrtVMGetADU0
  pop hl
  inc ( hl ); inc st
  ld hl, frame; adubuf0
  ld bc, ( indexbuf0 )
  ld a, 00000111b
  and b
  ld b, a;     index laeuft von 0..2047
  add hl, bc
  add hl, bc; hl -> buffer
  inc bc
  ld ( indexbuf0 ), bc; inc indexbuf0
  ;hl -> messwertpuffer
  in0 b, ( ADU+4 )
  in0 c, ( ADU )
  INCFG0
  jp nz, nofilt0
  ;
filt0:
  ;bc=ungefilterter messwert
  ;filter
  push hl
  ld hl, ( kette0+13*2 )
  add hl, bc
  ld ( kette0+14*2 ), hl
  
  ld hl, ( kette0+12*2 )
  add hl, bc
  ld ( kette0+13*2 ), hl

  ld hl, ( kette0+ 11*2 )
  add hl, bc
  ld ( kette0+12*2 ), hl

  ld hl, ( kette0+10*2 )
  add hl, bc
  ld ( kette0+11*2 ), hl

  ld hl, ( kette0+ 9*2 )
  add hl, bc
  ld ( kette0+10*2 ), hl

  ld hl, ( kette0+ 8*2 )
  add hl, bc
  ld ( kette0+ 9*2 ), hl

  ld hl, ( kette0+ 7*2 )
  add hl, bc
  ld ( kette0+ 8*2 ), hl

  ld hl, ( kette0+ 6*2 )
  add hl, bc
  ld ( kette0+ 7*2 ), hl

  ld hl, ( kette0+ 5*2 )
  add hl, bc
  ld ( kette0+ 6*2 ), hl

  ld hl, ( kette0+ 4*2 )
  add hl, bc
  ld ( kette0+ 5*2 ), hl

  ld hl, ( kette0+ 3*2 )
  add hl, bc
  ld ( kette0+ 4*2 ), hl

  ld hl, ( kette0+ 2*2 )
  add hl, bc
  ld ( kette0+ 3*2 ), hl
  
  ld hl, ( kette0+ 1*2 )
  add hl, bc
  ld ( kette0+ 2*2 ), hl

  ld hl, ( kette0+ 0*2 )
  add hl, bc
  ld ( kette0+ 1*2 ), hl;   kette[1]=kette[0]

  ld ( kette0+ 0*2 ), bc;   kette[0]=inp
  ;
  ld hl, ( kette0+14*2 )
  add hl, bc
  ld c, l
  ld b, h
  ;
  ;division /16
  srl b
  rr c
  srl b
  rr c
  srl b
  rr c
  srl b
  rr c
  pop hl;                   hl -> messwertpuffer
  ;
nofilt0:
  ;nofilter
  ld ( hl ), c
  inc hl
  ld ( hl ), b
  out ( ADU ), a
  ;
  ;                               
  extern PrtVMFreeADU0
  push bc
  call PrtVMFreeADU0
  pop bc
  INCFG7; dip switch 7 auf 1
  jr nz, nolog0     
  ;
log0:  
  ;logger starten mit value in de                 
  push de
  ld d, b
  ld e, c
  call logmw
  pop de
  ;
nolog0:  
  pop hl
  pop bc
  pop af
  ei
  ret    
  ;
work1:
  extern PrtVMGetADU1
  push hl
  call PrtVMGetADU1
  pop hl
  inc ( hl ); inc st
  ld hl, frame; adubuf1
  ld bc, ( indexbuf1 )
  ld a, 00000111b
  and b
  ld b, a;     index laeuft von 0..2047
  add hl, bc
  add hl, bc; hl -> buffer
  inc bc
  ld ( indexbuf1 ), bc; inc indexbuf1
  in0 b, ( ADU+4 )
  in0 c, ( ADU )
  INCFG1
  jp nz, nofilt1
  ;
filt1:
  ;bc=ungefilterter messwert
  ;filter
  push hl
  ld hl, ( kette1+13*2 )
  add hl, bc
  ld ( kette1+14*2 ), hl
  
  ld hl, ( kette1+12*2 )
  add hl, bc
  ld ( kette1+13*2 ), hl

  ld hl, ( kette1+ 11*2 )
  add hl, bc
  ld ( kette1+12*2 ), hl

  ld hl, ( kette1+10*2 )
  add hl, bc
  ld ( kette1+11*2 ), hl

  ld hl, ( kette1+ 9*2 )
  add hl, bc
  ld ( kette1+10*2 ), hl

  ld hl, ( kette1+ 8*2 )
  add hl, bc
  ld ( kette1+ 9*2 ), hl

  ld hl, ( kette1+ 7*2 )
  add hl, bc
  ld ( kette1+ 8*2 ), hl

  ld hl, ( kette1+ 6*2 )
  add hl, bc
  ld ( kette1+ 7*2 ), hl

  ld hl, ( kette1+ 5*2 )
  add hl, bc
  ld ( kette1+ 6*2 ), hl

  ld hl, ( kette1+ 4*2 )
  add hl, bc
  ld ( kette1+ 5*2 ), hl

  ld hl, ( kette1+ 3*2 )
  add hl, bc
  ld ( kette1+ 4*2 ), hl

  ld hl, ( kette1+ 2*2 )
  add hl, bc
  ld ( kette1+ 3*2 ), hl
  
  ld hl, ( kette1+ 1*2 )
  add hl, bc
  ld ( kette1+ 2*2 ), hl

  ld hl, ( kette1+ 0*2 )
  add hl, bc
  ld ( kette1+ 1*2 ), hl;   kette[1]=kette[0]

  ld ( kette1+ 0*2 ), bc;   kette[0]=inp
  ;
  ld hl, ( kette1+14*2 )
  add hl, bc
  ld c, l
  ld b, h
  ;
  ;division /16
  srl b
  rr c
  srl b
  rr c
  srl b
  rr c
  srl b
  rr c
  pop hl;                   hl -> messwertpuffer
  ;
nofilt1:
  ;nofilter
  ld ( hl ), c
  inc hl
  ld ( hl ), b
  out ( ADU+1 ), a
  ;
  extern PrtVMFreeADU1
  push bc
  call PrtVMFreeADU1
  pop bc
  ;
  INCFG6; dip switch 6 auf 1
  jr nz, nolog1
  ;
log1:  
  ;logger starten mit value in de                 
  push de
  ld d, b
  ld e, c
  call logmw
  pop de
  ;
nolog1:  
  pop hl
  pop bc
  pop af
  ei
  ret    
  #endasm       
  #endif
 }


