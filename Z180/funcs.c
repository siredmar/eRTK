#define FUNCS_SELF
#include "types.h"
#include "io.h"
#include "iomod.h"
#include "rtk180.h" 
#include "sema.h"
#include "funcs.h"
#include "data.h"


char display[DISMAX+1];
byte lcd_hochlauf;
const byte ASCII[16] = "0123456789ABCDEF";

static char * bildp[4];
static byte save_dis[DISMAX];

void enable( void ) {
  #ifndef _MSC_VER
  #asm
  ei
  #endasm
  #endif
 }


void disable( void ) {
  #ifndef _MSC_VER
  #asm
  di
  #endasm
  #endif
 }

void halt( void ) {
  #ifndef _MSC_VER
  #asm
  halt
  #endasm
  #endif
 }

/* fuer die lcd die wait states auf 2 setzen */
#ifndef _MSC_VER
#asm
TwoWaits macro
  in0 a, ( DCNTL )
  or 00010000b
  out0 ( DCNTL ), a
  endm
  
OneWait macro
  in0 a, ( DCNTL )
  and 11001111b
  out0 ( DCNTL ), a
  endm
#endasm
#endif

void init_d( void ) { /*Display initialisieren*/
  #ifndef _MSC_VER
  #asm           
    TwoWaits
    out0 ( PWATCHDOG ), a
    call wait
    out0 ( PWATCHDOG ), a
    ld a,00111100b  ;function set: 8 bit, 2 lines, 5x10 dots
    out0 (0c0h),a
    call wait
    out0 ( PWATCHDOG ), a
    ld a,00000001b  ;clear display
    out0 (0c0h),a
    call wait
    out0 ( PWATCHDOG ), a
    ld a,00001100b  ;display on, cursor off, cursor blink off
    out0 (0c0h),a
    call wait
    out0 ( PWATCHDOG ), a
    ld a,00000110b
    out0 (0c0h),a   ;entry mode set: increment, no shift
    call wait
    out0 ( PWATCHDOG ), a
    ld a,10000000b
    out0 (0c0h),a
    OneWait
    ret
  #endasm
  #endif
 }

static void wait( void ) {
  #ifndef _MSC_VER
  #asm
    push af
_wait:
    in0 a,(0c0h)
    and 80h
    jr nz,_wait
    pop af
    ret
  #endasm
  #endif
 }

void putchar( byte c ) { /* Charakter ausgeben */
  #ifndef _MSC_VER
  #asm
    ld hl,2
    add hl,sp
    call wait
    TwoWaits
    ld a,(hl)
    out0 (d_disp),a
    OneWait
  #endasm
  #endif
 }


#ifndef _MSC_VER
#asm
LCD_SCAN equ 5
#endasm
#endif

