
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#define F_CPU 16000000UL
#define BAUDRATE 9600
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

void USART_init(void);
void USART_send( unsigned char data);
void USART_putstring(char* StringPtr);

char String[10] = "";

volatile uint16_t overflow = 0;
volatile uint16_t start = 0;
volatile uint16_t end = 0;
volatile uint16_t pulse_width = 0;
volatile uint8_t cd_toggle = 0;

void USART_init(void){
	/*Set baud rate */
	UBRR0H = (unsigned char)(BAUD_PRESCALLER>>8);
	UBRR0L = (unsigned char)BAUD_PRESCALLER;
	//Enable receiver and transmitter
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void USART_send( unsigned char data)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void USART_putstring(char* StringPtr){
	while(*StringPtr != 0x00)
	{
		USART_send(*StringPtr);
		StringPtr++;
	}
}

ISR(TIMER0_COMPA_vect) {
	PORTD ^= (1 << PORTD6);
}

ISR(TIMER1_OVF_vect) {
	overflow++;
}

ISR(TIMER1_COMPA_vect) {
	PORTB &= ~(1 << PORTB1);
	DDRB &= ~(1 << PORTB1);
	TCCR1B |= (1 << ICES1);
	TIMSK1 &= ~(1 << OCIE1A);
}

ISR(TIMER1_CAPT_vect) {
	if (TCCR1B & (1 << ICES1)) {
		start = ICR1;
		overflow = 0;
		TCCR1B &= ~(1 << ICES1);
	} else {
		end = ICR1;
		pulse_width = (end + (overflow * 65535)) - start;
		TIMSK1 |= (1 << OCIE1A);
		DDRB |= (1 << PORTB1);
		PORTB |= (1 << PORTB1);
		OCR1A = TCNT1 + 160;
	}
}

ISR(PCINT2_vect) {
	cd_toggle++;
	cd_toggle = cd_toggle % 4;
}

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
	// enable serial communication
	USART_init();
	adc_init();
	uint16_t light_level = 0;
		
	DDRD = 0;
	DDRD |= (1 << PORTD6);
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << CS00) | (1 << CS01); 
	TIMSK0 |= (1 << OCIE0A);
	TIFR0 |= (1 << OCF0A);
	
	PORTD |= (1 << PORTD7);
	PCICR |= (1 << PCIE2);
	PCIFR |= (1 << PCIF2);
	PCMSK2 |= (1 << PCINT23);
	
	DDRB = 0;
	DDRB |= (1 << PORTB1);
	DDRB |= (1 << PORTB2) | (1 << PORTB3) | (1 << PORTB4);
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << CS10) | (1 << ICES1);
	TIMSK1 |= (1 << TOIE1) | (1 << OCIE1A) | (1 << ICIE1);
	TIFR1 |= (1 << TOV1) | (1 << OCF1A) | (1 << ICF1);
	sei();
	PORTB |= (1 << PORTB1);
	OCR1A = TCNT1 + 160;
	
	uint8_t light_level_quantized = 0;
	uint8_t pulse_width_quantized = 0;	
	uint8_t temp2, temp1, temp0;
	for(;;) {
		light_level = adc_read(0);
		
		if (light_level < 544) {
			light_level_quantized = 0;
		} else if (light_level >= 544 && light_level < 589) {
			light_level_quantized = 1;
		} else if (light_level >= 589 && light_level < 633) {
			light_level_quantized = 2;
		} else if (light_level >= 633 && light_level < 678) {
			light_level_quantized = 3;
		} else if (light_level >= 678 && light_level < 722) {
			light_level_quantized = 4;
		} else if (light_level >= 722 && light_level < 767) {
			light_level_quantized = 5;
		} else if (light_level >= 767 && light_level < 811) {
			light_level_quantized = 6;
		} else {
			light_level_quantized = 7;
		}
		 
		temp0 = (light_level_quantized & (1 << 2));
		temp1 = (light_level_quantized & (1 << 1));
		temp2 = (light_level_quantized & (1 << 0));
		
		if (temp0 != (PORTB & (1 << PORTB4)) >> 2) { PORTB ^= (1 << PORTB4);};
		if (temp1 != (PORTB & (1 << PORTB3)) >> 2) { PORTB ^= (1 << PORTB3);};
		if (temp2 != (PORTB & (1 << PORTB2)) >> 2) { PORTB ^= (1 << PORTB2);};
		
		if (cd_toggle) {
			OCR0A = ((pulse_width + 60000)/500);
		} else {
			if (pulse_width < 7143) {
				OCR0A = 119;
				// pulse_width_quantized = 0;
				} else if (pulse_width >= 7143 && pulse_width < 19286) {
				OCR0A = 106;
				// pulse_width_quantized = 1;
				} else if (pulse_width >= 19286 && pulse_width < 26429) {
				OCR0A = 95;
				// pulse_width_quantized = 2;
				} else if (pulse_width >= 26429 && pulse_width < 33571) {
				OCR0A = 89;
				// pulse_width_quantized = 3;
				} else if (pulse_width >= 33571 && pulse_width < 40714) {
				OCR0A = 80;
				// pulse_width_quantized = 4;
				} else if (pulse_width >= 40714 && pulse_width < 47857) {
				OCR0A = 71;
				// pulse_width_quantized = 5;
				} else if (pulse_width >= 47857 && pulse_width < 55000) {
				OCR0A = 63;
				// pulse_width_quantized = 6;
				} else {
				OCR0A = 60;
				// pulse_width_quantized = 7;
			}
		}
		//sprintf(String,"Light level: %u temp0: %u temp1: %u temp2: %u PORTB2: %03u PORTB3: %03u PORTB4: %03u \n", light_level_quantized, temp0, temp1, temp2, (PORTB << PORTB2), (PORTB << PORTB3), (PORTB << PORTB4)); // Print to terminal (converts a number into a string)
		//USART_putstring(String);
		sprintf(String,"Light level: %u Pulse width: %u \n", light_level_quantized, pulse_width); // Print to terminal (converts a number into a string)
		USART_putstring(String);
	}
}