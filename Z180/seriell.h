extern byte asci0_rxd_nmax;
extern byte * asci0_rxd_tcd_state;
extern byte * asci0_txd_tcd_state;

extern byte asci1_rxd_nmax;
extern byte * asci1_rxd_tcd_state;
extern byte * asci1_txd_tcd_state;

void asci0_ausgabe( byte * buf, byte n );
void asci1_ausgabe( byte * buf, byte n );
void scc_a_ausgabe( byte * bufr, byte nmax ); /* es werden alle Zeichen incl. 0H gesendet */
void scc_b_ausgabe( byte * bufr, byte nmax ); /* es werden alle Zeichen incl. 0H gesendet */

void asci0_clear( void );
void asci1_clear( void );
void scc_clear_a( void );
void scc_clear_b( void );

word asci0_einlesen( byte * adresse, byte nmax, byte timeout ); /* adresse = Speicherbereich zum Einlesen */
word asci1_einlesen( byte * adresse, byte nmax, byte timeout ); /* adresse = Speicherbereich zum Einlesen */
word scc_a_einlesen( byte * adresse, byte nmax, byte timeout ); /* liefert anzahl eingelesener bytes, bei time out 0! */
word scc_b_einlesen( byte * adresse, byte nmax, byte timeout ); /* liefert anzahl eingelesener bytes, bei time out 0! */