void LCDRefresher( void ) {
    #ifndef _MSC_VER
    #asm

    ;die 4 lcd zeilen werden nacheinander mit dem vergleichspuffer
    ;auf veraenderungen abgesucht. ist auch nur ein zeichen anders
    ;wird die komplette zeile neu ausgeschrieben ( ca. 1,4 msec )
    ;nach jedem zeilen test wird x_sleep( LCD_SCAN ) aufgerufen um
    ;nicht zu viel rechenleistung zu verbrauchen. die worst case
    ;laufzeit fuer eine komplette bilderneuerung errechnet sich wie folgt:
    ;    anztasks = 6
    ;    pause call < 6 msec
    ;    eine zeile 1,4 msec + LCD_SCAN * 6 msec
    ;    gesamt 4 * ( 1,4 + 6 * LCD_SCAN ) msec
    ;    LCD_SCAN = 5 -> laufzeit < 125 msec fuer ein komplettes bild 

start_line0:
    ld c, LCD_SCAN;      scan frequenz errechnen
    ld b, 0
    push bc
    call x_short_sleep;          taskwechsel erzwingen
    pop bc
;                        zeile 0 bearbeiten
line0:;                  laufzeit pro zeile 1,4 msec bei 6MHz
    ld hl, ( bildp )
    ld de, save_dis
    ld b, 20
test0:
    ld a, ( de )
    cp ( hl )
    jr nz, fill0;        zeile 0 muss erneuert werden
    inc hl
    inc de
    djnz test0
    jr line1;            zeile 0 ist auf neuestem stand
fill0:;                  zeile 0 erneuern
    ld hl, ( bildp )
    ld de, save_dis
    ld b, 20
wait0:
    in0 a,(0c0h)
    and 80h
    jr nz, wait0;        warte auf lcd
    di
    TwoWaits
    ld a, 2
    out0 ( 0c0h ), a;    cursor home
    OneWait
    ei
ffill0:
cont0:
    in0 a,(0c0h)
    and 80h
    jr nz, cont0;        warten auf lcd
    di
    TwoWaits
    ld a, ( hl )
    out0 ( 0c1h ), a;    zeichen ausgeben
    ld ( de ), a;        vergleichspuffer erneuern
    OneWait
    ei
    inc hl
    inc de
    djnz ffill0;         20 mal da 20 zeichen pro zeile
    ;
start_zeile1:
    ld c, LCD_SCAN
    ld b, 0
    push bc
    call x_short_sleep
    pop bc

line1:
    ld hl, ( bildp+2 )
    ld de, save_dis+20
    ld b, 20
test1:
    ld a, ( de )
    cp ( hl )
    jr nz, fill1
    inc hl
    inc de
    djnz test1
    jr line2
fill1:
    ld hl, ( bildp+2 )
    ld de, save_dis+20
    ld b, 20
wait1:
    in0 a,(0c0h)
    and 80h
    jr nz, wait1
    di
    TwoWaits
    ld a, 10000000b+40
    out0 ( 0c0h ), a; goto 40
    OneWait
    ei
ffill1:
cont1:
    in0 a,(0c0h)
    and 80h
    jr nz, cont1
    di
    TwoWaits
    ld a, ( hl )
    out0 ( 0c1h ), a
    ld ( de ), a
    OneWait
    ei
    inc hl
    inc de
    djnz ffill1

start_zeile2:
    ld c, LCD_SCAN
    ld b, 0
    push bc
    call x_short_sleep
    pop bc

line2:
    ld hl, ( bildp+4 )
    ld de, save_dis+40
    ld b, 20
test2:
    ld a, ( de )
    cp ( hl )
    jr nz, fill2
    inc hl
    inc de
    djnz test2
    jr line3
fill2:
    ld hl, ( bildp+4 )
    ld de, save_dis+40
    ld b, 20
wait2:
    in0 a,(0c0h)
    and 80h
    jr nz, wait2
    di
    TwoWaits
    ld a, 10000000b+20
    out0 ( 0c0h ), a; goto 20
    OneWait
    ei
ffill2:
cont2:
    in0 a,(0c0h)
    and 80h
    jr nz, cont2
    di
    TwoWaits
    ld a, ( hl )
    out0 ( 0c1h ), a
    ld ( de ), a
    OneWait
    ei
    inc hl
    inc de
    djnz ffill2

start_zeile3:
    ld c, LCD_SCAN
    ld b, 0
    push bc
    call x_short_sleep
    pop bc

line3:
    ld hl, ( bildp+6 )
    ld de, save_dis+60
    ld b, 20
test3:
    ld a, ( de )
    cp ( hl )
    jr nz, fill3
    inc hl
    inc de
    djnz test3
    jr lcd_ready
fill3:
    ld hl, ( bildp+6 )
    ld de, save_dis+60
    ld b, 20
wait3:
    in0 a,(0c0h)
    and 80h
    jr nz, wait3
    di
    TwoWaits
    ld a, 10000000b+84
    out0 ( 0c0h ), a; goto 84
    OneWait
    ei
ffill3:
cont3:
    in0 a,(0c0h)
    and 80h
    jr nz, cont3
    di
    TwoWaits
    ld a, ( hl )
    out0 ( 0c1h ), a
    ld ( de ), a
    OneWait
    ei
    inc hl
    inc de
    djnz ffill3

lcd_ready:
    #endasm
    #endif
 }

void Tdisplay( void ) { /* Display -Refresh  task */
  static byte w;
  static unsigned redraw; /* lcd erneuern alle 20s */
  ;
  for( w=0; w<DISMAX; w++ ) { /* Displaypuffer loeschen */
    display[w]=' ';
    save_dis[w]=0;
   }
  bildp[0]=display;
  bildp[1]=display+20;
  bildp[2]=display+40;
  bildp[3]=display+60;
  ;
  if( !kaltstart ) x_short_sleep( 200 ); /* 1sec fuer lcd power on, wegen PWATCHDOG timeout */
  lcd_hochlauf=0;
  redraw=200;
  ;
  while( 1 ) { /* */
    LCDRefresher();    
    x_sleep( 5 );
    --redraw;
    if( redraw==0 ) { /* 10s abgelaufen */
      redraw=200;
      memset( save_dis, 0, sizeof( save_dis ) ); /* erzwinge redraw */
      init_d();
     }
   }
  ; 
 }


