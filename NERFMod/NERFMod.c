/*
 * NERFMod.c
 *
 * Created: 17.03.2014 01:13:16
 *  Author: Timm
 */ 

//-------------------------Defines------------------------
#define F_CPU 1e6

//-------------------------Includes-----------------------
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//-------------------------Variables-----------------------
volatile int ammoCounter = 0;
int newMagSize;
char newMag = 1;
volatile int fireMode = 0;
volatile char fire = 0;
volatile int speed = 1;
volatile char motorOn = 1;
volatile int adcOn = 0;
uint16_t volt = 0;

//-------------------------Methods-------------------------
void init(void);
void startUp(void);
int getFireMode(void);
void display(int display);
void displayLE(int display);
void displaySpeed(int speed, char motorOn);
int getMag(void);
void setSpeed(int speed, char motorOn);
void LEEnabel(int LENumber);
void LEDisable(int LENumber);


int main(void)
{
	init();
	startUp();
	for (int i = 1; i < 4; i++) LEEnabel(i);
	while (1)
	{
		fireMode = getFireMode();
		newMagSize = getMag();			//save the Mag size 
		if(newMagSize == -1) 
		{
			newMag = 0;					//Mag-slot empty 
			ammoCounter = -1;
		}
		
		if(((newMag == 0) & !(newMagSize == -1)))		//Mag-slot not empty anymore 
		{
			ammoCounter = newMagSize;
			newMag = 1;
		}
		if(adcOn == 0)		display(ammoCounter);
		if(adcOn == 1)		
		{
			volt |= (ADCL);
			volt |= (ADCH << 8);
 			volt = volt *15;
			volt = volt /1023;
 			display(volt);
		}
		displaySpeed(speed, motorOn);
		setSpeed(speed, motorOn);
	}
}

void init()
{
	//PORTS
	DDRA = 0b01000000;		//except Pin 6 all Pin as Input
	PORTA = 0b11111111;		//activate PullUp for all Input, Output to VCC
	  //Latch
	DDRB = 0b11111111;		//all Pin as Output
	PORTB = 0b00000000;		//all Pin to GND
	
	DDRC = 0b01111111;		//except Pin 7 all Pin as Output
	PORTC = 0b10000000;		//activate PullUp for Pin 7, Output to GND
	
	DDRD = 0b11110011;		//except Pin 2,3 all Pin as Output
	PORTD = 0b00001111;		//activate PullUp for all Input, Output to GND
	
	//EXTERNAL INTERRUPT
	EIMSK |= (1 << INT0);							//activates the INT0 Interrupt
	EIMSK |= (1 << INT1);							//activates the INT1 Interrupt
	EICRA |= (1 << ISC11) | (1 << ISC01);			//sets INT1 to falling edge 
	
	//EXTERNAL PC-INTERRUPT
	PCMSK0 |= ((1<<PCINT0) | (1<<PCINT1) | (1<<PCINT2));	//set Port A Pin 0,1,2 to External Interrupt
	PCMSK2 |= (1 << PCINT7);						//set Port C Pin 7 to External Interrupt
	PCICR |= ((1 << PCIE0) | (1 << PCIE2));			//activate Port A,C External Interrupt
	
	//PWM Push
	TCCR2A |= (1 << WGM20);							//select phase correct PWM
	OCR2A = 200;									//sets the rate of Output
	
	//PMW FlyWheels
	TCCR1A |= (1 << WGM10) | (1 << COM1A1);			//select phase correct PWM, Pin D5
	OCR1AH = 0x00;
	OCR1AL = 0x00;									//set PWM to zero
	
	//ADC
	ADMUX |= (1 << REFS0);							//select AVCC as reference Voltage
	ADMUX |= ((1<< MUX0) | (1 << MUX1) | (1 << MUX2));	//ADC on Pin 7
	ADCSRA |= (1 << ADEN);							//enables the ADC
	ADCSRA |= ((1 << ADPS2) | (1 << ADPS1));		//set clock division factor to 64
	
	//POWER REDUCTION
	PRR0 |= (1 << PRTWI);							//deactivates TWI
	PRR0 |= (1 << PRUSART1);						//deactivates USART1
	PRR0 |= (1 << PRUSART0);						//deactivates USART0
	
	sei();											//activate Global Interrupt
	
}
void startUp ()
{
	display(-2);
	displaySpeed(1,0);
	setSpeed(1,0);
	_delay_ms(5);
	
	display(-1);
	_delay_ms(5);
	
	for (int i = 1; i < 5; i++)
	{
		display(-2);
		displaySpeed(i,1);
		setSpeed(i,1);
		_delay_ms(5);
		
		display(-1);
		_delay_ms(5);
	}
	for (int i = 4; i > 0; i--)
	{
		display(-2);
		displaySpeed(i,1);
		setSpeed(i,1);
		_delay_ms(5);
		
		display(-1);
		_delay_ms(5);
	}
}

int getFireMode()
{
	return 1;
}
int getMag()
{
	if(!(PINA & (1 << PINA5)))			//Pin 5 GND
	{
		if(!(PINA & ((1 << PINA3) | (1 << PINA4))))		return 6;		//Pin 3,4 GND
		if(!(PINA & (1 << PINA3)))						return 12;		//Pin 3 GND, Pin 4 VCC
		if(!(PINA & (1 << PINA4)))						return 18;		//Pin 3 VCC, Pin 4 GND
		if(PINA & ((1 << PINA3) | (1 << PINA4)))		return 25;		//Pin 3,4 VCC
	}
	return -1;
}

