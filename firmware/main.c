/*************************************************************************
Title:    testing output to a HD44780 based LCD display.
Author:   Peter Fleury  <pfleury@gmx.ch>  http://jump.to/fleury
File:     $Id: test_lcd.c,v 1.6 2004/12/10 13:53:59 peter Exp $
Software: AVR-GCC 3.3
Hardware: HD44780 compatible LCD text display
          ATS90S8515/ATmega if memory-mapped LCD interface is used
          any AVR with 7 free I/O pins if 4-bit IO port mode is used
**************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h> 
#include <avr/interrupt.h>
#include "usart.h"

#include "uart_hex.c"

#define USART0_RX_BUFFER_SIZE 32      //definiert die größe des Empfangsbuffers. Die Buffergröße kann 2, 4, 8, 16, 32, 64, 128 oder 256 Byte groß sein.
#define USART0_TX_BUFFER_SIZE 32      //definiert die größe des Sendebuffers. Die Buffergröße kann 2, 4, 8, 16, 32, 64, 128 oder 256 Byte groß sein.

#define MESSAGE_SIZE 6

int32_t volatile laser_pos = 0;
int32_t volatile turn_pos = 0;
int32_t volatile laser_dest = 0;
int32_t volatile turn_dest = 0;
int8_t  volatile recv_counter = 0;
char    volatile recv_buffer[MESSAGE_SIZE+1];


void usart0_puts (const char *s)
{
    while (*s)
    {
        usart0_putc(*s);
        s++;
    }
}
void step_laser_left() {
	// Set direction pin
	PORTD |= (1<<5);
	_delay_us(3);
	PORTD |= (1<<6);
	_delay_us(3);
	PORTD &= ~(1<<6);
	laser_pos--;
}

void step_laser_right() {
	// Set direction pin
	PORTD &= ~(1<<5);
	_delay_us(3);
	PORTD |= (1<<6);
	_delay_us(3);
	PORTD &= ~(1<<6);
	laser_pos++;
}


void step_turntable_right() {
	// Set direction pin
	PORTD &= ~(1<<3);
	_delay_us(3);
	PORTD |= (1<<4);
	_delay_us(3);
	PORTD &= ~(1<<4);
	turn_pos++;
}

void step_turntable_left() {
	// Set direction pin
	PORTD |= (1<<3);
	_delay_us(3);
	PORTD |= (1<<4);
	_delay_us(3);
	PORTD &= ~(1<<4);
	turn_pos--;
}

void setup_inputs() {
	// Configure PA0 as output enable laser.
	DDRA  |= (1 << 0);
	PORTA |= (1 << 0);

	// Configure endstop pin for laser.
	DDRA  |= (1 << 1);
	PORTA |= (1 << 1);
}

void home_laser() {
	while (PINA & (1<<0)) {
		step_laser_left();
		_delay_ms(1);
	}
	laser_pos = 0;
}


void process_home() {
	usart0_puts("Processing message home\r\n");
	usart0_puts("recv_buffer: ");
	usart0_puts(recv_buffer);
	usart0_puts("\r\n");
	for (uint8_t i = 0; i < MESSAGE_SIZE; i++) {
		if (recv_buffer[i] != 'H') {
			return;
		}
	}
	usart0_puts("Homing laser\r\n");
	home_laser();
	usart0_puts("Laser homed\r\n");
}

void process_laser() {
	laser_dest = ((int32_t) recv_buffer[4]) | ((int32_t) recv_buffer[3]) << 8;
	usart0_puts("New laser destination set\r\n");
}

void process_message() {
	usart0_puts("Processing message\r\n");
	switch(recv_buffer[0]) {
		case 'H': process_home(); break;
		case 'L': process_laser(); break;
	}
}

int main(void) {

	recv_buffer[MESSAGE_SIZE] = 0;
	
	wdt_disable();
    usart0_init();
		
	sei();


	DDRD |= (1<<3);
	DDRD |= (1<<4);
	DDRD |= (1<<5);
	DDRD |= (1<<6);

	setup_inputs();


	for (;;) {
/*
		unsigned char command = usart0_getc();
		switch (command) {
			case 'd': step_down(); break;
			case 'u': step_up();  break;
			case 'l': step_left(); break;
			case 'r': step_right(); break;
			case '1': step_left(); step_up();  break;
			case '2': step_right(); step_up(); break;
			case '3': step_left(); step_down(); break;
			case '4': step_right(); step_down(); break;
		}
*/
		if (laser_pos < laser_dest) {
			step_laser_right();
			usart0_putc('r');
		}
		if (laser_pos > laser_dest) {
			step_laser_left();
			usart0_putc('l');
		}

		if (usart0_hasc()) {
			recv_buffer[recv_counter] = usart0_getc();
			usart0_putc(recv_buffer[recv_counter]);
			usart0_putc(' ');
			usart0_putc(recv_counter + 65);
			usart0_puts("\r\n");
			recv_counter++;
			if (recv_counter == MESSAGE_SIZE) {
				process_message();
				recv_counter = 0;
			}
		}
		_delay_ms(1);
	}

	return 0;
}



