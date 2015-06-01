/*
 * adc.h
 *
 * Created: 27.05.2015 14:45:41
 *  Author: er
 */ 


#ifndef ADC_H_
#define ADC_H_

uint8_t adc_sequencer( void );    //soll im system timer interrupt aufgerufen werden
uint16_t adc_get( uint8_t mux );  //holen des aktuellen wandlungswertes
uint16_t adc_wait( uint8_t mux ); //warten bis auf diesem kanal eine neue messung vorliegt und dann liefern
void adc_init( void );            //beim hochlauf aufzurufen

#endif /* ADC_H_ */