/*
 * CarPlayerTiny.cpp
 *
 * Created: 25.07.2020 1:18:09
 */ 

// 1.2 MHz (default) built in resonator
#define F_CPU 1200000UL

#include <avr/io.h>
#include <util/delay.h>

#define LED_BIT _BV(PB2)

#define MODE_ENGINE 0
#define MODE_MUSIC 1
#define IN_PB4 (PINB & 0x10)

#define WHEEL_ROTATION_INTERVAL 20
#define LONG_BUTTON_PRESS_INTERVAL 10

typedef enum
{
	BTN_NONE,
	BTN_SHORT_PRESS,
	BTN_LONG_PRESS
} BUTTONS;

BUTTONS GetButton()
{
	static uint8_t pressCounter=0;	
	
	uint8_t isPressed=IN_PB4;
	_delay_ms(50);
	if(isPressed)
	{
		pressCounter++;
	}
	else if(pressCounter > 0)
	{
		_delay_ms(50);
		if(pressCounter>=LONG_BUTTON_PRESS_INTERVAL)
		{
			pressCounter=0;
			return BTN_LONG_PRESS;
		}
		else
		{
			pressCounter=0;
			return BTN_SHORT_PRESS;
		}
	}

	return BTN_NONE;
}

#define EEPROM_write(uiAddress, ucData)\
{\
	while(EECR & (1<<EEPE));\
	EEAR = uiAddress;\
	EEDR = ucData;\
	EECR |= (1<<EEMPE);\
	EECR |= (1<<EEPE);\
}

#define EEPROM_read(var, uiAddress)\
{\
	while(EECR & (1<<EEWE));\
	EEAR = uiAddress;\
	EECR |= (1<<EERE);\
	var=EEDR;\
}

int main()
{
	DDRB |= LED_BIT; // OUTPUT

	DDRB &= ~(1 << DDB4); //set PB4 as button input
	
//	EEPROM_read(volume,0);
	
	while(1)
	{
	//	switch(GetButton())
	//	{
	//	}

		PORTB |= LED_BIT; // HIGH
		_delay_ms(1000);
		PORTB &= ~LED_BIT; // LOW
		_delay_ms(1000);
	}
}