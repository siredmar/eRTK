/*
 * samd21.c
 *
 * Created: 29.06.2015 10:23:53
 *  Author: er
 */ 

#include "sam.h"

//arm thumb register set
//r0-r12 general purpose registers
//r13=MSP stack pointer
//r14=LR link register
//r15=PC program counter
//PSR program status register
//PRIMASK
//CONTROL

//void * pp_stack;

#ifdef __SAMD21J18A__ 
void test( void ) {
	__asm volatile (
	

	
	);
}
void asmtest( void ) {
	asm volatile(
	"mov r0, #8\n"
	"mov r1, #9\n"
	"mov r2, #10\n"
	"mov r3, #11\n"
	"mov r4, #12\n"
	"mov r8, r0\n"
	"mov r9, r1\n"
	"mov r10, r2\n"
	"mov r11, r3\n"
	"mov r12, r4\n"
	"mov r0, #0\n"
	"mov r1, #1\n"
	"mov r2, #2\n"
	"mov r3, #3\n"
	"mov r4, #4\n"
	"mov r5, #5\n"
	"mov r6, #6\n"
	"mov r7, #7\n"

	"push {lr}\n" \
	"push {r0-r7}\n" \
	"mov r0, r8\n" \
	"mov r1, r9\n" \
	"mov r2, r10\n" \
	"mov r3, r11\n" \
	"mov r4, r12\n" \
	"push { r0-r4 }\n" \

	//do some destructive things
	"mul r0, r0, r1\n"
	"mul r1, r1, r2\n"
	"mul r2, r2, r3\n"
	"mul r3, r3, r4\n"
	"push {r0-r3}\n"
	"pop  {r4-r7}\n"
	"mov r8, r0\n"
	"mov r9, r1\n"
	"mov r10, r2\n"
	"mov r11, r3\n"
	"mov r12, r4\n"
	
	"pop { r0-r4 }\n" \
	"mov r12, r4\n" \
	"mov r11, r3\n" \
	"mov r10, r2\n" \
	"mov r9, r1\n" \
	"mov r8, r0\n" \
	"pop { r0-r7 }\n" \
	"pop { pc }\n" \
	);
}

volatile unsigned cnt;
void  SysTick_Handler( void ) {
	++cnt;
}

/* Constants required to manipulate the NVIC. */
#define NVIC_SYSTICK_CTRL		( ( volatile uint32_t *) 0xe000e010 )
#define NVIC_SYSTICK_LOAD		( ( volatile uint32_t *) 0xe000e014 )
#define NVIC_SYSTICK_CLK		4
#define NVIC_SYSTICK_INT		2
#define NVIC_SYSTICK_ENABLE		1

void init_Systimer( void ) {
	*(NVIC_SYSTICK_LOAD) = ( 48000000 / 1000 ) - 1UL;
	*(NVIC_SYSTICK_CTRL) = NVIC_SYSTICK_CLK | NVIC_SYSTICK_INT | NVIC_SYSTICK_ENABLE;
}
#endif

volatile unsigned cnt2;


/**
 * \brief Application entry point.
 *
 * \return Unused (ANSI-C compatibility).
 */
int main(void)
{
    /* Initialize the SAM system */
    SystemInit();

	init_Systimer();
	asm("cpsie i\n");
	while( 1 ) {
		++cnt2;
	}
}
