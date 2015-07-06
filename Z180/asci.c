/**************************************************************************
 *     PROJEKT:          RTK180 V2.1                                      *
 **************************************************************************
 *                                                                        *
 *     FILENAME:  ASCI.C                                                  *
 *                                                                        *
 *     VARIANTE:                                                          *
 *                                                                        *
 *     VERSION:   2.1                                                     *
 *                                                                        *
 *     AUFGABE:   Treiber fuer Z180 asynchrone serielle Schnittstellen    *
 *                                                                        *
 *     SPRACHE:   C u. Z180 Assembler                                     *
 *                                                                        *
 *     DATUM:     24. Okt 1992                                            *
 *                                                                        *
 *     PROGRAMMIERER:         /te                                         *
 *                                                                        *
 **************************************************************************/

#include "types.h"
#include "io.h"     
#include "data.h"
#include "ascii.h"
#include "rtk180.h"

#ifndef _MSC_VER
#asm
MHZ6
;
         public init_asci
         public asci1
         public asci0
         public xstop

init_asci:
;Register der Schnittstelle 1 laden

        ld a,0
        ld hl,0
        ld (asci0_rxd_wpos),a
        ld (asci0_rxd_rpos),a
        ld (asci0_rxd_anz), a
        ld (asci0_txd_wpos),a
        ld (asci0_txd_rpos),a
        ld (asci1_rxd_wpos),a
        ld (asci1_rxd_rpos),a
        ld (asci1_rxd_anz), a
        ld (asci1_txd_wpos),a
        ld (asci1_txd_rpos),a
        ld ( xstop ), a
        ld (asci0_fe),hl
        ld (asci0_pe),hl
        ld (asci0_ov),hl
        ld (asci1_fe),hl
        ld (asci1_pe),hl
        ld (asci1_ov),hl
        ld ( asci0_r ), hl
        ld ( asci0_t ), hl
        ld ( asci1_r ), hl
        ld ( asci1_t ), hl

		;preemptive teil
		ld ( asci0_rxd_tcd_state ), hl
		ld ( asci0_rxd_nmax ), a
		ld ( asci0_txd_tcd_state ), hl
		ld ( asci1_rxd_tcd_state ), hl
		ld ( asci1_rxd_nmax ), a
		ld ( asci1_txd_tcd_state ), hl


  IFDEF MHZ6
  title asci module for 6,144 MHz
        ;asci0:9600,n,8,2
        ld a,00001000b
        out0 (stat0),a   ;rie, tie nicht
        ld a,01100101b
        out0 (cntla0),a  ;re, te, 8bit, no parity, 2 stopbit
        ld a,00000010b   ;9600Bd,00000001b=19200
        out0 (cntlb0),a  
 
        ;asci1:9600,e,8,2 fuer speicher
        ld a,00001000b
        out0 (stat1),a   ;rie, tie nicht
        ld a,01100111b
        out0 (cntla1),a  ;re, te, 8bit, parity, 2 stopbit
        ld a,00000010b
        out0 (cntlb1),a  ;even parity, 9600Bd
  ENDIF
  ;
  IFDEF MHZ9
  title asci module for 9,216 MHz
        ;asci0:9600,n,8,2
        ld a,00001000b
        out0 (stat0),a   ;rie, tie nicht
        ld a,01100101b
        out0 (cntla0),a  ;re, te, 8bit, no parity, 2 stopbit
        ld a,00100001b   ;9600Bd,00000000b=19200
        out0 (cntlb0),a  
 
        ;asci1:2400,e,8,2
        ld a,00001000b
        out0 (stat1),a   ;rie, tie nicht
        ld a,01100111b
        out0 (cntla1),a  ;re, te, 8bit, parity, 2 stopbit
        ld a,00100011b
        out0 (cntlb1),a  ;even parity, 2400Bd
  ENDIF
        ret


;/* vom asci0 interrupt angestossene Bedienroutine */
asci0:
	di
	push hl
	push af
    push bc

	in0 a,(stat0)
	and 01110000b
	jr z,noerror0   	;Schnittstellenfehler ?

	bit 6,a
	jr z,err10		;Overrun
	ld hl,(asci0_ov)
	inc hl
	ld (asci0_ov),hl
    in0 h,(rdr0)            ;defektes zeichen auslesen u. verwerfen

err10:  bit 5,a			;parity
	jr z,err20
	ld hl,(asci0_pe)
	inc hl
	ld (asci0_pe),hl
    in0 h,(rdr0)

