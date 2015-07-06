#include "types.h"
#define SCC_SELF
#include "scc.h"
#include "rtk180.h"
#include "data.h"

#ifndef _MSC_VER
#asm
       public SCC_init, int2
       public i_a_rxd, i_a_txd, i_a_special, i_a_external
;
       
;************* SCC DATENBEREICH ************************************

scc_table_len equ 30+19
scc_table:              
        ; port wr  wr inhalt ( wr=write register )
        db CA,  1, 0
        db CA,  2, 0
        db CA,  3, 0
        db CA,  4, 0
        db CA,  5, 0
        db CA,  6, 0
        db CA,  7, 0
        db CA,  8, 0
        db CA,  9, 0
        db CA, 10, 0
        db CA, 11, 0
        db CA, 12, 0
        db CA, 13, 0
        db CA, 14, 0
        db CA, 15, 0
        db CB,  1, 0
        db CB,  2, 0
        db CB,  3, 0
        db CB,  4, 0
        db CB,  5, 0
        db CB,  6, 0
        db CB,  7, 0
        db CB,  8, 0
        db CB,  9, 0
        db CB, 10, 0
        db CB, 11, 0
        db CB, 12, 0
        db CB, 13, 0
        db CB, 14, 0
        db CB, 15, 0
        ;
		db CA,  9, 11000000b; force hardware reset
		db CB,  9, 11000000b; force hardware reset
        db CB,  4, WR4B
        db CA,  4, WR4A
        db CB,  3, WR3B
        db CA,  3, WR3A
        db CB,  5, WR5B
        db CA,  5, WR5A
        db CB, 12, WR12B
        db CA, 12, WR12A
        db CB, 13, WR13B
        db CA, 13, WR13A
        db CB, 11, WR11B
        db CA, 11, WR11A
        db CB, 14, WR14B
        db CA, 14, WR14A
        db CB,  1, WR1B
        db CA,  1, WR1A
        db CB,  9, 00101010b;  enable master int, soft intack enable


SCC_init:
        ;grundzustand scc
        xor a
        out ( CA ), a
        ld a, 00110000b; error reset
        out ( CA ), a
        xor a
        out ( CB ), a
        ld a, 00110000b; error reset
        out ( CB ), a
        ;
        ;prog modes  
        ld a, scc_table_len
        ld l, a
        ld ix, scc_table
        ;
prog_scc_mode:
        ld c, ( ix+0 )
        ld b, 0
        ld a, ( ix+1 )
        out ( c ), a
        ld a, ( ix+2 )
        out ( c ), a
        inc ix
        inc ix
        inc ix 
        dec l
        jr nz, prog_scc_mode
        ;
        ld a,0
        ld (scc_a_rxd_wpos),a
        ld (scc_a_rxd_rpos),a
        ld (scc_a_txd_wpos),a
        ld (scc_a_txd_rpos),a
        ld (scc_b_rxd_wpos),a
        ld (scc_b_rxd_rpos),a
        ld (scc_b_txd_wpos),a
        ld (scc_b_txd_rpos),a        
        ;
        ld ( scc_a_rxd_anz ), a 
        ld ( scc_b_rxd_anz ), a
        ;
        ;
        ld hl,0
        ld (scc_a_fe),hl
        ld (scc_a_pe),hl
        ld (scc_a_ov),hl
        ld (scc_b_fe),hl
        ld (scc_b_pe),hl
        ld (scc_b_ov),hl
        ;
        ld ( scc_a_tcd_ptr ), hl
        ld ( scc_b_tcd_ptr ), hl
        ;
        ret

; ***** ankopplung an rtk180 echtzeitsystem *****
	DSEG	
task_ready:		ds 1;	wenn ein i/o auftrag abgearbeitet ist wird flag gesetzt fuer scheduling

;pointer auf tcd der aktiven task
	public scc_a_tcd_ptr, scc_b_tcd_ptr
scc_a_tcd_ptr:	ds 2     	
scc_b_tcd_ptr:	ds 2
		
;merker fuer einzulesende zeichenanzahl im kanal
   	public scc_a_rxd_anz, scc_b_rxd_anz
scc_a_rxd_anz:	ds 1	;nur wenn !=0 dann ist einlesen aktiv
scc_b_rxd_anz:	ds 1


	CSEG  
;call aus int2 mit af, hl, bc, ix registern frei, terminierung durch normalen ret !	
scc_a_txd_ready:; zeichen alle ausgegeben
  public scc_a_txd_ready
  ;1. check for valid tcd pointer
  ld hl, ( scc_a_tcd_ptr )
  ld a, h
  or l
  ret z;	das war ein schwerer fehler !
  ;2. laufenden systimer loeschen
  ld ix, ( scc_a_tcd_ptr )
  ld a, ( ix+OFFSET_ID )
  ld hl, systimer
  add a, l           
  ld l, a
  jr nc, inpage_a
  inc h
