#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>
#include <can.h>

// ---------------------------------------------------------------- Definitions

// Check CAN revision both defined
#if defined CAN_REV_2A && defined CAN_REV_2B
#warning "Both CAN revisions defined. Using 2.0B."
#undef CAN_REV_2A
#endif

// Check CAN revision not defined
#if !defined CAN_REV_2A && !defined CAN_REV_2B
#warning "No CAN revision defined. Using 2.0B."
#define CAN_REV_2B
#endif

// Convert an ID to the CAN revision 2.0A IDT-register format.
#define _ID_TO_IDT_2A(id) (uint32_t)id << 21

// Convert an ID to the CAN revision 2.0B IDT-register format.
#define _ID_TO_IDT_2B(id) (uint32_t)id << 3

// Convert the CAN revision 2.0A IDT-register format to an ID.
#define _IDT_2A_TO_ID(canidt) canidt >> 21

// Convert the CAN revision 2.0B IDT-register format to an ID.
#define _IDT_2B_TO_ID(canidt) canidt >> 3

// ---------------------------------------------------------------------- Types

// --------------------------------------------------------------------- Memory

static volatile uint8_t mobs_in_use = 1;
static volatile void (*handle_receive)(uint16_t id, uint8_t *dat, uint8_t len) = NULL;

// --------------------------------------------------------- External Functions

void can_init(uint16_t txid) {
	uint8_t dat_i;

	// Reset CAN controller
	CANGCON = _BV(SWRES);

	// Set CAN timing bits
	CANBT1 = 0x02;
	CANBT2 = 0x04;
	CANBT3 = 0x13;

	// Enable interrupt(s)
	CANGIE = _BV(ENIT) | _BV(ENRX);

	// Initialize MOb0 (tx)
	CANPAGE = 0x00;
	CANSTMOB = 0x00;
	CANIDM = 0xFFFFFFFF;
	#if defined CAN_REV_2A
	CANCDMOB = 0x00;
	CANIDT = _ID_TO_IDT_2A(txid);
	#elif defined CAN_REV_2B
	CANCDMOB = _BV(IDE);
	CANIDT = _ID_TO_IDT_2B(txid);
	#endif

	// Initialize MOb1 to MOb14 (rx)
	for (dat_i = 1; dat_i < 14; dat_i++) {
		CANPAGE = dat_i << 4;
		CANSTMOB = 0x00;
		CANIDM = 0xFFFFFFFF;
		CANIDT = 0x00000000;
		#if defined CAN_REV_2A
		CANCDMOB = 0x00;
		#elif defined CAN_REV_2B
		CANCDMOB = _BV(IDE);
		#endif
	}

	// Enable CAN controller
	CANGCON = _BV(ENASTB);
}

void can_filter(uint16_t rxid) {

	// Abort if all MObs are in use
	if (mobs_in_use > 14) {
		return;
	}

	// Select MOb
	CANPAGE = mobs_in_use++ << 4;

	// Set MOb id
	#if defined CAN_REV_2A
	CANIDT = _ID_TO_IDT_2A(rxid);
	#elif defined CAN_REV_2B
	CANIDT = _ID_TO_IDT_2B(rxid);
	#endif

	// Enable reception
	CANCDMOB |= _BV(CONMOB1);
}

void can_register_receive_handler(void (*receive_handler)(uint16_t id, uint8_t *dat, uint8_t len)) {

	// Register handle
	handle_receive = (volatile void *)receive_handler;
}

void can_receive(uint16_t *rxid, uint8_t *dat, uint8_t *len) {
	uint8_t cp, cp_max, dat_i;
	
	// Find MOb with set rx flag
	cp_max = mobs_in_use << 4;
	for (cp = 0x10; cp < cp_max; cp+=0x10) {
		CANPAGE = cp;
		if (CANSTMOB & _BV(RXOK)) {
			break;
		}
	}

	// Copy id
	#if defined CAN_REV_2A
	*rxid = _IDT_2A_TO_ID(CANIDT);
	#elif defined CAN_REV_2B
	*rxid = _IDT_2B_TO_ID(CANIDT);
	#endif

	// Copy message length and message
	*len = CANCDMOB & 0x0F;
	for (dat_i = 0; dat_i < *len; dat_i++) {
		*(dat+dat_i) = CANMSG;
	}

	// Reset rx flag and re-enable reception
	CANSTMOB &= ~_BV(RXOK);
	CANCDMOB |= _BV(CONMOB1);
}

void can_transmit(uint8_t *dat, uint8_t len) {
	uint8_t dat_i;

	// Select MOb0
	CANPAGE = 0x00;

	// Set message
	for (dat_i = 0; dat_i < len; dat_i++) {
		CANMSG = *(dat+dat_i);
	}

	// Set message length
	CANCDMOB = (CANCDMOB & 0xF0) | len;

	// Start transmission
	CANCDMOB |= _BV(CONMOB0);
}

// ------------------------------------------------- Interrupt Service Routines

ISR(CANIT_vect) {
}