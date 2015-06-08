/*
 * adc.h
 *
 * Created: 27.05.2015 14:45:41
 *  Author: er
 */ 


#ifndef ADC_H_
#define ADC_H_

#define ADC_HZ1000         1
#define ADC_HZ100         10
#define ADC_HZ10         100
#define ADC_HZ5          200


#define ANZ_ADC 2
typedef struct {
  uint8_t mux;       //0..15 beim atmega256x
  uint8_t ref;       //wert fuer referenz
  uint8_t scaler;    //0=nie, 1=jeden lauf, 2=jeden 2ten lauf, usw
  uint8_t cnt;       //aufwaertszaehler
  uint16_t value;    //adc messwert
  uint8_t tid;       //wenn eine task gestartet werden soll
 } tadc;
extern tadc adc_cntrl[ANZ_ADC];

uint8_t adc_sequencer( void );    //soll im system timer interrupt aufgerufen werden
uint16_t adc_get( uint8_t mux );  //holen des aktuellen wandlungswertes
uint16_t adc_wait( uint8_t mux ); //warten bis auf diesem kanal eine neue messung vorliegt und dann liefern
void adc_init( void );            //beim hochlauf aufzurufen

#endif /* ADC_H_ */