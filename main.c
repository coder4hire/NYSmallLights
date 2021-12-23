/*
 * CarPlayerTiny.cpp
 *
 * Created: 25.07.2020 1:18:09
 */ 

// 9.6 MHz (default) built in resonator
#define F_CPU 9600000UL

#include "light_ws2812.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define OUT_BIT _BV(3)
#define LED_BIT _BV(PB4)

#define IN_PB4 (PINB & 0x10)

#define LONG_BUTTON_PRESS_INTERVAL 10

#define LEDS_COUNT 9
#define outputPin 3
#define FADE_STEPS 16
#define FADE_WAIT_STEPS 20
#define BLACK_STEPS 10

#define RAND_CAP 200
#define TRUE 1
#define FALSE 0

#define EFFECT_TIMEOUT_TICKS 136

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

struct cRGB led[LEDS_COUNT];
struct cRGB fadeInTarget;
int8_t fadeCounter=1;

unsigned short seed=123;
unsigned short effectTicks=0;

uint8_t currentEffectIdx=0;

void effectRandom(uint8_t isStart);
void effectFadeIn(uint8_t isStart);

void (*effects[])(uint8_t)=
{
	effectRandom,
	effectFadeIn
};

uint8_t linrand()
{
	//seed = (75 * seed + 74) % 0x8001;
	seed=(seed>>(seed&2))+0x9779;
	return seed<=RAND_CAP ? seed : seed-RAND_CAP;
}

#define delayTicks(ticks) { _delay_ms(100*ticks);effectTicks+=ticks;}

int main()
{
	void (*currentEffect)(uint8_t) = effects[0];
	
	DDRB |= OUT_BIT; // OUTPUT	
	DDRB |= LED_BIT; // OUTPUT

	//DDRB &= ~(1 << DDB4); //set PB4 as button input
	
//	EEPROM_read(volume,0);
	
	currentEffect(TRUE);
	effectTicks=0;
	while(1)
	{
	//	switch(GetButton())
	//	{
	//	}


		if(effectTicks>EFFECT_TIMEOUT_TICKS)
		{
			effectTicks=0;
			currentEffectIdx=(currentEffectIdx+1)%(sizeof(effects)/sizeof(effects[0]));
			currentEffect=effects[currentEffectIdx];
			currentEffect(TRUE);
		}
		else
		{
			currentEffect(FALSE);
		}			


//	PORTB &= ~LED_BIT; // LOW
	}
}

void effectRandom(uint8_t isStart)
{
	for(uint8_t i=0;i<LEDS_COUNT;i++)
	{
		led[i].r=linrand();
		led[i].g=linrand();
		led[i].b=linrand();
	}	
    ws2812_setleds(led,LEDS_COUNT);
    delayTicks(3);
}

void effectFadeIn(uint8_t isStart)
{
	if(isStart || fadeCounter==-BLACK_STEPS)
	{
		fadeCounter=-BLACK_STEPS;		
		fadeInTarget.r=linrand();
		fadeInTarget.g=linrand();
		fadeInTarget.b=linrand();
		uint8_t offset = linrand()&3;
		if(offset<4)
		{
			((uint8_t*)&fadeInTarget)[offset]>>=4;
		}
	}
	
	fadeCounter++;
	if(fadeCounter==-BLACK_STEPS+1)
	{
		for(uint8_t i=0;i<LEDS_COUNT*3;i++)
		{
			((uint8_t*)&led)[i]=0;
		}
		ws2812_setleds(led,LEDS_COUNT);
	}
	else if(fadeCounter>0 && fadeCounter<=FADE_STEPS)
	{
		led[0].r=(((unsigned short)fadeInTarget.r)*fadeCounter)>>4;
		led[0].g=(((unsigned short)fadeInTarget.g)*fadeCounter)>>4;
		led[0].b=(((unsigned short)fadeInTarget.b)*fadeCounter)>>4;

		for(uint8_t i=1;i<LEDS_COUNT;i++)
		{
			led[i]=led[0];
		}
		ws2812_setleds(led,LEDS_COUNT);
	}
	else if(fadeCounter>FADE_STEPS+FADE_WAIT_STEPS)
	{
		fadeCounter=-BLACK_STEPS;
	}
	
	delayTicks(1);
}