void message( char * text, word time, word info ) { /* display von text fuer time mit zusatzinfo */
  ;
  if( time==0xffff ) { /* bildpuffer umsetzen */
    ReserveMessage();
    disable();
    bildp[0]=text;
    bildp[1]=text+20;
    bildp[2]=text+40;
    bildp[3]=text+60;
    enable();
   }
  else if( time==0xfffe ) { /* bildpuffer wieder auf display */
    disable();
    bildp[0]=display;
    bildp[1]=display+20;
    bildp[2]=display+40;
    bildp[3]=display+60;
    enable();
    ReleaseMessage();
   }
  else { /* normale messages */
    ReserveMessage();
    disable();
    bildp[0]=text;
    bildp[1]=text+20;
    bildp[2]=text+40;
    bildp[3]=text+60;
    enable();
    if( ( word )text>=0x9000 ) { /* sonst zeigt er auf konstanten string */
      if( info ) { /* info anzeigen ? */
        if( info<100 ) {
          text[18]=info/10+'0';
          text[19]=info%10+'0';
         }
        else {
          text[15]=info/10000+'0';
          info%=10000;
          text[16]=info/1000+'0';
          info%=1000;
          text[17]=info/100+'0';
          info%=100;
          text[18]=info/10+'0';
          info%=10;
          text[19]=info+'0';
         }
       }
     }
    if( time ) x_sleep( time );
    else {
      system_stop( 0 );
      system_stop( 1 );
      while( 1 ) {
        if( Fehler1>10 ) break;
        if( Fehler2>10 ) break;
        x_sleep( 20 );
       }
      system_ok( 0 );
      system_ok( 1 );
     }
    disable();
    bildp[0]=display;
    bildp[1]=display+20;
    bildp[2]=display+40;
    bildp[3]=display+60;
    enable();
    ReleaseMessage();
   }
 }

void TextMessage( char * text, word time, word info ) {
  char buf[81];
  ;
  TextCopy( buf, text );
  message( buf, time, info );
 } 

void trigger_wd( void ) {
  #ifndef _MSC_VER
  #asm
  out0 ( PWATCHDOG ), a
  #endasm
  #endif
 }




void strcpy( char * ziel, char * quelle ) {
  while( *quelle ) {
    *ziel=*quelle;
    ++ziel;
    ++quelle;
   }
 }


void strncpy( char * ziel, char * quelle, word nmax ) {
  while( *quelle && nmax ) {
    *ziel=*quelle;
    ++ziel;
    ++quelle;
    --nmax;
   }
 }


byte memcmp( byte * str1, byte * str2, word nmax ) {
  byte n;
  ;
  for( n=0; n<nmax; n++ ) {
    if( str1[n]==str2[n] ) continue;
    else return 0xff;
   } 
  return 0;
 }




word strlen( char * ptr ) { /* reentrante funktion */
  word len;
  ;
  len=0;
  while( 1 ) {
    if( *ptr ) {
      ++len;
      ++ptr;
     } 
    else return len;
   } 
 }


char toupper( char c ) {
  if( c>='a' && c<='z' ) return c-'a'+'A';
  else return c;
 }


byte isdigit( char c ) {
   if( c>='0' && c<='9' ) return c;
   return 0;
 }


byte isxdigit( char c ) {
  if( c>='0'&&c<='9' ) return c;
  if( c>='a'&&c<='f' ) return c;
  if( c>='A'&&c<='F' ) return c;
  return 0;
 }


byte hextobyte( char * p ) { /* zweistelligen string in byte wandeln */
  byte val;
  ;
  val=0;
  if( p[0] ) {
    if( isdigit( p[0] ) ) val=p[0]-'0';
    else val=toupper( p[0] )-'A'+10;
    val<<=4;
   }
  if( p[1] ) {
    if( isdigit( p[1] ) ) val+=p[1]-'0';
    else val+=toupper( p[1] )-'A'+10;
   }
  return val;
 }