err20:  bit 4,a                 ;frame
	jr z,err30
	ld hl,(asci0_fe)
	inc hl
	ld (asci0_fe),hl
    in0 h,(rdr0)

err30:  in0 a,(rdr0)		;zeichen auslesen
	in0 a,(cntla0)
	and 11110111b
	out0 (cntla0),a		;reset error flag
	jp over_asci0

noerror0:
    in0 a,(stat0)
    bit 7,a                 ;receive data reg. full
    jr nz,asci0_rxd         ;wenn bit7=1 springe nach asci0_rxd

    bit 1,a                 ;transmit data reg. empty
    jp nz,asci0_txd         ;wenn bit1=1 springe nach asci0_txd


asci0_rxd:
    ld hl, asci0_rxd_anz
    inc ( hl );             inc rx counter
    ld hl, asci0_r
    inc ( hl )
    ld hl,asci0_rxd_buf     ;Anfangsadresse Empfangspuffer  xx00H
    ld a,(asci0_rxd_wpos)   ;Eintragszeiger lesen
    ld l,a                  ;Element adressieren HL -> Puffer
    inc a
    ld (asci0_rxd_wpos),a   ;Eintragszeiger inkrementieren
    in0 a,(rdr0)
    jp noxoff; kein protokoll fahren
    cp XOFF
    jr nz, noxoff
rec_xoff:
    ld a, 1
    ld ( xstop ), a
    ;ld a, 'S'
    ;ld ( display ), a
    in0 a,(stat0)
    and 11111110b
    out0 (stat0),a         ;sendeinterrupt sperren
    jr over_asci0
noxoff:
    jp noxon
    cp XON
    jr nz, noxon
rec_xon:
    ld a, 0
    ld ( xstop ), a
    ;ld a, 'Q'
    ;ld ( display ), a
    in0 a,(stat0)
    or 00000001b
    out0 (stat0),a         ;sendeinterrupt freigeben
    jr over_asci0
noxon:
    ld (hl),a

	;preemptive system
	extern asci0_rxd_nmax, asci0_rxd_tcd_state
	ld a, ( asci0_rxd_wpos )
	ld hl, asci0_rxd_rpos
	sub ( hl );				a=+-zeichenanzahl
	bit 7, a
	jr z, asci0_positive
	neg;					a=abs( zeichenanzahl )
asci0_positive:
	ld hl, asci0_rxd_nmax;	hl -> anzahl einzulesender zeichen
	cp ( hl );
	jr c, asci0_rxd_cont;	nmax nicht erreicht
asci0_rxd_start:;				nmax erreicht, task starten
	ld hl, ( asci0_rxd_tcd_state );pointer auf tcd.state oder NULL
	ld a, h
	or l
	jr z, asci0_rxd_no_tcd;	keine task wartet 
	ld a, READY
	ld ( hl ), a;			task state auf ready
	;tcd.state pointer auf NULL
	ld hl, 0
	ld ( asci0_rxd_tcd_state ), hl
asci0_rxd_no_tcd:
asci0_rxd_cont:
	;ende preemptive system
    jr over_asci0

asci0_txd:
    ld hl, asci0_t
    inc ( hl )
    ld a,(asci0_txd_wpos)
    ld hl,(asci0_txd_rpos)
    cp l
    jr z,bufferleer0         ;wenn wpos=rpos->buffer leer
    ld hl,asci0_txd_buf     ;anfangsadresse sendepuffer  xx00h
    ld a,(asci0_txd_rpos)   ;austragszeiger lesen
    ld l,a                  ;elememt adressieren HL -> puffer
    inc a
    ld (asci0_txd_rpos),a   ;  austragszeiger inkrementieren
    ld a,(hl)
    out0 (tdr0),a           ;Austragsregister asci0 auslesen
    jr over_asci0

bufferleer0:
    in0 a,(stat0)
    and 11111110b           ;transmit interrupt enable=0
    out0 (stat0),a          ;TIE reset

	;preemptive teil
	extern asci0_txd_tcd_state
	ld hl, ( asci0_txd_tcd_state )
	ld a, h
    or l
    jr z, over_asci0
	ld a, READY
	ld ( hl ), a;			task ready
	ld hl, 0
	ld ( asci0_txd_tcd_state ), hl 

over_asci0:
    pop bc
   	pop af
    pop hl
    ei
    reti			;Return from interrupt mit enable intr. !!!





