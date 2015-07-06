#define ANZSEMA 20
void sema_init( void );
void ReserveAlloc( void );
void ReleaseAlloc( void );
void ReserveVM( void );
void ReleaseVM( void );
void ReserveDword( void );
void ReleaseDword( void );
void ReserveMessage( void );
void ReleaseMessage( void );
/*void get_accustar( void );
void free_accustar( void );*/
void ReserveLogger( void );
void ReleaseLogger( void );
void ReserveMB( byte mb_id );
void ReleaseMB( byte mb_id );
