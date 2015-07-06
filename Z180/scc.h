
/* 
; ***** 85C30 definitionen *****
; es sind zwei kanaele a und b vorhanden.
; der scc hat 15 write register zu konfiguration von denen mehrere zurueckgelesen
; werden koennen sowie 1 datenregister pro kanal
; der zugriff auf write register x ( wrx ) erfolgt durch indizierung:
;	1. write x auf control register
;	2. 8 phi clocks warten ( laufzeit opcodes out, load, out reicht ! )
;	3. write wert fuer x auf control register
*/

/*
; ***** SCCZ85C30 hardware adressen *****
*/
#define DA 0x83
#define DB 0x82
#define CA 0x81
#define CB 0x80

#ifndef _MSC_VER
#asm
DA equ 83h;	datenregister kanal a
DB equ 82h; datenregister kanal b
CA equ 81h; control register kanal a
CB equ 80h; control register kanal b
#endasm
#endif

#ifndef _MSC_VER
#asm
; ***** Speziell RS485 mit Richtungsumschaltung Transceiver ueber RTS *****
; Die Funktion auto enables bedeutet, dass ueber CTS_Aktiv das Senden freigegeben wird
; und mit dem letzten gesendeten Zeichen nach dem Transmit Shift Register das Signal
; RTS wieder inaktiv wird. Kann hier nicht verwendet werden, da CTS auf high liegt !
; Wenn daher RS485 mit Transmitter Freischaltung ueber RTS benutzt wird, schaltet der
; Treiber beim letzten Tx_Int wenn Tx Puffer leer ist das Signal RTS inaktiv=hi 
; und das letzte Zeichen geht  N I C H T  mehr auf die Leitung.
; Also  M U S S  bei RS485 immer ein dummy char beim Senden angefuegt werden, der von dem
; inaktiven RS485 Transmitter abgeblockt wird !
;
; *** Version 3.1 ***
; hier ist cts/dcd auf active low gelegt und auto enables kann eingeschaltet werden !
;
; ***** SCCZ85C30 konfiguration *****
; *** Bitrate bei phi=6,144 Meg, ( wr13=0 )
;	0008t=19200bd -> wr12=00001000b
;	0018t= 9600bd -> wr12=00010010b
;	0038t= 4800bd -> wr12=00100110b
;   0078t= 2400bd -> wr12=01001110b
; *** paritaet
;   parity on   -> wr4(0) = 1
;   parity off  -> wr4(0) = 0
;   parity even -> wr4(1) = 1
;   parity odd  -> wr4(1) = 0
; *** datenbitanzahl
;   senden -> wr5(6,5)
;	  	00 	-> 	5 bits
;	  	10 	-> 	6 bits
;	  	01	-> 	7 bits
;		11	->	8 bits
;	empfangen -> wr3(7,6)
;		00	->	5 bits
;		10	->	6 bits
;		01	->	7 bits
;		11	->	8 bits
; *** stopbitanzahl	wr4(3,2)
;		11	->	2 stop bits
;		10	->	1,5 stop bits
;		01	->	1 stop bit
;		00	->	synchron mode
;
WR1A  equ 00010100b; int on all rx or special, parity is special, all ints off
WR1B  equ 00010100b; int on all rx or special, parity is special, all ints off
WR3A  equ 11100001b; rx 8bits, rx enable, auto enables on
WR3B  equ 11100001b; rx 8bits, rx enable, auto enables on
WR4A  equ 01000100b; clk/16, 1sbits, no parity
WR4B  equ 01000100b; clk/16, 1sbits, no parity
WR5A  equ 01101000b; dtr off, txd 8bit, tx enable
WR5B  equ 01101000b; dtr off, txd 8bit, tx enable
WR11A equ 01010010b; no xtal, rxclk=brgen, txclk=brgen, trxc=inp, trxc=brgen
WR11B equ 01010010b; no xtal, rxclk=brgen, txclk=brgen, trxc=inp, trxc=brgen
WR12A equ 00010010b; tc_lo = 12h = 18t -> 9600bd
WR12B equ 00010010b; tc_lo = 12h = 18t -> 9600bd
WR13A equ 00000000b; tc_hi = 0
WR13B equ 00000000b; tc_hi = 0
WR14A equ 00000011b; brgen is source, brgen enable
WR14B equ 00000011b; brgen is source, brgen enable
#endasm
#endif

#ifndef SCC_SELF
extern s_tcd * scc_a_tcd_ptr;
extern s_tcd * scc_b_tcd_ptr;
extern byte scc_a_rxd_anz, scc_b_rxd_anz;
byte SCCFound( void );
#endif
