/*
 * Analog.cpp
 *
 * Created: 10/19/2014 1:02:01 AM
 *  Author: jpagel
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "Analog.h"

uint8_t analog_reference = 0;

//void StartADC()
//{
	//#if defined(ADCSRA)
	//// set a2d prescale factor to 128
	//// 16 MHz / 128 = 125 KHz, inside the desired 50-200 KHz range.
	//// XXX: this will not work properly for other clock speeds, and
	//// this code should use F_CPU to determine the prescale factor.
	//ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);	
//
	//// enable a2d conversions
	//ADCSRA |= (1<<ADEN);
//#endif
//}



int analogRead(uint8_t pinNumber)
{	
	return ADCsingleREAD(pinNumber);
}

#if defined(__AVR_ATMEGA__) || defined(__AVR_ATMEGA328P__) || defined(__AVR_ATMEGA8__) || defined
//This enables the pin for one read, and then disables
int ADCsingleREAD(uint8_t ADCn_touse)
{
	int ADCval;

#if defined(__AVR_ATMEGA328P__) || defined(__AVR_ATMEGA168P__) || defined(__AVR_ATMEGA8__) || defined(__AVR_ATMEGA8A__) || defined(__AVR_ATMEGA88__)
	ADMUX = ADCn_touse;         // use #1 ADC
	ADMUX |= (1 << REFS0);    // use AVcc as the reference
	ADMUX &= ~(1 << ADLAR);   // clear for 10 bit resolution
	
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);// | (1 << ADPS0);
	ADCSRA &= ~(1<<ADPS0);
	ADCSRA |= (1 << ADEN);    // Enable the ADC

	ADCSRA |= (1 << ADSC);    // Start the ADC conversion

	while(ADCSRA & (1 << ADSC));      // Thanks T, this line waits for the ADC to finish


	ADCval = ADCL;
	ADCval = (ADCH << 8) + ADCval;    // ADCH is read so ADC can be updated again
#endif

	//ADCSRA &= ~(1 << ADEN); 
	return ADCval;
}

int ADCAveragedRead(uint8_t ADCn_ToUse, uint8_t numSamplesToAverage)
{	
	uint8_t sampleTotal = 0;
	
	for(uint8_t i = 0; i <= numSamplesToAverage;i++)
	{
		sampleTotal += ADCsingleREAD(ADCn_ToUse);
		asm("");
	}
	ADCSRA &= ~(1 << ADEN);
	return (sampleTotal / numSamplesToAverage);
	
	
}


//Start Constant Read (Interupt driven)

volatile uint8_t ADCvalue;

void StartADCFreeRunning()
{
	#include <avr/interrupt.h>
	ADMUX = 0;                // use ADC0
	ADMUX |= (1 << REFS0);    // use AVcc as the reference
	ADMUX |= (1 << ADLAR);    // Right adjust for 8 bit resolution

	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 128 prescale for 16Mhz
	#if (F_CPU > 8000000UL)
	ADCSRA &= ~(1<<ADPS0);
	#endif
	
	#if defined(__AVR_ATMEGA328P__) || defined(__AVR_ATMEGA168P__) || defined(__AVR_ATTINY44A__) || defined(__AVR_ATTINY24A__) || defined(__AVR_ATTINY84A__) || defined(__AVR_ATTINY45__) || defined(__AVR_ATTINY25__) || defined(__AVR_ATTINY85__)
	ADCSRA |= (1 << ADATE);   // Set ADC Auto Trigger Enable
	#endif
	
	#if defined(__AVR_ATMEGA8__) || defined(__AVR_ATMEGA8A__) || defined(__AVR_ATMEGA88__)
	ADCSRA |= (1 << ADFR);
	#endif
	
	ADCSRB = 0;               // 0 for free running mode

	ADCSRA |= (1 << ADEN);    // Enable the ADC
	ADCSRA |= (1 << ADIE);    // Enable Interrupts

	ADCSRA |= (1 << ADSC);    // Start the ADC conversion

	sei();    // Thanks N, forgot this the first time =P
	
}


ISR(ADC_vect)
{
	ADCvalue = ADCH;          // only need to read the high value for 8 bit
	// REMEMBER: once ADCH is read the ADC will update
	
	// if you need the value of ADCH in multiple spots, read it into a register
	// and use the register and not the ADCH

}

//example of mux decode i freerun mode
//ISR(ADC_vect)
//{
	//uint8_t tmp;            // temp register for storage of misc data
//
	//tmp = ADMUX;            // read the value of ADMUX register
	//tmp &= 0x0F;            // AND the first 4 bits (value of ADC pin being used)
//
	//ADCvalue = ADCH;        // read the sensor value
//
	//if (tmp == 0)
	//{
		//// put ADCvalue into whatever register you use for ADC0 sensor
		//ADMUX++;            // add 1 to ADMUX to go to the next sensor
	//}
	//
	//else if (tmp == 1)
	//{
		//// put ADCvalue into whatever register you use for ADC1 sensor
		//ADMUX++;            // add 1 to ADMUX to go to the next sensor
	//}
	//else if (tmp == 2)
	//// put ADCvalue into whatever register you use for ADC2 sensor
	//ADMUX &= 0xF8;      // clear the last 4 bits to reset the mux to ADC0
//}



//XMEGA
#if defined(__AVR_XMEGA__)

void setupADCFreeRun()
{
	
	// ADCA is enabled
	// Resolution: 8 Bits
	// Current consumption: No limit
	// Conversion mode: Signed
	ADCA.CTRLB=ADC_CURRLIMIT_NO_gc | (1<<ADC_CONMODE_bp) | ADC_RESOLUTION_12BIT_gc;
	
	// Clock frequency: 500.000 kHz
	ADCA.PRESCALER=ADC_PRESCALER_DIV4_gc;
	
	
	ADCA.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
	ADCA.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );
	ADCA.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
	ADCA.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );
	
	// Sampling Time: 0.5 clock cycles = 4.0 us
	//ADCA.SAMPCTRL=0x00;
	
	// Reference: Internal 1.00 V
	// Temperature reference: On
	ADCA.REFCTRL=ADC_REFSEL_INT1V_gc | (1<<ADC_TEMPREF_bp) | (1<<ADC_BANDGAP_bp);
	
	// Initialize the ADC Compare register
	ADCA.CMPL=0x00;
	ADCA.CMPH=0x00;
	
	// ADC channel 0 gain: 1
	// ADC channel 0 input mode: Differential input signal
	ADCA.CH0.CTRL=(0<<ADC_CH_START_bp) | ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	
	// ADC channel 0 positive input: PA6 pin
	// ADC channel 0 negative input: PA1
	ADCA.CH0.MUXCTRL=ADC_CH_MUXPOS_PIN6_gc | ADC_CH_MUXNEG_PIN1_gc | ADC_CH_MUXINT_SCALEDVCC_gc;
	
	// ADC is in Free Running mode
	// Conversions are continuously performed on channel 0
	ADCA.EVCTRL=ADC_EVACT_NONE_gc;
	
	// Free Running mode: On
	ADCA.CTRLB|=ADC_FREERUN_bm;
	
	// Enable the ADC
	ADCA.CTRLA|=ADC_ENABLE_bm;
	// Insert a delay to allow the ADC common mode voltage to stabilize
	_delay_us(2);

	
	
}

uint16_t analogreadPA6(void)
{
	uint16_t data;

	// Wait for the AD conversion to complete
	while ((ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm)==0);
	// Clear the interrupt flag
	ADCA.CH0.INTFLAGS=ADC_CH_CHIF_bm;
	// Read the AD conversion result
	data=ADCA.CH0.RES;
	//data = (8<<data);
	
	return data;
}

void analogWritePORTC(uint8_t portC_PIN_Number, uint8_t dutycyclepercent)
{
	double TCC0_PERValue = 50.0; //Frequency lower number is higher frequency
	
	double percentage = (double)((double)dutycyclepercent/100.0);
	
	uint16_t CCABUFValue = TCC0_PERValue * (double)percentage;
	//uint16_t incomingdutycycle = (uint16_t)dutycyclepercent;
	//float convertedvalue = 65200 * (float)(incomingdutycycle / 100);
	
	PORTC_DIR |= (1<<portC_PIN_Number);  //Set PCn as the output port
	TCC0_PER = TCC0_PERValue;            //Set the period of the waveform
	TCC0_CTRLB |= 0x03;           //Single slope mode
	TCC0_CTRLB |= 0x10;           //channel selection CCAEN
	TCC0_CTRLA |= 0x02;          //clock selection clk/2
	
	
	//TCC0_CCA = 0x2000;
	TCC0_CCABUF = CCABUFValue;
	
	while((TCC0_INTFLAGS & 0x01) == 0);
	
	TCC0_INTFLAGS = 0x00;
}

//HIGH_RES Waveform

//

#endif