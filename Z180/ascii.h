/*********************************************************************
 *          C -Deklaration der ASCII -Steuerzeichen                  *
 *********************************************************************/

#define NUL  00
#define SOH  01
#define STX  02
#define ETX  03
#define EOT  04
#define ENQ  05
#define ACK  06
#define BEL  07
#define BS   08
#define HT   09
#define LF   10
#define VT   11
#define FF   12
#define CR   13
#define SO   14
#define SI   15
#define DLE  16
#define DC1  17
#define DC2  18
#define DC3  19
#define DC4  20
#define NAK  21
#define SYN  22
#define ETB  23
#define CAN  24
#define EM   25
#define SUB  26
#define ESC  27
#define FS   28
#define GS   29
#define RS   30
#define US   31

#ifndef _MSC_VER
#asm
XON  equ 11h
XOFF equ 13h
#endasm
#endif

#define XON  0x11
#define XOFF 0x13
