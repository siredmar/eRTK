#define RTC_SEC      0
#define RTC_MIN      2
#define RTC_STD      4
#define RTC_TAG      7
#define RTC_MON      8
#define RTC_JAH      9
#define RTC_DAYCODE  6

/* codes der wochentag speicherzelle */
#define SUNDAY       1
#define MONDAY       2
#define TUESDAY      3
#define WEDNESDAY    4
#define THURSDAY     5
#define FRIDAY       6
#define SATURDAY     7

byte rtc_detect( void ); /* feststellen, ob rtc vorhanden ist -> 1 sonst 0 */
void rtc_run( void ); /* startet die rtc */
void rtc_set( void ); /* schreiben der aktuellen zeit in den rtc */
void rtc_get( void ); /* auslesen des rtc ins ram */
void time_info( void );
