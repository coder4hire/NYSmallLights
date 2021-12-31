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
#define BTN_BIT _BV(PB4)

#define IN_PB4 (PINB & BTN_BIT)

#define LONG_BUTTON_PRESS_INTERVAL 10

#define LEDS_COUNT 9
#define outputPin 3
#define FADE_STEPS 32
#define FADE_WAIT_STEPS 18
#define FADE_GROUPS 3
#define BLACK_STEPS 12

#define TRUE 1
#define FALSE 0

#define EFFECT_TIMEOUT_TICKS 256

#define setleds(ledarray) ws2812_sendarray_mask((uint8_t*)ledarray, LEDS_COUNT*3, _BV(ws2812_pin))

typedef enum
{
	BTN_NONE,
	BTN_SHORT_PRESS,
	BTN_LONG_PRESS
} BUTTONS;

/*BUTTONS GetButton()
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
}*/

/*#define EEPROM_write(uiAddress, ucData)\
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
}*/

struct cRGB led[LEDS_COUNT];
struct scRGB fadeSteps[FADE_GROUPS];

unsigned short seed=4175;
unsigned short effectTicks=0;

uint8_t currentEffectIdx=0;

void effectRandom();
void effectFadeIn();
void effectFadeOut();
void (*effects[])()=
{
	effectRandom,
	effectFadeIn,
	effectFadeOut
};

uint8_t linrand()
{
	seed = 3 * seed;
	return (seed>>((seed>>3)&0xF));
}

#define delayTicks(ticks) { _delay_ms(50*ticks);effectTicks+=ticks;}

int main()
{
	void (*currentEffect)() = effects[0];
	
	DDRB = OUT_BIT; // Setting output for OUT_BIT, others for input
	PORTB |= BTN_BIT; // Set pull-up high for button

	
//	EEPROM_read(volume,0);
	
	currentEffect(TRUE);
	while(1)
	{
		//switch(GetButton())
		//{
			//case BTN_SHORT_PRESS:
				//effectTicks=EFFECT_TIMEOUT_TICKS;
				//break;
		//}


		if(effectTicks>=EFFECT_TIMEOUT_TICKS || !IN_PB4)
		{
			_delay_ms(200);
			effectTicks=0;
			currentEffectIdx++;
			if(currentEffectIdx>=(sizeof(effects)/sizeof(effects[0])))
			{
				currentEffectIdx=0;
			}
			currentEffect=effects[currentEffectIdx];
			currentEffect(TRUE);
		}
		else
		{
			currentEffect(FALSE);
		}			
	}
}

uint8_t getUnsignedRandomStep() {
	return (linrand()&0x7)|1;
}

void effectRandom()
{
	for(uint8_t i=0;i<LEDS_COUNT;i++)
	{
		led[i].r=linrand();
		led[i].g=linrand();
		led[i].b=linrand();
	}
	setleds(led);
	delayTicks(6);
}

void doTransition(uint8_t fadeCounter)
{
	static uint8_t prevSparkPos=255;
	static uint8_t shouldDoSpark=0;

	if(fadeCounter>=BLACK_STEPS) {
		if(fadeCounter<BLACK_STEPS+FADE_STEPS)
		{
			for(uint8_t i=0;i<FADE_GROUPS;i++)
			{
				led[i].r+=fadeSteps[i].r;
				led[i].g+=fadeSteps[i].g;
				led[i].b+=fadeSteps[i].b;
				led[i+3]=led[i];
				led[i+6]=led[i];			
			}
			setleds(led);
		}
		else
		{
			if(shouldDoSpark)
			{
				uint8_t sparkPos = fadeCounter-(BLACK_STEPS+FADE_STEPS);
				if(sparkPos>=LEDS_COUNT) {
					sparkPos = (LEDS_COUNT*2-1)-sparkPos;
				}
				if(prevSparkPos<LEDS_COUNT) {
					led[prevSparkPos]=led[prevSparkPos + (prevSparkPos<FADE_GROUPS ? FADE_GROUPS: -FADE_GROUPS)];
				}
				prevSparkPos = sparkPos;
				if(sparkPos<LEDS_COUNT)
				{
					led[sparkPos].r=255;
					led[sparkPos].g=255;
					led[sparkPos].b=255;
				}
				setleds(led);
			}
		}
	}
	else
	{
		shouldDoSpark=linrand()>128;
	}

	delayTicks(1);
}

void effectFadeIn()
{
	uint8_t fadeCounter=effectTicks&0x3f;
	if(fadeCounter==0)
	{
		for(uint8_t i=0;i<FADE_GROUPS;i++)
		{
			fadeSteps[i].r=getUnsignedRandomStep();
			fadeSteps[i].g=getUnsignedRandomStep();
			fadeSteps[i].b=getUnsignedRandomStep();
		}
		
		for(uint8_t i=0;i<LEDS_COUNT*3;i++)
		{
			((uint8_t*)&led)[i]=0;
		}
		setleds(led);
	}
	
	doTransition(fadeCounter);
}

void effectFadeOut()
{
	uint8_t fadeCounter=effectTicks&0x3f;
	if(fadeCounter==0)
	{
		for(uint8_t i=0;i<FADE_GROUPS;i++)
		{
			led[i].r = (linrand()&0xE0)|0x20;
			led[i].g = (linrand()&0xE0)|0x20;
			led[i].b = (linrand()&0xE0)|0x20;

			fadeSteps[i].r=-(led[i].r>>5);
			fadeSteps[i].g=-(led[i].g>>5);
			fadeSteps[i].b=-(led[i].b>>5);
		
			led[i+3]=led[i];
			led[i+6]=led[i];
		}
		setleds(led);		
	}
	
	doTransition(fadeCounter);
}
