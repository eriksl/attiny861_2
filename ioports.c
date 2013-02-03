#include <stdint.h>
#include <avr/io.h>

#include "ioports.h"

const adcport_t adc_ports[ADC_PORTS] = 
{
};

const ioport_t input_ports[INPUT_PORTS] =
{
	{ &PORTA, &PINA, &DDRA, 7, &PCMSK0, PCINT7, PCIE1 },	// a7
	{ &PORTA, &PINA, &DDRA, 6, &PCMSK0, PCINT6, PCIE1 },	// a6
	{ &PORTA, &PINA, &DDRA, 5, &PCMSK0, PCINT5, PCIE1 },	// a5
	{ &PORTA, &PINA, &DDRA, 2, &PCMSK0, PCINT2, PCIE1 },	// a2
	{ &PORTA, &PINA, &DDRA, 1, &PCMSK0, PCINT1, PCIE1 },	// a1
	{ &PORTA, &PINA, &DDRA, 0, &PCMSK0, PCINT0, PCIE1 },	// a0
};

const ioport_t output_ports[OUTPUT_PORTS] =
{
};

const ioport_t internal_output_ports[INTERNAL_OUTPUT_PORTS] =
{
};

const pwmport_t pwm_ports[PWM_PORTS] =
{
	{ &PORTB, &DDRB, 3, &TCCR1A, COM1B1, COM1B0, FOC1B, PWM1B, &TC1H, &OCR1B },
	{ &PORTB, &DDRB, 5, &TCCR1C, COM1D1, COM1D0, FOC1D, PWM1D, &TC1H, &OCR1D }
};
