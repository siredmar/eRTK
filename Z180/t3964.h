
/**************** Zeiten laut 3964 -Protokoll ****************/
#define TZ 40  /* 200 msec Ueberwachungszeit zwischen den Datenbytes */
#define TQ 110 /* 550 msec Ueberwachungszeit nach STX */

/********** interne Festlegung auf 50 Bytes *********/
#define MAX_BLOCK_LEN 50

extern byte ausgab_3964( byte * puffer, unsigned anz ); /* Senderoutine 3964 */
extern byte einles_3964( byte * puffer, unsigned anz, byte U_zeit ); /* Empfangsroutine 3964 */
