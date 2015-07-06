#include "types.h"
#include "logger.h"
#include "funcs.h"
#include "text.h"
#include "iomod.h"
#include "memtest.h"

#ifndef _MSC_VER
#asm
RAMSTART     equ 0d000h;     beginn fuer zyklischen ramtest, ende code bereich
RAMENDE_HIGH equ 0f0h;       wert in h fuer ende rambereich
#endasm
#endif

/*static*/void TestError( void ) { /* fehler im test festgestellt */
  #ifndef _MSC_VER
  #asm
  di    
  ;ld hl, TEXT    < hl wird vom caller geladen >
  push hl
  call logslcd
  pop hl    
  ld hl, 0
  push hl
  call system_stop
  pop hl
  ld hl, 1
  push hl
  call system_stop
  pop hl
shutdown:    
  di
  halt
  jp shutdown       
  ;
RamTestError:
  ld hl, T_RAM_ERR
  jr TestError
  ;   
RomTestError:
  ld hl, T_ROM_ERR
  jr TestError
  ;  
  #endasm         
  #endif
 }

/*static*/void RomCs( void ) { /* rom check sum berechnen */
  static char counter;
  ;
  #ifndef _MSC_VER
  #asm     
  ld hl, 0
  xor a
  ld b, 0
@page:  
  add a, ( hl )
  inc hl  
  djnz @page                             
  push af
  push hl;		save hl in x_short_sleep
  ld hl, 10
  push hl
  call x_short_sleep
  pop hl
  pop hl;		restore hl
  pop af
  ld b, 0
  ld c, a;		save accu in c
  ld a, h
  cp high RAMSTART
  jr z, RomEnde
  ld a, c;		restore accu  
  jr @page               
  ;
RomEnde:
  ld a, c
  or a
  jr nz, RomTestError  
  #endasm         
  #endif        
  ;
  if( ++counter>( get_maske()&0x0f ) ) {
    counter=0;
    if( !TextSum16OK() ) {
      #ifndef _MSC_VER
      #asm                      
      ld hl, T_TEXTCS
      push hl
      call logs
      pop hl
      di
      nop
      rst 38h 
      #endasm         
      #endif
     }
   }   
 }
                       
void Tmemtest( void ) {
  while( 1 ) {
    #ifndef _MSC_VER
    #asm   
    ;ramtest
    ld hl, RAMSTART
    ld b, 0
loop:
    ld a, h
    cp RAMENDE_HIGH
    jr nz, nowrap
wrap:
    call RomCs
    ld hl, RAMSTART
    ld b, 0
nowrap:         
    di
    ld a, ( hl )
    xor 0ffh
    ld ( hl ), a
    cp ( hl )
    jp nz, RamTestError
    xor 0ffh
    ld ( hl ), a
    cp ( hl )
    jp nz, RamTestError
    ei
    inc hl 
    djnz loop 
    push hl;	save hl in x_short_sleep
    ld hl, 10
    push hl        
    extern x_short_sleep
    call x_short_sleep
    pop hl 
    pop hl;		restore hl
    ld b, 0
    jr loop
    #endasm         
    #endif
   }
 }

#ifndef _MSC_VER
#asm        
  extern logs
T_TEXTCS: db 10, 13, 'nuc: code +?', 0  ; /* text segment aenderung erkannt */
#endasm
#endif                      


/* beim hochlauf mit logslcd perom 0 crc abtesten */

/*static*/void InitRomCs( void ) { /* rom check sum berechnen */
  #ifndef _MSC_VER
  #asm     
  ld hl, 0
  xor a
  ld b, 0
@ipage:  
  add a, ( hl )
  inc hl  
  djnz @ipage                       
  extern trigger_wd      
  call trigger_wd
  ld b, 0
  ld c, a;		save accu in c
  ld a, h
  cp high RAMSTART
  jr z, iRomEnde
  ld a, c;		restore accu  
  jr @ipage               
  ;
iRomEnde:
  ld a, c
  or a
  jr nz, iRomTestError  
  ;
iRomOk:
  ld hl, T_IROMOK
  push hl
  call logslcd  
  pop hl
  ret
iRomTestError:
  ld hl, T_IROMERR
  push hl
  call logslcd
  pop hl
  ret
  #endasm         
  #endif        
 }
 
 