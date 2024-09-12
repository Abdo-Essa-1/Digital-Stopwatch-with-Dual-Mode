/*
 ============================================================================
 Name        : StopWatch.c
 Author      : Abdelrahman Essa
 Date        : 11/8/2024
 Description : Stop Watch timer which have count up and count down modes
 ============================================================================
 */

/************** Includes **************/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


/************* Definitions ************/
#define TRUE		(1u)
#define FALSE		(0u)
#define CountUp		(1u)
#define CountDown	(0u)
signed char Seconds = 0;
signed char Minutes = 0;
signed char Hours = 0;
unsigned char reset = FALSE;
unsigned char pause = FALSE;
unsigned char resume = FALSE;
unsigned char flag = FALSE;
unsigned char alarm = FALSE;
unsigned char PauseStatus = FALSE;
unsigned char CountMode = CountUp;


int main()
{
	/********** Initializations **********/

	/** Segment Initializations **/
	DDRC |= 0x0F;									// Setting first 4 pins in port c as o/p (seven segment decoder output)
	DDRA |= 0x3F;									// Setting first 6 pins in port a as o/p (Digit control for the seven segments)
	/** Enabling Timer1 **/
	TCCR1A = _BV(FOC1A) | _BV(FOC1B);
	TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);	// Setting the timer to CTC mode with pre-secalar 1024
	TCNT1 = 0;										// Clearing the timer register
	OCR1A = 15625;									// Setting OCR1A to make the timer counts to 1 second
	TIMSK = _BV(OCIE1A);							// Enabling the timer 1 compare A interrupt
	sei();											// Enabling the global interrupt flag
	/** Digits divide **/
	unsigned char FirDig = 0;
	unsigned char SecDig = 0;
	/** Interrupts Enable **/
	DDRD &= ~_BV(PD2); PORTD |= _BV(PD2);			// Setting PD2 as i/p Pin and Enable the internal pull-up
	DDRD &= ~_BV(PD3);								// Setting PD3 as i/p Pin
	DDRB &= ~_BV(PB2); PORTB |= _BV(PB2);			// Setting PB2 as i/p Pin and Enable the internal pull-up
	MCUCR |= _BV(ISC01) | _BV(ISC11) | _BV(ISC10);	// Setting INT0 to falling edge and INT1 to rising edge
	MCUCSR &= ~_BV(ISC2);							// Setting INT2 to falling edge
	GICR |= _BV(INT0) | _BV(INT1) | _BV(INT2);		// Enabling the interrupts requests
	/** LEDs and Buzzer initializations **/
	DDRD |= _BV(PD4) | _BV (PD5);					// Setting PD4 and PD5 as o/p Pins (LEDs)
	PORTD |= _BV(PD4); PORTD &= ~_BV(PD5);			// Turning the red led initially ON and the yellow Initially OFF (Count Up)
	DDRD |= _BV(PD0); PORTD &= ~_BV(PD0);			// Setting PD0 as o/p Pin and initially OFF (Buzzer)
	/** Switch counter initializations **/
	DDRB &= ~_BV(PB7); PORTB |= _BV(PB7);			// Setting PB7 as i/p Pin and Enable the internal pull-up (Switch Mode)
	/** Time Edit buttons **/
	DDRB &= ~0b01111011; PORTB |= 0b01111011;		// Setting PB0,1,3,4,5,6 as i/p Pins and Enable internal pull-up (Edit buttons)


	/************** Program **************/
	for(;;)
	{
		/** Checking phase of the counters **/
		if(Seconds > 59)							// Checking if the seconds exceeds 59 second
		{
			Seconds = 0;							// Reseting the seconds
			++Minutes;								// Incrementing the minutes
		}
		else if(Seconds < 0)						// Checking if the seconds ended
		{
			Seconds = 59;							// Refilling the seconds
			--Minutes;								// Decrementing the minutes
		}
		if(Minutes > 59)							// Checking if the minutes exceeds 59 minute
		{
			Minutes = 0;							// Reseting the minutes
			++Hours;								// Incrementing the hours
		}
		else if(Minutes < 0)						// Checking if the minutes ended
		{
			Minutes = 59;							// Refilling the minutes
			--Hours;								// Decrementing the hours
		}
		if(Hours > 99)								// Checking if the hours exceeds 99 hour
		{
			// Reseting the whole Stop Watch Timer
			Seconds = 0;
			Minutes = 0;
			Hours = 0;
		}
		else if(Hours < 0)							// Checking if the hours ended so the whole time is ended
		{
			// Stopping timer1 clock
			TCCR1B &= ~0x07;						// Clearing the Timer1 clock select
			// Reseting the whole Stop Watch Timer
			Seconds = 0;
			Minutes = 0;
			Hours = 0;
			// Enabling the alarm
			PORTD |= _BV(PD0);
			alarm = TRUE;
		}
		/** Controlling the StopWatch **/
		if(reset)									// Checking if the reset is pressed
		{
			reset = FALSE;
			// Reseting the whole Stop Watch Timer
			Seconds = 0;
			Minutes = 0;
			Hours = 0;
		}
		if(pause)									// Checking if the pause is pressed
		{
			pause = FALSE;
			PauseStatus = TRUE;
			// Stopping timer1 clock
			TCCR1B &= ~0x07;						// Clearing the Timer1 clock select
			if(alarm)								// Checking if the pause pressed while alarming
			{
				// Disable the alarm
				PORTD &= ~_BV(PD0);
				alarm = FALSE;
			}
		}
		if(resume)									// Checking if the resume is pressed
		{
			resume = FALSE;
			PauseStatus = FALSE;
			// Resuming the timer
			TCCR1B |= _BV(CS12) | _BV(CS10);		// Retrieving the clock select
		}
		if(!(PINB & _BV(PB7)))						// Checking if the Switch is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				CountMode ^= 1;						// Toggling the mode
				PORTD ^= _BV(PD4) | _BV(PD5);		// Toggling the LEDs
				if(alarm)
				{
					TCCR1B |= _BV(CS12) | _BV(CS10);// Retrieving the clock select
					PORTD &= ~_BV(PD0);				// Disable the alarm
					alarm = FALSE;
					PauseStatus = FALSE;
				}
			}
		}
		else if(!(PINB & _BV(PB0)) && PauseStatus)	// Checking if the hours decrement is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Hours > 0)
				{
					--Hours;
				}
			}
		}
		else if(!(PINB & _BV(PB1)) && PauseStatus)	// Checking if the hours increment is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Hours < 99)
				{
					++Hours;
				}
			}
		}
		else if(!(PINB & _BV(PB3)) && PauseStatus)	// Checking if the minutes decrement is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Minutes > 0)
				{
					--Minutes;
				}
			}
		}
		else if(!(PINB & _BV(PB4)) && PauseStatus)	// Checking if the minutes increment is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Minutes < 59)
				{
					++Minutes;
				}
			}
		}
		else if(!(PINB & _BV(PB5)) && PauseStatus)	// Checking if the seconds decrement is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Seconds > 0)
				{
					--Seconds;
				}
			}
		}
		else if(!(PINB & _BV(PB6)) && PauseStatus)	// Checking if the seconds increment is pressed
		{
			if(!flag)
			{
				flag = TRUE;
				if(Seconds < 59)
				{
					++Seconds;
				}
			}
		}
		else
		{
			flag = FALSE;
		}
		/** Displaying phase in the seven segments **/
		FirDig = Seconds % 10;						// Getting the first digit of the seconds
		SecDig = Seconds / 10;						// Getting the second digit of the seconds
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA0);							// Enabling the first digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= FirDig;							// Displaying the first digit of the seconds
		_delay_ms(2);								// Delay
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA1);							// Enabling the second digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= SecDig;							// Displaying the second digit of the seconds
		_delay_ms(2);								// Delay
		FirDig = Minutes % 10;						// Getting the first digit of the minutes
		SecDig = Minutes / 10;						// Getting the second digit of the minutes
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA2);							// Enabling the third digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= FirDig;							// Displaying the second digit of the minutes
		_delay_ms(2);								// Delay
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA3);							// Enabling the fourth digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= SecDig;							// Displaying the second digit of the minutes
		_delay_ms(2);								// Delay
		FirDig = Hours % 10;						// Getting the first digit of the hours
		SecDig = Hours / 10;						// Getting the second digit of the hours
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA4);							// Enabling the fifth digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= FirDig;							// Displaying the second digit of the hours
		_delay_ms(2);								// Delay
		PORTA &= ~0x3F;								// Disabling all digits
		PORTA |= _BV(PA5);							// Enabling the sixth digit
		PORTC &= ~0x0F;								// Clearing first 4 pins in Port c
		PORTC |= SecDig;							// Displaying the second digit of the hours
		_delay_ms(2);								// Delay
	}
}


ISR(TIMER1_COMPA_vect)
{
	switch(CountMode)
	{
	case CountUp:
		++Seconds;
		break;
	case CountDown:
		--Seconds;
		break;
	}
}


ISR(INT0_vect)
{
	reset = TRUE;
}


ISR(INT1_vect)
{
	pause = TRUE;
}


ISR(INT2_vect)
{
	resume = TRUE;
}
