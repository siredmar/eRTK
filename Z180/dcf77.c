#include "types.h"
#include "io.h"                         
#include "rtc.h"
#include "data.h"
#include "funcs.h" 
#include "rtk180.h"
#include "iomod.h"
#include "dcf77.h"


/* #define DCF_DISPLAY */
byte dcf77; /* 1 -> impulse kommen, sonst 0 */
word einrb_on_time, einrb_off_time; /* anzeige pulse in einrb */
byte time_set;
byte dcf77_sec;                             
word dcf_good; /* zeigt die anzahl korrekter lesungen an */


static void get_pulse( void ) {
  while( 1 ) {
    x_wef(); /* warte auf fallende flanke, in on/off time stehen die zeiten */
    einrb_on_time=on_time;
    einrb_off_time=off_time;
    if( ( on_time>2*200 )
      ||( off_time>2*200 ) 
      ||( on_time<10 )
      ||( off_time<10 ) ) { /* fehlimpulse erkennen */
      continue;
     } 
    else {
      break;
     } 
   } 
 }

static word pegel;
static byte value[60];
static const byte WERTIGKEIT[]={ 1, 2, 4, 8, 10, 20, 40, 80 };

static byte sum( byte index, byte len ) {
  static byte sum, n;
  ;
  sum=0;
  for( n=0; n<len; n++ ) {
    sum+=WERTIGKEIT[n]*value[index+n];
   }
  return sum;
 }


static byte dcf77_min, dcf77_std, dcf77_tag, dcf77_mon, dcf77_jah;
static byte dcf77_sec2, dcf77_min2, dcf77_std2, dcf77_tag2, dcf77_mon2, dcf77_jah2;

static byte calculate_value( void ) {
  static byte p1, p2, p3, n;                 

  /* dcf77 zeit errechnen */
  p1=value[21]+value[22]+value[23]+value[24]+value[25]+value[26]+value[27]+value[28];
  if( !( p1&1 ) ) { /* gerade anzahl einsen */
    p2=value[29]+value[30]+value[31]+value[32]+value[33]+value[34]+value[35];
    if( !( p2&1 ) ) { /* gerade anzahl einsen */
      p3=0;
      for( n=36; n<59; n++ ) {
        p3+=value[n];
       }
      if( !( p3&1 ) ) { /* gerade anzahl einsen */
        dcf77_min=sum( 21, 7 );
        dcf77_std=sum( 29, 6 );
        dcf77_tag=sum( 36, 6 );
        dcf77_mon=sum( 45, 5 );
        dcf77_jah=sum( 50, 8 );
        #ifdef DCF_DISPLAY
        display[20+19]=' ';
        #endif
        return 0;
       }
      else {  
        #ifdef DCF_DISPLAY
        display[20+19]='3';
        #endif 
        return 0xff;
       }
     }
    else {    
      #ifdef DCF_DISPLAY
      display[20+19]='2';
      #endif 
      return 0xff;
     }
   }
  else {    
    #ifdef DCF_DISPLAY
    display[20+19]='1';
    #endif      
    return 0xff; 
   }
 }


void Tdcf77( void ) {
  static byte puls_count;
  ;
neustart:              
  dcf_good=0;
  dcf77=0;
  time_set=0;
  dcf77_sec=0xff;
  puls_count=0;
  memset( value, 0, sizeof( value ) );
  while( 1 ) {
    while( 1 ) { 
      get_pulse(); /* nach on_time/off_time */
      ++puls_count;
      if( puls_count>2 ) dcf77=1; /* es kommen daten kontinuierlich */
      if( off_time>2*110 ) break; /* warten auf fehlenden synchron puls */
     } 
    ;
    /* grundzustand */
    dcf77_sec=0;
    while( 1 ) {
      get_pulse();
      ++dcf77_sec;
      if( dcf77_sec>2 ) dcf77=1; /* falls mit dem ersten puls gleich die luecke kam */
      if( dcf77_sec>59 ) {
        dcf77_sec=0; /* 0..59 */
       }
      if( off_time>2*110 ) { /* grundzustand nach luecke */
        dcf77_sec=0;
        if( !calculate_value() ) { /* daten ok */
          /* zeit in systemzeit kopieren */
          if( dcf77_mon>0 && dcf77_mon<13 ) {
            if( dcf77_tag>0 && dcf77_tag<32 ) {
              if( dcf77_std<25 ) {
                if( dcf77_min<60 ) {
                  if( dcf77_sec<60 ) {
                    if( dcf77_jah==dcf77_jah2
                      &&dcf77_mon==dcf77_mon2
                      &&dcf77_tag==dcf77_tag2
                      &&dcf77_std==dcf77_std2
                      &&dcf77_min==dcf77_min2+1
                      &&dcf77_sec==dcf77_sec2 ) { /* plausibel ? */
                      disable();             
                      ++dcf_good;
                      time_set=1;
                      time=0; /* synchronisation interne uhr mit dcf77 */
                      sekunde=dcf77_sec;
                      minute=dcf77_min;
                      stunde=dcf77_std;
                      tag=dcf77_tag;
                      monat=dcf77_mon;
                      if( dcf77_jah>=92 ) jahr=( 1900+dcf77_jah )-1980;
                      else jahr=( 2000+dcf77_jah )-1980;
                      enable();
                      ;
                      /* rtc synchronisieren */
                      rtc_set();
                     }
                    else dcf_good=time_set=0;  
                    dcf77_jah2=dcf77_jah;
                    dcf77_mon2=dcf77_mon;
                    dcf77_tag2=dcf77_tag;
                    dcf77_std2=dcf77_std;
                    dcf77_min2=dcf77_min;
                    dcf77_sec2=dcf77_sec;
                   }
                  else dcf_good=time_set=0;
                 }
                else dcf_good=time_set=0;
               }
              else dcf_good=time_set=0;
             }
            else dcf_good=time_set=0;
           }
          else dcf_good=time_set=0; 
         }
        else dcf_good=time_set=0; /* pruefsummen nicht ok */
       }  
      if( on_time<10 || on_time>40 ) { /* fehlmessung oder stoerung */
        goto neustart;
       }
      if( ( on_time+off_time )<170 ) { /* stoerimpuls */ 
        goto neustart;
       }
      if( on_time>2*12 ) pegel=1;
      else pegel=0;
      value[dcf77_sec]=pegel;
     }
   }
 }

