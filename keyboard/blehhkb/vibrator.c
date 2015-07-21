#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h> 
#include <stdint.h>
#include "vibrator.h"

#define VIBRATOR_TIMER_TOP F_CPU/(256*64)

uint8_t acc = 0;
uint8_t duty_cycle = 0;
uint8_t acc2 =0;
bool is_init = false;

void vibrator_timer_init(void)
{
    if(is_init){
        return;
    }
    is_init = true;
    /* Timer1 setup */
    /* CTC mode */
    TCCR1B |= _BV(WGM12);
    /* Clock selelct: clk/1 */
    TCCR1B |= _BV(CS10);
    /* Set TOP value */
    uint8_t sreg = SREG;
    cli();
    OCR1AH = (VIBRATOR_TIMER_TOP>>8)&0xff;
    OCR1AL = VIBRATOR_TIMER_TOP&0xff;
    SREG = sreg;
}
void vibrator_timer_enable(void)
{
    /* Enable Compare Match Interrupt */
    TIMSK1 |= _BV(OCIE1A);
}

void vibrator_timer_disable(void)
{
    /* Disable Compare Match Interrupt */
    TIMSK1 &= ~_BV(OCIE1A);
}
void vibrator_init(void)
{
	vibrator_timer_init();
    DDRF |= (1<<0);
    PORTF &= ~(1<<0);
}
void vibrator_open(void)
{
    PORTF |= (1<<0);
}
void vibrator_close(void)
{
 	PORTF &= ~(1<<0);
}
void vibrator_tick(uint8_t nums)
{
	if(duty_cycle>0){
		return;
	}
	duty_cycle=nums;
	vibrator_timer_enable();
}

ISR(TIMER1_COMPA_vect)
{
    acc++;
    if(acc == 255){
        acc = 0;
        acc2++;
    }
    if(acc2 < 7){
        vibrator_open();
    }
    else if(acc2 <10){
        vibrator_close();
    }
    else{
        acc=0;
        acc2=0;
        duty_cycle--;
    }
    if (duty_cycle == 0) 
    {
        vibrator_timer_disable();
        vibrator_close();
    }
}




