/*
 ===============================================================================
 Name        : Stop Watch
 Author      : Alaa Elsayed
 Version     : Final Version ( 18 / 09 / 2021 )
 Description : Check the PDF included in the file for the Project Requirements.
 ===============================================================================
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> // to provide delay function



/* Global Variables to keep track of the time factors */
unsigned int second = 0;
unsigned int minute = 0;
unsigned int hour = 0;



/* Description :
 * For clock = 1 Mhz and Prescaler F_CPU/64 every count will take ( 64 us )
 * so we need {{ 15625 }} counts to get ( 1 s ) compare match
 * so put initial timer counter = 0 and make output compare register = 15625
 * 0 --> 15625 ( 1 s per compare match )
 */
void Timer1_Init_Compare_Mode(void)
{
	SREG |= ( 1 << 7 ); // Enable interrupts by setting I-Bit

	TCNT1 = 0; // Set Timer1 initial value to 0

	OCR1A = 15625; // Set the Compare value

	/* Configure TIMER1 control register TCCR1A
	 * 1. non-PWM mode 					-> FOC1A = 1 && FOC1B = 0
	 * 2. CTC Mode ( Mode Number 4 )    -> WGM10 = 0 && WGM11 = 0
	 * 3. Disconnect OC1A and OC1B  	-> COM1A0 = 0 && COM1A1 = 0 && COM1B0 = 0 && COM1B1 = 0
	 */
	TCCR1A = ( 1 << FOC1A ) | ( 1 << FOC1B );

	/* Configure TIMER1 control register TCCR1B
	 * 1. CTC Mode ( Mode Number 4 )    -> WGM12 = 1 && WGM13 = 0
	 * 2. Clock = F_CPU / 64 			-> CS10 = 1  && CS11 = 1  && CS12 = 0
	 */
	TCCR1B = ( 1 << WGM12 ) | ( 1 << CS10 ) | ( 1 << CS11 );

	TIMSK |= ( 1 << OCIE1A ); // Enable Timer1 Compare mode Interrupt
} /* end Timer1_Init_Compare_Mode function */


/* External ( TIMER1 ) Interrupt Service Routine */
ISR (TIMER1_COMPA_vect)
{
    second++; // Increment the seconds as a default action

    /* Check if the seconds is equal to 60 */
    if (second == 60)
    {
    	second = 0; // Reset the seconds
    	minute++; // Increment the minutes
    } /* end if */

    /* Check if the minutes is equal to 60 */
    if ( minute == 60)
    {
    	second = 0; // Reset the seconds
    	minute = 0; // Reset the minutes
    	hour++; // Increment the hours
    } /* end if */
} /* end TIMER1 ISR */


/* ========================================================================================================= */


/* Enable external interrupt ( INT0 ) and Reset function configuration */
void INT0_INIT_RESET(void)
{
	DDRD &= ~( 1 << PD2 );			// Configure Pin 2 in PORTD as input pin

	PORTD |= ( 1 << PD2 );			// Enable internal pull-up resistance at PD2

	MCUCR |= ( 1 << ISC01 );		// Trigger INT0 with the falling edge

	GICR |= ( 1 << INT0 );			// Enable external interrupt for pin INT0
} /* end INT0_INIT_RESET function */


/* External ( INT0 ) Interrupt Service Routine */
ISR (INT0_vect)
{
	/* Reset the time factors by assigning them to ZERO */
	second = 0;
	minute = 0;
	hour = 0;
} /* end INT0 ISR */


/* ========================================================================================================= */


/* Enable external interrupt ( INT1 ) and Pause function configuration */
void INT1_INIT_PAUSE(void)
{
	DDRD &= ~( 1 << PD3 );		// Configure Pin 3 in PORTD as input pin

	MCUCR |= ( 1 << ISC11 ) | ( 1 << ISC10 );		// Trigger INT1 with the rising edge

	GICR |= ( 1 << INT1 );		// Enable external interrupt for pin INT1
} /* end INT1_INIT_PAUSE function */


/* External ( INT1 ) Interrupt Service Routine */
ISR (INT1_vect)
{
	/* To Pause the Stopwatch -> We need to Stop the Timer -> We will Clear the Clock Source */
	TCCR1B &= ~( 1 << CS10 ) & ~( 1 << CS11 );
} /* end INT1 ISR */


/* ========================================================================================================= */


/* Enable external interrupt ( INT2 ) and Resume function configuration */
void INT2_INIT_RESUME(void)
{
	DDRB &= ~( 1 << PB2 );		// Configure Pin 2 in PORTB as input pin

	PORTB |= ( 1 << PB2 );       // Enable internal pull-up resistance at PB2

	MCUCSR &= ~( 1 << ISC2 );    // Trigger INT2 with the failing edge

	GICR |= ( 1 << INT2 );		// Enable external interrupt for pin INT2
} /* end INT2_INIT_RESUME function */


/* External ( INT2 ) Interrupt Service Routine */
ISR (INT2_vect)
{
	/* To Resume the Stopwatch -> We need to Resume the Timer -> We will Set the Clock Source
	 * Clock = F_CPU / 64  ->  CS10 = 1  && CS11 = 1  && CS12 = 0
	 */
	TCCR1B |= ( 1 << CS10 ) | ( 1 << CS11 );
} /* end INT2 ISR */


/* ========================================================================================================= */


/* function main begins program execution */
int main(void)
{
	/********** Initialization Code **********/

	DDRC |= 0x0F;		// Configure first 4 Pins in PORTC as output pins >> for decoder
	PORTC &= 0xF0;		// Initialize the 7-segment with value 0 by clear the first four bits in PORTC -> ALL leds are OFF

	DDRA |= 0xFF;      // Configure first 6 Pins in PORTA as output pins >> for seven segment
	PORTA = 0xFF;      //set initial value 1 to first 6 pins in PORTA

	Timer1_Init_Compare_Mode();		// Start TIMER1
	INT0_INIT_RESET();		// Enable external interrupt >> INT0
	INT1_INIT_PAUSE();		// Enable external interrupt >> INT1
	INT2_INIT_RESUME();		// Enable external interrupt >> INT2


	/* Super loop */
	for(;;)
	{
		/********** Application Code **********/

		PORTA = ( 1 << PA5 );								// Enable 1st seven segment
		PORTC = ( PORTC & 0xF0 ) | ( second % 10 );			// Show the first digit of seconds
		_delay_ms(5);

		PORTA = ( 1 << PA4 );								// Enable 2nd seven segment
		PORTC = ( PORTC & 0xF0 )  |  ( second / 10 );		// Show the second digit of seconds
		_delay_ms(5);

		PORTA = ( 1 << PA3 );								// Enable 3rd seven segment
	    PORTC = ( PORTC & 0xF0 ) | ( minute % 10 );			// Show the first digit of minutes
		_delay_ms(5);

	    PORTA = ( 1 << PA2 );								// Enable 4th seven segment
		PORTC = ( PORTC & 0xF0 ) | ( minute / 10 );			// Show the second digit of minutes
		_delay_ms(5);

		PORTA = ( 1 << PA1 );								// Enable 5th seven segment
	    PORTC = ( PORTC & 0xF0 ) | ( hour % 10 );			// Show the first digit of hours
	    _delay_ms(5);

	    PORTA = ( 1 << PA0 );								// Enable 6th seven segment
	    PORTC = ( PORTC & 0xF0 ) | ( hour / 10 );			// Show the second digit of hours
	    _delay_ms(5);
	} /* end super loop */
} /* end function main */