;/* vom asci1 interrupt angestossene Bedienroutine */
asci1:
	di
	push hl
	push af

	in0 a,(stat1)
	and 01110000b
	jr z,noerror1		;Schnittstellenfehler ?

	bit 6,a
	jr z,err11		;Overrun
	ld hl,(asci1_ov)
	inc hl
	ld (asci1_ov),hl
    in0 h,(rdr1)            ;defektes zeichen auslesen u. verwerfen

err11:  bit 5,a			;parity
	jr z,err21
	ld hl,(asci1_pe)
	inc hl
	ld (asci1_pe),hl
    in0 h,(rdr1)

err21:  bit 4,a                 ;frame
	jr z,err31
	ld hl,(asci1_fe)
	inc hl
	ld (asci1_fe),hl
    in0 h,(rdr1)

err31:  in0 a,(rdr1)		;zeichen auslesen
	in0 a,(cntla1)
	and 11110111b
	out0 (cntla1),a		;reset error flag
	jp over_asci1

noerror1:
    in0 a,(stat1)
    bit 7,a                 ;receive data reg. full
    jr nz,asci1_rxd         ;wenn bit7=1 springe nach asci1_rxd

    bit 1,a                 ;transmit data reg. empty
    jp nz,asci1_txd         ;wenn bit1=1 springe nach asci1_txd


asci1_rxd:
    ld hl, asci1_rxd_anz
    inc ( hl );             inc rx counter
    ld hl, asci1_r
    inc ( hl )
    ld hl,asci1_rxd_buf     ;Anfangsadresse Empfangspuffer  xx00H
    ld a,(asci1_rxd_wpos)   ;Eintragszeiger lesen
    ld l,a                  ;Element adressieren HL -> Puffer
    inc a
    ld (asci1_rxd_wpos),a   ;Eintragszeiger inkrementieren
    in0 a,(rdr1)
    ld (hl),a

	;preemptive system
	extern asci1_rxd_nmax, asci1_rxd_tcd_state
	ld a, ( asci1_rxd_wpos )
	ld hl, asci1_rxd_rpos
	sub ( hl );				a=+-zeichenanzahl
	bit 7, a
	jr z, asci1_positive
	neg;					a=abs( zeichenanzahl )
asci1_positive:
	ld hl, asci1_rxd_nmax;	hl -> anzahl einzulesender zeichen
	cp ( hl );
	jr c, asci1_rxd_cont;	nmax nicht erreicht
asci1_rxd_start:;				nmax erreicht, task starten
	ld hl, ( asci1_rxd_tcd_state );pointer auf tcd.state oder NULL
	ld a, h
	or l
	jr z, asci1_rxd_no_tcd;	keine task wartet 
	ld a, READY
	ld ( hl ), a;			task state auf ready
	;tcd.state pointer auf NULL
	ld hl, 0
	ld ( asci1_rxd_tcd_state ), hl
asci1_rxd_no_tcd:
asci1_rxd_cont:
	;ende preemptive system

    jr over_asci1

asci1_txd:
    ld hl, asci1_t
    inc ( hl )
    ld a,(asci1_txd_wpos)
    ld hl,(asci1_txd_rpos)
    cp l
    jr z,bufferleer1         ;wenn wpos=rpos->buffer leer
    ld hl,asci1_txd_buf     ;anfangsadresse sendepuffer  xx00h
    ld a,(asci1_txd_rpos)   ;austragszeiger lesen
    ld l,a                  ;elememt adressieren HL -> puffer
    inc a
    ld (asci1_txd_rpos),a   ;  austragszeiger inkrementieren
    ld a,(hl)
    out0 (tdr1),a           ;Austragsregister asci1 auslesen
    jr over_asci1

bufferleer1:
    in0 a,(stat1)
    and 11111110b           ;transmit interrupt enable=0
    out0 (stat1),a          ;TIE reset

	;preemptive teil
	extern asci1_txd_tcd_state
	ld hl, ( asci1_txd_tcd_state )
	ld a, h
    or l
    jr z, over_asci1
	ld a, READY
	ld ( hl ), a;			task ready
	ld hl, 0
	ld ( asci1_txd_tcd_state ), hl 
over_asci1:
   	pop af
	pop hl
	ei
	reti			;Return from interrupt mit enable intr. !!!

        dseg
xstop:     ds 2

  public asci0_r, asci0_t, asci1_r, asci1_t
asci0_r: ds 2
asci0_t: ds 2
asci1_r: ds 2
asci1_t: ds 2

        end
#endasm
#endif