void display(int display)
{
	if (display == -2)
	{
		PORTB = 255;
		
		LEDisable(1);
		LEDisable(2);
		LEEnabel(1);
		LEEnabel(2);
	}
	else if (display == -1)
	{
		PORTB = 16;
		LEDisable(1);
		LEDisable(2);
		LEEnabel(1);
		LEEnabel(2);
	}
	else
	{
		if(display < 10)
		{
			displayLE(0);
			LEDisable(1);
		}
		if(display >= 100)
		{
			PORTB = 0b00010000;	
			LEDisable(1);
		}
		if(display >= 90)
		{
			displayLE(9);
			LEDisable(1);
			display -= 90;
		}
		if(display >= 80)
		{
			displayLE(8);
			LEDisable(1);
			display -= 80;
		}
		if(display >= 70)
		{
			displayLE(7);
			LEDisable(1);
			display -= 70;
		}
		if(display >= 60)
		{
			displayLE(6);
			LEDisable(1);
			display -= 60;
		}
		if(display >= 50)
		{
			displayLE(5);
			LEDisable(1);
			display -= 50;
		}
		if(display >= 40)
		{
			displayLE(4);
			LEDisable(1);
			display -= 40;
		}
		if(display >= 30)
		{
			displayLE(3);
			LEDisable(1);
			display -= 30;
		}
		if(display >= 30)
		{
			displayLE(3);
			LEDisable(1);
			display -= 30;
		}
		if(display >= 20)
		{
			displayLE(2);
			LEDisable(1);
			display -= 20;
		}
		if(display >= 10)
		{
			displayLE(1);
			LEDisable(1);
			display -= 10;
		}
		
		LEEnabel(1);
		
		displayLE(display);
		LEDisable(2);
		LEEnabel(2);
		
	}
}
void displayLE(int display)
{
	switch (display)
	{
		case 0:
		{
			PORTB = 238;
			break;
		}
		case 1:
		{
			PORTB = 96;
			break;
		}
		case 2:
		{
			PORTB = 205;
			break;
		}
		case 3:
		{
			PORTB = 233;
			break;
		}case 4:
		{
			PORTB = 99;
			break;
		}
		case 5:
		{
			PORTB = 171;
			break;
		}
		case 6:
		{
			PORTB = 175;
			break;
		}
		case 7:
		{
			PORTB = 224;
			break;
		}
		case 8:
		{
			PORTB = 239;
			break;
		}
		case 9:
		{
			PORTB = 235;
			break;
		}
	}
}
void displaySpeed(int speed, char motorOn)
{
	if (motorOn == 0) PORTB = 0b00000000;
	if (motorOn == 1)
	{
		if(speed == 1)	PORTB = 0b11000000;
		if(speed == 2)	PORTB = 0b11110000;
		if(speed == 3)	PORTB = 0b11111100;
		if(speed == 4)	PORTB = 0b11111111;
	}
	LEDisable(3);
	LEEnabel(3);
}

void setSpeed(int speed, char motorOn)
{
	if (motorOn == 0)  
	{
		TCCR1B &= ~(1 << CS10);				//PWM off
	}
	if (motorOn == 1)
	{
		TCCR1B |= (1 << CS10);				//PWM on
 		if(speed == 1) OCR1AL = 50;			//set PWM to ...
 		if(speed == 2) OCR1AL = 75;
		if(speed == 3) OCR1AL = 125;
		if(speed == 4) OCR1AL = 255;
	}
}

void LEEnabel(int LENumber)
{
	if (LENumber == 1)	PORTD &= ~(1 << PORTD0);
	if (LENumber == 2)	PORTD &= ~(1 << PORTD1);
	if (LENumber == 3)  PORTA &= ~(1 << PORTA6);
}
void LEDisable(int LENumber)
{
	if (LENumber == 1)  PORTD |= (1 << PORTD0);
	if (LENumber == 2)	PORTD |= (1 << PORTD1);
	if (LENumber == 3)	PORTA |= (1 << PORTA6);
	_delay_ms(25);
}

ISR(INT0_vect)
{
	if (fire == 0 && motorOn == 1)
	{
		fire = 1;
		EICRA |= (1 << ISC00);				//sets INT0 to rising edge
		TCCR2A |= (1 << COM2A1);			//select the Output Pin	for PWM
		TCCR2B |= (1 << CS20);				//activates PWM 
	}
	else if (fire == 1)
	{
		fire = 0;
		EICRA &= ~(1 << ISC00);				//sets INT0 to falling edge
		TCCR2A &= ~(1 << COM2A1);			//select the Output Pin	for PWM
		TCCR2B &= ~(1 << CS20);				//deactivates PWM
	}
}
ISR(INT1_vect)
{
	if(adcOn == 0)
	{
		adcOn = 1;
		EICRA |= (1 << ISC10);				//sets INT1 to rising edge
		ADCSRA |= (1 << ADSC);				//activates the ADC for 1 conversion
	}
	else if(adcOn == 1)
	{
		adcOn = 0;
		EICRA &= ~(1 << ISC10);				//returns INT1 to falling edge
	}
	else adcOn = 0;

}

ISR(PCINT0_vect)
{ 
	if(!(PINA & (1<<PINA0)))			//Increases motor speed
	{
		if (speed < 4)	speed++;
	}
	if(!(PINA & (1<<PINA1)))			//Reduces motor speed
	{
		if(speed > 1)	speed--;
	}
	if(!(PINA & (1<<PINA2)))			//turns motor on and off
	{
		if(motorOn == 0) motorOn = 1;
		else if(motorOn == 1) motorOn = 0;
	}
}
ISR(PCINT2_vect)
{
	if(!(PINC & (1<<PINC7)))			//checks if something passes the light barrier
	{
		if(ammoCounter > 0)	ammoCounter--;
		PORTC ^= 1 << PINC0;
		_delay_ms(25);
		PORTC ^= 1 << PINC0;
	}
	
}