void wtoa( word zahl, char * str )  { /* fuenfstellig und rechtsbuendig */
  word div;
  ;
  div=zahl/10000;
  str[0]=div+'0';
  zahl-=div*10000;
  div=zahl/1000;
  str[1]=div+'0';
  zahl-=div*1000;
  div=zahl/100;
  str[2]=div+'0';
  zahl-=div*100;
  div=zahl/10;
  str[3]=div+'0';
  zahl-=div*10;
  str[4]=zahl+'0'; 
 }

void wtoa4( word zahl, char * str ) { /* vierstellig und rechtsbuendig */
  word div;
  ;
  if( zahl<9999 ) {
    div=zahl/1000;
    str[0]=div+'0';
    zahl-=div*1000;
    div=zahl/100;
    str[1]=div+'0';
    zahl-=div*100;
    div=zahl/10;
    str[2]=div+'0';
    zahl-=div*10;
    div=zahl;
    str[3]=div+'0';
   }
  else strncpy( str, "****", 4 );
 }

void wtoa3( word zahl, char * str ) { /* dreistellig und rechtsbuendig */
  word div;
  ;
  if( zahl<999 ) {
    div=zahl/100;
    str[0]=div+'0';
    zahl-=div*100;
    div=zahl/10;
    str[1]=div+'0';
    zahl-=div*10;
    div=zahl;
    str[2]=div+'0';
   }
  else strncpy( str, "***", 3 );
 }

void wtoa2( byte zahl, char * str ) { /* zweistellig und rechtsbuendig */
  word div;
  ;
  if( zahl<99 ) {
    div=zahl/10;
    str[0]=div+'0';
    zahl-=div*10;
    div=zahl;
    str[1]=div+'0';
   }
  else strncpy( str, "**", 2 );
 }

void wtoa3_1( word zahl, char * str ) { /* mit 3.1 stellen */
  word lzahl, div;
  ;
  lzahl=zahl;
  if( lzahl<=9999 ) {
    div=lzahl/1000;
    str[0]=div+'0';
    lzahl-=div*1000;
    div=lzahl/100;
    str[1]=div+'0';
    lzahl-=div*100;
    div=lzahl/10;
    str[2]=div+'0';
    lzahl-=div*10;
    str[3]=',';
    div=lzahl;
    str[4]=div+'0';
    ;
    /* vornullen mit spaces ueberschreiben */
    if( !zahl ) {
      str[0]=0x20; 
      str[1]=0x20; 
      str[2]=0x20; 
      str[3]=0x20; 
      str[4]='0';
     }
    else {
      if( str[0]=='0' ) str[0]=0x20;
      if( ( str[1]=='0' )&&( str[0]==0x20 ) ) str[1]=0x20;
     }
   }
  else strncpy( str, "***.*", 5 );
 }

void byte_to_binaer( byte byt, char * ptr ) { /* byte in einsen und nullen umsetzen */
  ptr[0]='0'+!!( byt&0x80 );
  ptr[1]='0'+!!( byt&0x40 );
  ptr[2]='0'+!!( byt&0x20 );
  ptr[3]='0'+!!( byt&0x10 );
  ptr[4]='0'+!!( byt&0x08 );
  ptr[5]='0'+!!( byt&0x04 );
  ptr[6]='0'+!!( byt&0x02 );
  ptr[7]='0'+!!( byt&0x01 );
 }

#ifdef OLD
memset( adr, wert, anz )
  char * adr, wert;
  word anz; {
  word n;
  ;
  for( n=0; n<anz; n++ ) {
    adr[n]=wert;
   }
 }
#endif

void memset( byte * adr,  byte wert, word anz ) {
  #ifndef _MSC_VER
  #asm
  pop ix;	return
  pop hl;	adr
  pop de;	wert
  pop bc;	anz
  push bc
  push de
  push hl
  push ix
  ;
@1:
  ld a, b
  or c
  ret z
  ;
  ld ( hl ), e
  inc hl
  dec bc
  jr @1
  ;
  #endasm
  #endif
 }  

byte inbyte( unsigned adr )  {
  #ifndef _MSC_VER
  #asm
    pop ix
    pop bc
    push bc
    push ix
    in a,(c)
    ld l,a
    ld h,0
    ret
  #endasm
  #endif            
  return 0;
 }

void outbyte( unsigned adr, byte wert ) {
  #ifndef _MSC_VER
  #asm
    pop ix
    pop bc
    pop de
    ld a,e
    out (c),a
    push de
    push bc
    push ix
    ret
  #endasm
  #endif
 }



