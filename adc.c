/*
 * adc.c
 *
 *  Created on: 21/08/2012
 *      Author: M. J. Cree
 *      Modified: D. O. Corlett
 *
 *  Modified 8/8/2013 to indicate 8-bit ADC read is broken.
 *
 *  Modified 6/09/16	to display the 8-bit approximation
 *  of the analogue input pin A0 on an LCD, by: D. O. Corlett
 */


#include <avr/io.h>
//#include "adc.h"
#include "my_lcd.h"
#include <util/delay.h>
#include <stdio.h>
#include <math.h>		// log() <-- natural log (ln())
#include <stdlib.h>		// int abs(int x) <-- gets absolute value of integer



/*
 * adc_init()
 *
 * Power up the ADC hardware and select the ADC input pin.
 * pin must be set to a value listed in Table 23-4 of the ATmega manual
 */
void adc_init(int pin)
{
	pin &= 0xf;

	/* Make sure the ADC circuit is powered up */
	PRR &= ~(1<<PRADC);		// Disable Power Reduction ADC bit

	/* Turn off GPIO on the specified analogue input pin */
	if (pin <= 7) {
		/* Only the first eight inputs correspond to GPIO on Port C */
		DIDR0 = 1<<pin;		// Disable digital input on the pin to be used as ADC.
	}

	/* Set up ADC control registers ready for conversions */
        /* Enable ADC and set the lowest bit clock rate. */
	ADCSRA = (1<<ADEN) | 0x07;

	/* Unused so lets just set ADCSRB to 0 */
	ADCSRB = 0;

	/* Select ADC input source, and reference. */
	if (pin == 8) {
		/* Voltage ref is 1.1V for CPU temperature sensor */
		ADMUX = (1<<REFS1) | (1<<REFS0) | pin;
	}else{
		/* Voltage ref is AVcc (5V) for all other inputs */
		ADMUX = (1<<REFS0) | pin;
	}
}


/*
 * adc_read()
 *
 * Perform an analogue to digital conversion and return
 * the converted value.
 * Input source is specified by calling adc_init() first.
 */

int adc_read(void)
{
	int val = 0;				// Initalise ADC value to 0.

	ADCSRA |= 1<<ADSC; 			//Starts conversion (ADSC = 1).

	// While conversion is still in progress:
	while ( ADCSRA & (1 << ADSC) )
	{
		/* do nothing */
	}

	/* Do a 10-bit conversion:	*/
	if ( ADMUX & (1 << ADLAR) )
	{
		// if ADLAR == 1:
		val = ADCL>>6;		// Do a 10-bit conversion by combining ADCL and ADCH making sure to read ADCL before ADCH.
		val |= ADCH<<2;
	}
	else
	{
		// if ADLAR == 0:
		val = ADCL;			// Do a 10-bit conversion by combining ADCL and ADCH making sure to read ADCL before ADCH.
		val |= ADCH<<8;
	}

	return val;
}

/* Calculate the temperature in K given an adc value of a thermistor voltage divider circuit*/
float adcToTempC(int A0)
{
	int Vmax = 5;
	int ADCmax = 1023;
	int R1 = 4700;
	int Rref = 5000;	// 5000 <-- Rref as measured at 25°C on DMM thermocouple

	//float Va = (((float)A0*Vmax)/ADCmax);
	float Va = ((float)A0+3.3769)/206.54;		// Account for zero offset.
	float R2 = (Va*R1)/(Vmax*(1-(Va/Vmax)));


	// Coefficients from thermistor data sheet:
	float A1 = 0.003354016;					// 3.354016E-3
	float B1 = 0.0002569355;				// 2.569355E-4 K^-1
	float C1 = 0.000002626311;				// 2.626311E-6 K^-2
	float D1 = 0.0000000675278;				// 0.675278E-7 K^-3

/*
	// Coefficients from linear fit:
	float A3 = 0.0033503011;
	float B3 = 0.00024008602;
	float C3 = 0.000002626311;				// 2.626311E-6 K^-2 from data sheet
	float D3 = 0.0000000675278;				// 0.675278E-7 K^-3 from data sheet

	// Coefficients from third order fit:
	float A3 = 0.0033513633;
	float B3 = 0.00023904614;
	float C3 = -0.00000045095379;
	float D3 = 0.00000019679716;
*/
	float R2Rref = R2/Rref;
	float lnR2Rref = log(R2Rref);
	float temp = 1/(A1 + B1*lnR2Rref + C1*lnR2Rref*lnR2Rref + D1*lnR2Rref*lnR2Rref*lnR2Rref);	// data sheet
	//float temp = 1/(A3 + B3*lnR2Rref + C3*lnR2Rref*lnR2Rref + D3*lnR2Rref*lnR2Rref*lnR2Rref);	// plot

	return temp - 273.15;	// + 0.5;			// add 0.5 to round correctly
}



/* converts a number of type double to an integer with specified number of decimal
 * places and stores the string in specified memory location with specified buffer size.*/
void floatTostring(char *strResult, int bufferSize, float FloatPntNum, int decimalPlaces)
{
	int IntPart = (int)FloatPntNum;													// Calculate the integer part.
	int FloatPart = abs((int)((FloatPntNum - IntPart)*powf(10, decimalPlaces)));	// Calculate the decimal part.
	//snprintf (strResult, bufferSize, "%i.%02i", IntPart, FloatPart);					// Create the floating point number string.


	//Solution to variable precision provided by Shea Gosnell
	char format[] = {'%','i','.','%','0', decimalPlaces+48 ,'i',0};
	snprintf (strResult, bufferSize, format, IntPart, FloatPart);					// Create the floating point number string.
	//end code of Shea
}

/* Puts the given value into the specified array location:
 * location: 	Start address of array.
 * size: 		Allocated size of the array.
 * position: 	Position in the array where the value is to be stored.
 * value: the 	Value to be stored in the array.
 */
int LogVal(int *location, int size, int position, int value)
{
	if(position < size)
	{
		location[position] = value;
		return 0;		// return location stored.
	}
	else
	{
		return 1;		// return error: location not in array.
	}
}

int main()
{
	adc_init(0);		// Initalise pin 0 on portC for ADC read.
	my_lcd_init(4);		// Initalise 4 bit LCD on port D

	char con[33];		// Define display string

	int A0;

	/*Define variables for temperature log*/
	//int position = 0;
	//int time = 0;
	//int logarray[8640];


	/* variables for returning a string
	 * with the floatTostring().*/
	int Tempsize = 10;					// Size of the string.
	char *Temp_C = malloc(Tempsize);	// Allocate memory for the string.

	while(1)
	{
		A0 = adc_read();		// Read the ADC value on pin A0;

		/* Convert ADC value on A) to temperature value in C,
		 * and convert to string to be displayed.*/
		floatTostring(Temp_C, Tempsize, adcToTempC(A0), 2);

		// Create a string with the A0 ADC value.
		snprintf( con, 33, "%s%i%c%s%s%c%s", "A0 = ", (int)A0, '\n',"Temp = ", Temp_C, 0xDF,"C" );	//0xDF is °
		free(Temp_C);				// free the allocated memory created for the floatTostring() call.

		my_lcd_display(con);		// Display A0 ADC value on LCD.
		_delay_ms(100);

		/* log temperature*/
		//if(!(time % 1000))			// every 10 seconds
		//{
		//	LogVal(logarray,8640,position,A0);
		//position++;
		//}
		//time++;
	}

	return 0;
}
