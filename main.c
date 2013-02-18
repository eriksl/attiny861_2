#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "ioports.h"
#include "pwm_timer1.h"
#include "watchdog.h"

enum
{
	WATCHDOG_PRESCALER	= WATCHDOG_PRESCALER_256K,
};

typedef enum
{
	pwm_mode_fade_none			= 0,
	pwm_mode_fade_in			= 1,
	pwm_mode_fade_out			= 2,
	pwm_mode_fade_in_out_cont	= 3,
	pwm_mode_fade_out_in_cont	= 4
} pwm_mode_t;

typedef struct
{
	uint16_t	duty;
	uint16_t	saved_duty;
	pwm_mode_t	pwm_mode:8;
} pwm_meta_t;

static	const	ioport_t		*ioport;
static			pwm_meta_t		pwm_meta[PWM_PORTS];
static			pwm_meta_t		*pwm_slot;

static	uint8_t		slot, keys_down;
static	uint16_t	duty;

static void set_pwm(uint8_t slot, uint16_t duty)
{
	pwm_meta[slot].duty = duty;
	pwm_timer1_set_pwm(slot, duty);

#if 0	// 0.3 mA
	if((pwm_meta[0].duty == 0) && (pwm_meta[1].duty == 0))
	{
		if(!(PRR & _BV(PRTIM1)))
			pwm_timer1_stop();
	}
	else
	{
		if(PRR & _BV(PRTIM1))
			pwm_timer1_start();
	}
#endif
}

ISR(WDT_vect)
{
	for(slot = 0, pwm_slot = &pwm_meta[0]; slot < PWM_PORTS; slot++, pwm_slot++)
	{
		duty = pwmmeta->duty;

		switch(pwm_slot->pwm_mode)
		{
			case(pwm_mode_fade_in):
			case(pwm_mode_fade_in_out_cont):
			{
				if(duty < 0x3ff)
					duty++;
				else
				{
					duty = 0x3ff;

					if(pwm_slot->pwm_mode == pwm_mode_fade_in)
						pwm_slot->pwm_mode = pwm_mode_fade_none;
					else
						pwm_slot->pwm_mode = pwm_mode_fade_out_in_cont;
				}

				pwm_slot->duty = duty;
				pwm_timer1_set_pwm(slot, duty);

				break;
			}

			case(pwm_mode_fade_out):
			case(pwm_mode_fade_out_in_cont):
			{
				if(duty > 0)
					duty--;
				else
				{
					duty = 0;

					if(pwm_slot->pwm_mode == pwm_mode_fade_out)
						pwm_slot->pwm_mode = pwm_mode_fade_none;
					else
						pwm_slot->pwm_mode = pwm_mode_fade_in_out_cont;
				}

					pwmmeta->saved_duty = 0x3ff;
				}

				break;
			}
		}
	}

	watchdog_setup(WATCHDOG_PRESCALER);
}

ISR(PCINT_vect)
{
	uint8_t new_keys_down = 0;

	for(slot = 0, ioport = &input_ports[0]; slot < INPUT_PORTS; slot++, ioport++)
		if(!(*ioport->pin & _BV(ioport->bit)))
			new_keys_down++;

	if(new_keys_down < keys_down)
	{
		keys_down = new_keys_down;
		return;
	}

	keys_down = new_keys_down;

	ioport = &input_ports[0];

	for(slot = 0;  slot < PWM_PORTS; slot++, ioport += 3)
	{
		if(!(*ioport[0].pin & _BV(ioport[0].bit)) && !(*ioport[1].pin & _BV(ioport[1].bit)))
		{
			pwmmeta->pwm_mode = pwm_mode_fade_out;
			continue;
		}

		if(!(*ioport[0].pin & _BV(ioport[0].bit)) && !(*ioport[2].pin & _BV(ioport[2].bit)))
		{
			pwmmeta->pwm_mode = pwm_mode_fade_in;
			continue;
		}

		if(!(*ioport[1].pin & _BV(ioport[1].bit)) && !(*ioport[2].pin & _BV(ioport[2].bit)))
		{
			pwmmeta->pwm_mode = pwm_mode_fade_in_out_cont;
			continue;
		}

		if(!(*ioport[0].pin & _BV(ioport[0].bit)))
		{
			pwm_meta[slot].pwm_mode = pwm_mode_fade_none;

			duty = pwm_timer1_get_pwm(slot);

			if(pwmmeta->saved_duty > 0)	// off
			{
				duty = pwmmeta->duty = pwmmeta->saved_duty;
				pwmmeta->saved_duty = 0;
			}
			else
			{
				pwmmeta->saved_duty = pwmmeta->duty;
				duty = pwmmeta->duty = 0;	// on
			}
		}

		if(!(*ioport[1].pin & _BV(ioport[1].bit)))
		{
			pwmmeta->pwm_mode = pwm_mode_fade_none;

			duty = pwmmeta->duty >> 1;

			if(duty == 0)
				duty = 1;

			pwm_meta[slot].duty = duty;
			pwm_timer1_set_pwm(slot, duty);
		}

		if(!(*ioport[2].pin & _BV(ioport[2].bit)))
		{
			pwmmeta->pwm_mode = pwm_mode_fade_none;

			duty = pwmmeta->duty << 1;

			if(duty == 0)
				duty = 1;

			if(duty > 0x3ff)
				duty = 0x3ff;

			pwm_meta[slot].duty = duty;
			pwm_timer1_set_pwm(slot, duty);
		}
	}
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

	for(slot = 0, ioport = &input_ports[0]; slot < INPUT_PORTS; slot++, ioport++)
	{
		*ioport->port		&= ~_BV(ioport->bit);
		*ioport->ddr		&= ~_BV(ioport->bit);
		*ioport->port		|=  _BV(ioport->bit);
		*ioport->pcmskreg	|=  _BV(ioport->pcmskbit);
		GIMSK				|=  _BV(ioport->gimskbit);
	}

	for(slot = 0, pwmport = &pwm_ports[0]; slot < PWM_PORTS; slot++, pwmport++)
	{
		*pwmport->port		&= ~_BV(pwmport->bit);
		*pwmport->ddr		|= _BV(pwmport->bit);
	}

	for(slot = 0, pwmmeta = &pwm_meta[0]; slot < PWM_PORTS; slot++, pwmmeta++)
	{
		pwmmeta->pwm_mode	= pwm_mode_fade_none;
		pwmmeta->duty		= 0x000;
		pwmmeta->saved_duty	= 0x3ff;
		pwm_timer1_set_pwm(slot, 0);
	}

	// 1 mhz / 4 / 1024 = 244 Hz
	pwm_timer1_init(PWM_TIMER1_PRESCALER_4);
	pwm_timer1_set_max(0x3ff);
	pwm_timer1_set_pwm(0, 0);
	pwm_timer1_set_pwm(1, 0);
	pwm_timer1_start();

	keys_down = 0;
	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();

	watchdog_setup(WATCHDOG_PRESCALER);
	watchdog_start();

	for(;;)
	{
		sleep_mode();
	}

	return(0);
}