byte get_maske( void ) {
  #ifndef _MSC_VER
  #asm
  in0 a, ( 18h )  ;zufallszahl input
  cp 0            ;null maske gibt es nicht
  jr nz, nonull
  ld a, -1        ;dann lieber FF
nonull:
  ld l, a
  ld h, 0
  ret
  #endasm
  #endif
  return 0;
 }

void memcpy( byte * dest, byte * src, word nmax ) { /* standard parameter reihenfolge ! */
  #ifndef _MSC_VER
  #asm   
  pop ix; return
  pop de; dest
  pop hl; src
  pop bc; nmax
  push bc
  push hl
  push de
  push ix
  ;
  ldir
  #endasm
  #endif
 }

void byte_ascii( byte b, char * p ) { /* byte in hex anzeigen */
  byte temp;
  ;
  temp=b>>4;
  p[0]=ASCII[temp];
  temp=b&0x000f;
  p[1]=ASCII[temp];
 }


extern char TEXTSTART[], TEXTEND[]; 
extern unsigned TEXTCS16;

char TextSum16OK( void ) {
  static char * from, * to;
  static unsigned cs;
  ;
  from=TEXTSTART;
  to=TEXTEND;
  cs=0;
  while( from<to ) {
    cs+=*from++;
   }
  return cs==TEXTCS16;
 }

void TextnCopy( char * dst, char * src, unsigned anz ) { /*abort mit anz*/
  char zei;
  ;
  while( anz-- ) {
    zei=*src^( ( ( unsigned )src )-( ( unsigned )TEXTSTART ) );
    if( !zei ) break;
    *dst=zei;
    ++dst;
    ++src;
   }
 }

void TextCopy( char * dst, char * src ) { /*abort mit 0 in string*/
  char zei;
  ;
  while( 1 ) {
    zei=*src^( ( ( unsigned )src )-( ( unsigned )TEXTSTART ) );
    if( !( zei ) ) break;
    *dst=zei;
    ++dst;
    ++src;
   }
 }

/*
   log funktion auf lcd
*/
void logslcd( char * text ) { /* anzeige auf lcd */
  char zei;
  unsigned n;
  ;
  trigger_wd();
  init_d();
  zei=1;
  while( zei ) {
    zei=*text^( ( ( unsigned )text )-( ( unsigned )TEXTSTART ) );
    if( zei ) putchar( zei );
    ++text;
	/*trigger_wd();*/
   }
  for( n=0; n<30000; n++ ) trigger_wd();
  for( n=0; n<20000; n++ ) trigger_wd();
 }



#ifdef FAST_MULT
/* diese beiden funcs sind fuer schnelle integer multiplikation gedacht */

word emu_mul_word_byte( w_wert, b_wert ) /* emulation */
  word w_wert;
  byte b_wert; {
  word temp;

  temp=w_wert*b_wert;
  return temp;
 }

word mul_word_byte( w_wert, b_wert ) /* schnelle word byte multiplikation */ 
  word w_wert;
  byte b_wert; {
  #ifndef _MSC_VER
  #asm
  pop ix;   return adress
  pop de;   word w_wert: d=high( w_wert ), e=low( w_wert )
  pop bc;   byte b_wert: c=b_wert, b=don't care

  ld h, c;  
  ld l, e;  l=low( w_wert ), h=b_wert
  mlt hl;   hl=wert low

  ld b, d;  b=high( w_wert ), c=b_wert
  mlt bc;   bc=wert high

  ld a, h
  add a, c
  ld h, a;  hl=wert

  push bc
  push de
  push ix  
  #endasm
  #endif
 }
#endif

#ifdef FINDB
/* reentrant auxiliary function findbyte: */
/* findb returns index of value val in string p or -1 if failed */
findb( p, val, nmax ) 
  char * p;
  byte val;
  byte nmax; {
  byte n;
  for( n=0; n<nmax; n++ ) {
    if( p[n]==val ) return n;
   }
  return 0xffff;
 }   
#endif

/* 16 Bit Pruefsumme eines Speicherblocks */