inpage_a:
  ld a, 0
  ld ( hl ), a  
  ;3. tcd.state auf ready setzen
  ld a, READY
  ld ( ix+OFFSET_STATE ), a
  ;4. kill tcd pointer     
  xor a
  ld ( scc_a_tcd_ptr ), a
  ld ( scc_a_tcd_ptr+1 ), a   
  dec a
  ld ( task_ready ), a
  ret

scc_b_txd_ready:; zeichen alle ausgegeben
  public scc_b_txd_ready
  ;1. check for valid tcd pointer
  ld hl, ( scc_b_tcd_ptr )
  ld a, h
  or l
  ret z;	das war ein schwerer fehler !
  ;2. laufenden systimer loeschen
  ld ix, ( scc_b_tcd_ptr )
  ld a, ( ix+OFFSET_ID )
  ld hl, systimer
  add a, l
  ld l, a
  jr nc, inpage_b
  inc h
inpage_b:
  ld a, 0
  ld ( hl ), a  
  ;3. tcd.state auf ready setzen
  ld a, READY
  ld ( ix+OFFSET_STATE ), a
  ;4. kill tcd pointer     
  xor a
  ld ( scc_b_tcd_ptr ), a
  ld ( scc_b_tcd_ptr+1 ), a
  dec a
  ld ( task_ready ), a
  ret        

scc_a_rxd_ready:; alle zeichen eingelesen
  public scc_a_rxd_ready
  jr scc_a_txd_ready
  ret

scc_b_rxd_ready:; alle zeichen eingelesen
  public scc_b_rxd_ready
  jr scc_b_txd_ready
  ret


				
int2:;					*** SCC IRQ ***
         ;di ist automatisch gegeben
         push af
         push hl
         push bc;		c sichert statusbyte !
         push ix
         ;      
         xor a
         ld ( task_ready ), a
         ;
         ;software intack, auslesen int quelle
         ld a, 2;		
         out ( CB ),a
         nop
         in a, ( CB );	read register 2 auslesen um Interruptquelle festzustellen
         ;and 00001110b
         ;
scc_next:;				read int pending register until all flags off
		 ld a, 3
		 out ( CA ), a
		 nop
		 in a, ( CA )         
         ;
         bit 5, a
         jr z,l1
         call I_A_RxD          
         jr scc_next
         ;
l1:      bit 4, a
         jr z,l2
         call I_A_TxD
         jr scc_next
         ;
l2:      bit 3, a
         jr z,l3
         call I_A_EXTERNAL
         jr scc_next
         ;
l3:      bit 2, a
         jr z,l5
         call I_B_RxD
         jr scc_next
         ;
l5:      bit 1, a
         jr z,l6
         call I_B_TxD
         jr scc_next
         ;
l6:      bit 0, a
         jr z, scc_done
         call I_B_EXTERNAL
         jr scc_next
         ;
scc_done:;alle pending ints abgearbeitet
         ld a, 9
         out ( CA ), a
         ld a, 00101010b;  enable master int u. soft intack
         out ( CA ), a
         ;   
         ld a, ( task_ready )
         or a
         jr z, no_Sh
         extern x_scheduler
         call x_scheduler
no_sh:          
         pop ix
         pop bc
         pop hl
         pop af
         ei
         ret


I_A_RxD:                       		;zeichen nur in buffer schreiben
  public I_A_RxD
         ld hl, scc_a_rxd_buf  		;bufferadresse laden
         ld a, ( scc_a_rxd_wpos ) 
         ld l, a               		;buffer offset addiert
         inc a
         ld ( scc_a_rxd_wpos ), a 	;eintragszeiger inkrementiert
         in a, ( DA )            	;zeichen einlesen
         ld ( hl ), a             	;und in buffer schreiben
         ;
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ;           
         ;wird aktiv gelesen ?
         ld a, ( scc_a_rxd_anz )
         or a
         ret z
         ;einlesen aktiv: berechne anz im rx buffer
         ld a, ( scc_a_rxd_wpos )
         ld hl, scc_a_rxd_rpos
         sub ( hl )         
         ld l, a  
         ;l=anzahl zeichen im puffer
         ld a, ( scc_a_rxd_anz )
         cp l;	soll-ist, entweder identisch oder mehr als noetig eingelesen
         jp c, scc_a_rxd_ready;	dort geht es mit ret zurueck !
         jp z, scc_a_rxd_ready
         ret;					noch nicht genug zeichen da 

