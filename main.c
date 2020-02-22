#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include "lcd.h"
#include "usart.h"

#define F_CPU 16000000UL
#define FREQ 16000000
#define BAUD 9600
#define HIGH 1
#define LOW 0
#define BUFFER 1024
#define BLACK 0x000001

char displayChar = 0;
char String[10] = "";

uint16_t array[5] = { 65535, 65535, 65535, 65535, 65535 }; // l_score, r_score, l_padde, r_padde, ball

void adc_init() {
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t ch) {
	ch &= 0b00000111;
	ADMUX = (ADMUX & 0xF8) | ch;
	ADCSRA |= (1 << ADSC);
	while(ADCSRA & (1 << ADSC));
	return(ADC);
}

int main(void)
{
	//setting up the gpio for backlight
	USART_init();
	DDRD |= 0x80;
	PORTD &= ~0x80;
	PORTD |= 0x00;
	DDRB |= 0x05;
	PORTB &= ~0x05;
	PORTB |= 0x00;
	DDRB |= (1 << PORTB5);
	PORTB &= ~(1 << PORTB5);
	DDRC = 0;
	PORTC = 0;
	//lcd initialisation
	lcd_init();
	lcd_command(CMD_DISPLAY_ON);
	lcd_set_brightness(0x18);
	write_buffer(buff);
	_delay_ms(10000);
	clear_buffer(buff);	
	adc_init();
	
	
	
	while (1)
	{
		
		DDRC = 0x02;
		PORTC = 0x01;
		_delay_ms(10);
		
		uint16_t x_pos, y_pos, x_screen, y_screen;
		
		if (!(PINC & (1 << PINC0))) {
			
			PORTB |= (1 << PORTB5);
			DDRC = 0x0A;
			PORTC = 0x08;
			_delay_ms(10);
			y_pos = adc_read(0);
			if (y_pos > 856) { y_pos = 865; } 
			else if (y_pos < 330) { y_pos = 330; }
			DDRC = 0x05;
			PORTC = 0x01;
			_delay_ms(10);
			x_pos = adc_read(3);
			if (x_pos < 135) { x_pos = 135; } 
			else if (x_pos > 845) { x_pos = 845; }
			x_screen = (x_pos - 135) * 2/11;
			y_screen = 64 - (y_pos - 330)*4/33;
			
		} else { PORTB &= ~(1 << PORTB5); }
		
		
	}
}
