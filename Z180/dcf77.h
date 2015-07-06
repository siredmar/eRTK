extern byte dcf77; /* 1 -> impulse kommen, sonst 0 */
extern word einrb_on_time, einrb_off_time; /* anzeige pulse in einrb */
extern byte time_set;
extern byte dcf77_sec;                             
extern word dcf_good; /* zeigt die anzahl korrekter lesungen an */

void Tdcf77( void );