I_B_RxD:                       ;zeichen nur in buffer schreiben
  public I_B_RxD
         ld hl, scc_b_rxd_buf  ;bufferadresse laden
         ld a, ( scc_b_rxd_wpos )
         ld l, a               ;buffer offset addiert
         inc a
         ld ( scc_b_rxd_wpos ), a ;eintragszeiger incrementieren
         in a, ( DB )           ;zeichen einlesen
         ld ( hl ), a            ;und in buffer schreiben
         ;      
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ;
         ;wird aktiv gelesen ?
         ld a, ( scc_b_rxd_anz )
         or a
         ret z
         ;     
         ;einlesen aktiv: berechne anz im rx buffer
         ld a, ( scc_b_rxd_wpos )
         ld hl, scc_b_rxd_rpos
         sub ( hl )
         ld l, a
         ;l=anzahl zeichen im puffer
         ld a, ( scc_b_rxd_anz )
         cp l;	soll-ist, entweder identisch oder mehr als noetig eingelesen
         jp c, scc_b_rxd_ready;	dort geht es mit ret zurueck !
         jp z, scc_b_rxd_ready
         ret;					noch nicht genug zeichen da 


I_A_TxD:                          ;sendepuffer ausgeben bis er leer ist
  public I_A_TxD
         ld a,(scc_a_txd_rpos)
         ld hl,scc_a_txd_wpos
         cp (hl)
         jp z,scc_a_kein_zeichen  ;alle zeichen bereits ausgegeben ?
         inc a
         ld (scc_a_txd_rpos),a    ;austragszeiger erhoehen
         dec a
         ld hl,scc_a_txd_buf
         ld l,a                   ;pufferelement adressieren
         ld a,(hl)
         out (DA),a              ;zeichen ausgeben
         ;
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret
         ;
scc_a_kein_zeichen:
         ld a, 00101000b ;reset Tx INT pending Kommando an ch A
         out ( CA ), a
         ; reset rts signal
         ld a, 5
         out ( CA ), a
         ld a, WR5A
         and 11111101b; rts=0
         out ( CA ), a     
         ;hier muss unbedingt res hi ius, sonst ist sender irq gesperrt !!!
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         jp scc_a_txd_ready;		dort geht es mit ret zurueck
         ;ret

I_B_TxD:                         ;sendepuffer ausgeben bis er leer ist
  public I_B_TxD
         ld a,(scc_b_txd_rpos)
         ld hl,scc_b_txd_wpos
         cp (hl)
         jp z,scc_b_kein_zeichen  ;alle zeichen ausgegeben?
         inc a
         ld (scc_b_txd_rpos),a    ;austragszeiger erhoehen
         dec a  
         ld hl,scc_b_txd_buf
         ld l,a                   ;pufferelement adressieren
         ld a,(hl)
         out (DB),a              ;zeichen ausgeben
         ;
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret
		 ;
scc_b_kein_zeichen:
         ld a, 00101000b ;reset Tx INT pending Kommando an ch B
         out ( CB ), a
         ; reset rts signal
         ld a, 5
         out ( CB ), a
         ld a, WR5B
         and 11111101b; rts=0
         out ( CB ), a 
         ;hier muss unbedingt res hi ius, sonst ist sender irq gesperrt !!!
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         jp scc_b_txd_ready;		dort geht es mit ret zurueck
         ;ret


I_A_EXTERNAL:   
  public I_A_EXTERNAL
         ld a, 00010000b ;reset ext INT Kommando an ch A
         out ( CA ),a
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret

I_A_SPECIAL:         
  public I_A_SPECIAL
         ld a, 00110000b
         out ( CA ),a
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret

I_B_EXTERNAL:       
  public I_B_EXTERNAL
         ld a, 00010000b ;reset ext INT Kommando an ch A
         out ( CB ),a
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret

I_B_SPECIAL:         
  public I_B_SPECIAL
         ld a, 00110000b
         out ( CB ),a
         ld a, 00111000b; reset highest ius
         out ( CA ), a
         ret

#endasm
#endif

byte SCCFound( void ) { /* check ob der scc vorhanden ist */
  #ifndef _MSC_VER
  #asm  
  ld c, 12; adresse time constant register low
  ;lade alten inhalt nach b
  out0 ( CA ), c
  nop
  in0 b, ( CA )
  ;mache rw test
  ld a, 55h
  out0 ( CA ), c
  nop
  out0 ( CA ), a
  nop
  out0 ( CA ), c
  nop
  in0 l, ( CA )
  cp l
  jr nz, failed
  ld a, 0aah
  out0 ( CA ), c
  nop
  out0 ( CA ), a
  nop
  out0 ( CA ), c
  nop
  in0 l, ( CA )
  cp l
  jr nz, failed
  ;restauriere alten inhalt
  out0 ( CA ), c
  nop
  out0 ( CA ), b
  ld hl, -1; scc vorhanden
  ret
failed:    
  ld hl, 0
  ret
  #endasm     
  #endif  
  return 0;
 }
 
/*#define DIAGNOSTIC*/
#ifdef DIAGNOSTIC
char regsa[16];
char regsb[16];
SCCDump() { 
  static char n;
  ;   
  disable();
  for( n=0; n<16; n++ ) {
    outbyte( CA, n );
    regsa[n]=inbyte( CA );
    outbyte( CB, n );
    regsb[n]=inbyte( CB );
   } 
  enable();
 }
#endif 
