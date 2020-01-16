#include <avr/io.h>
#include <avr/interrupt.h>
#include <can.h>

int can_test_tx(void) {
	
	can_init(0);
	sei();

	uint8_t dat[8] = {0xAA};
	const uint32_t duration = 1000000;
	uint32_t ledi;
	uint8_t leds = 0x01;
	uint8_t dir = 0;
	DDRC = 0xFF;
	
	while (1) {
		
		dat[0] = leds;
		can_transmit(dat, 1);
		
		for (ledi = 0; ledi < duration; ledi++);
		if (leds == 0x80) {
			dir = 1;
		} else if (leds == 0x01) {
			dir = 0;
		}
		if (dir) {
			leds >>= 1;
		} else {
			leds <<= 1;
		}
		PORTC = leds;
	}

	while(1);
	return 0;
}

int can_test_rx(void) {
	
	can_init(1);
	can_filter(0);
	sei();

	DDRC = 0xFF;
	PORTC = 0x00;
	
	uint16_t id;
	uint8_t dat[8];
	uint8_t len;
	
	while(1) {
		while(can_message_available()) {
			can_receive(&id, dat, &len);
			PORTC = dat[0];
		}
	}
	
	return 0;
}

int main(void) {
    can_test_rx();
    return 0;
}