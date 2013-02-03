#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "ioports.h"
#include "pwm_timer1.h"
#include "watchdog.h"

enum
{
	WATCHDOG_PRESCALER = WATCHDOG_PRESCALER_2K,
};

typedef enum
{
	pwm_mode_none				= 0,
	pwm_mode_fade_in			= 1,
	pwm_mode_fade_out			= 2,
	pwm_mode_fade_in_out_cont	= 3,
	pwm_mode_fade_out_in_cont	= 4
} pwm_mode_t;

typedef struct
{
	uint8_t		duty;
	pwm_mode_t	pwm_mode:8;
} pwm_meta_t;

static	const	ioport_t		*ioport;
static			pwm_meta_t		pwm_meta[PWM_PORTS];
static			pwm_meta_t		*pwm_slot;

static	uint8_t		watchdog_counter;
static	uint8_t		slot, dirty;
static	uint16_t	duty16, diff16;

ISR(WDT_vect)
{
	dirty = 0;

	if(watchdog_counter < 255)
		watchdog_counter++;

	pwm_slot = &pwm_meta[0];

	for(slot = 0; slot < PWM_PORTS; slot++)
	{
		duty16	= pwm_timer1_get_pwm(slot);
		diff16	= duty16 / 8;

		if(diff16 < 8)
			diff16 = 8;

		switch(pwm_slot->pwm_mode)
		{
			case(pwm_mode_fade_in):
			case(pwm_mode_fade_in_out_cont):
			{
				if(duty16 < (1020 - diff16))
					duty16 += diff16;
				else
				{
					duty16 = 1020;

					if(pwm_slot->pwm_mode == pwm_mode_fade_in)
						pwm_slot->pwm_mode = pwm_mode_none;
					else
						pwm_slot->pwm_mode = pwm_mode_fade_out_in_cont;
				}

				pwm_timer1_set_pwm(slot, duty16);

				break;
			}

			case(pwm_mode_fade_out):
			case(pwm_mode_fade_out_in_cont):
			{
				if(duty16 > diff16)
					duty16 -= diff16;
				else
				{
					duty16 = 0;

					if(pwm_slot->pwm_mode == pwm_mode_fade_out)
						pwm_slot->pwm_mode = pwm_mode_none;
					else
						pwm_slot->pwm_mode = pwm_mode_fade_in_out_cont;
				}

				pwm_timer1_set_pwm(slot, duty16);

				break;
			}
		}

		pwm_slot++;
	}

	watchdog_setup(WATCHDOG_PRESCALER);
}

ISR(PCINT_vect)
{
}

int main(void)
{
	cli();

	PRR =		(0 << 7)		|
				(0 << 6)		|	// reserved
				(0 << 5)		|
				(0 << 4)		|
				(0 << PRTIM1)	|	// timer1
				(1 << PRTIM0)	|	// timer0
				(1 << PRUSI)	|	// usi
				(1 << PRADC);		// adc / analog comperator

	DDRA	= 0;
	DDRB	= 0;
	GIMSK	= 0;
	PCMSK0	= 0;
	PCMSK1	= 0;

	for(slot = 0; slot < INPUT_PORTS; slot++)
	{
		ioport				= &input_ports[slot];
		*ioport->port		&= ~_BV(ioport->bit);
		*ioport->ddr		&= ~_BV(ioport->bit);
		*ioport->port		|=  _BV(ioport->bit);
		*ioport->pcmskreg	|=  _BV(ioport->pcmskbit);
		GIMSK				|=  _BV(ioport->gimskbit);
	}

	for(slot = 0; slot < PWM_PORTS; slot++)
	{
		*pwm_ports[slot].ddr 		|= _BV(pwm_ports[slot].bit);
		*pwm_ports[slot].port		&= ~_BV(pwm_ports[slot].bit);
		pwm_meta[slot].pwm_mode		= pwm_mode_none;
	}

	// 1 mhz / 4 / 1024 = 244 Hz
	pwm_timer1_init(PWM_TIMER1_PRESCALER_4);
	pwm_timer1_set_max(0x3ff);
	pwm_timer1_start();

	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();

	watchdog_setup(WATCHDOG_PRESCALER);
	watchdog_start();

	*pwm_ports[1].port |= _BV(pwm_ports[1].bit);

	for(;;)
	{
		uint8_t divisor;

		for(divisor = 0; divisor < 8; divisor++)
		{
			sleep_mode();
		}

		*pwm_ports[0].port ^= _BV(pwm_ports[0].bit);
		*pwm_ports[1].port ^= _BV(pwm_ports[1].bit);
	}

	return(0);
}