/*checksum( startadresse, laenge )
  word startadresse;
  word laenge; {
#asm
    pop ix  ;get retadr
    pop hl  ;get start
    pop bc  ;get laenge
    ld de,0 ;zaehler auf 0

nextsum:
    ld a,b
    or c
    jp z,endsum
    ld a,e
    add a,(hl)
    ld e,a
    jp nc,sumover
    inc d
sumover:
    inc hl
    dec bc
    jp nextsum

endsum: ld h,d
    ld l,e
    push bc
    push hl
    push ix
   #endasm
 }
*/
/*
strncmp( str1, str2, max )
  char *str1, *str2;
  word max; {
  word i;
  if( ( i=max )==0 ) return 0;
  while( *str1==*str2 ) {
    if( --i ) {
      ++str1;
      ++str2;
     }
    else break;
   }
  return ( *str1-*str2 );
 }
*/

/*
strncat(str1, str2, max)
char *str1, *str2;
int max;
{
    int i;
    if ( (i = max) <= 0 )
       return;
    while ( *str1 ++ ) ;
    -- str1;
    while ( i-- && (*str1 ++ = * str2 ++) ) ;
    *str1 = 0;
}
*/
/*
strcmp(str1, str2)
char *str1, *str2;
{
    while ( *str1  == * str2  )
       {
       if(*str1 ++)
        ++ str2;
       else
        return 0;
       }
    return (*str1 - *str2);
}
*/

/*
strcat(str1, str2)
char *str1, *str2;
{
    while ( *str1 ++ ) ;
    -- str1;
    while ( *str1 ++ = * str2 ++ ) ;
}
*/

/*
isupper(c)
int c;
{
   if (c >= 'a' && c <= 'z')
    return c;
   return 0;
}
*/
/*
isspace(c)
int c;
{
   if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
    return c;
   return 0;
}
*/

/*
islower(c)
int c;
{
   if  (c >= 'a' && c <= 'z')
    return c;
   return 0;
}
*/
/*
isascii(c)
int  c;
{
 if (c & 0xff80)
  return 0;
 return c;
}
*/

/*
isalpha(c)
int c;
{
  if ((c >= 'a' && c <= 'z') || (c >= 'a' && c <= 'z'))
    return c;
  return 0;
}
*/

/*
isalnum(c)
int c;
{
   if ( isalpha(c) )
    return c;
   return isdigit(c);
}
*/

/*
natow( str, n )
  char* str;
  char n; {
  extern word atow();
  static char temp[10];
  fill( temp, 0, 10 );
  strncpy( temp, str, 10 );
  return atow( temp );
 }
*/

/*
word atow(hstr)
  char *hstr; {
  static char *str;
  static word wert;

  wert = 0;
  str=hstr;
  while( isspace( *str ) )
    ++str;
  while( isdigit( *str ) )
    wert=wert*10+( *str++ - '0' );
   return wert;
 }
*/

/*
abs( i ) int i; {
   if( i>=0 ) return i;
   return -i;
 }
*/

/*
dword hextodw( p, nmax ) 
  char* p;
  int nmax; {  hex to dword mit n stellen :nur von task0 callen !! 
  static dword accu;
  byte n;

  accu=0l;
  n=nmax-1;
  while( 1 ) {
    if( p[n] ) {  null buffer erkennen 
      if( isdigit( p[n] ) ) accu+=p[n]-'0';
      else accu+=toupper( p[n] )-'A'+10;
     }
    if( !( --n ) ) break;
    accu<<=4;  naechstes digit 
   }
  return accu;
 }
*/
#ifdef FILL
/* Funktion zum Fuellen eines Speicherblocks */
fill( startadresse, laenge, inhalt )
  word startadresse;
  word laenge;
  byte inhalt; {
  #asm
    pop ix  ;get retadr
    pop hl  ;get start
    pop bc  ;get laenge
    pop de  ;get char (e)

testfill:
    ;out0 ( PWATCHDOG ), a
    ld a,b
    or c
    jp z,endfill
    ld (hl),e
    inc hl
    dec bc
    jp testfill

endfill:
    push de
    push bc
    push hl
    push ix
   #endasm
 }
#endif
#ifdef MEMCPY
memcpy( dest, src, nmax ) /* standard parameter reihenfolge ! */
  byte * src;
  byte * dest;
  word nmax; {
  word n;

  for( n=0; n<nmax; n++ ) {
    dest[n]=src[n];
   }
 }
#endif

int abs( int val ) {
  if( val>=0 ) return val;
  else return -val;
 }


void never( void ) { /* nirwana */
  unsigned a=1, b=99, c=99, d=15, e=10;
  ;
  while( 1 ) {
    a=( ( b*c )/d )/e;
   }
 }

