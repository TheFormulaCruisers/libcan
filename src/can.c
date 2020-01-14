#include <avr/io.h>
#include <avr/interrupt.h>
#include <can.h>

// ---------------------------------------------------------------- Definitions

// Check CAN revision both defined
#if defined CAN_REV_2A && defined CAN_REV_2B
#warning "Both CAN revisions defined. Using 2.0B."
#undef CAN_REV_2A
#define CAN_REV_2B
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

// --------------------------------------------------------------------- Memory

static volatile void (*handle_receive)(uint16_t id, uint8_t *dat, uint8_t len);

// --------------------------------------------------------- External Functions

void can_init(uint16_t txid) {

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
	CANIDT = _ID_TO_IDT_2B(txid);
	
#ifdef CAN_REV_2A
	CANCDMOB = 0x00;
#endif
#ifdef CAN_REV_2B
	CANCDMOB = _BV(IDE);
#endif

	// Initialize MOb1 to MOb14 (rx)
	uint8_t dat_i;
	for (dat_i = 1; dat_i < 14; dat_i++) {

		CANPAGE = dat_i << 4;
		CANSTMOB = 0x00;
		CANIDM = 0xFFFFFFFF;
		CANIDT = 0x00000000;

#ifdef CAN_REV_2A
		CANCDMOB = 0x00;
#endif
#ifdef CAN_REV_2B
		CANCDMOB = _BV(IDE);
#endif
	}

	// Enable CAN controller
	CANGCON = _BV(ENASTB);
}

void can_filter(uint16_t rxid) {
    
	uint8_t dat_i;
	for (dat_i = 1; dat_i < 14; dat_i++) {

		// Select MOb[i]
		CANPAGE = dat_i << 4;

		// Use MOb[i] if its id is zero (i.e. not yet set)
		if (CANIDT == 0x00000000) {
			
#ifdef CAN_REV_2A
			CANIDT = _ID_TO_IDT_2A(rxid);
#endif
#ifdef CAN_REV_2B
			CANIDT = _ID_TO_IDT_2B(rxid);
#endif

			CANCDMOB = _BV(CONMOB1);
			break;
		}
	}
}

void can_register_receive_handler(void (*receive_handler)(uint16_t id, uint8_t *dat, uint8_t len)) {

	// Register the receive handler
	handle_receive = (volatile void *)receive_handler;
}

void can_receive(uint16_t *rxid, uint8_t *dat, uint8_t *len) {

	uint8_t mob_i, dat_i;
	for (mob_i = 1; mob_i < 14; mob_i++) {

		// Select MOb[i]
		CANPAGE = mob_i << 4;

		// Read MOb[i] if its reception bit has been set
		if (CANSTMOB & _BV(RXOK)) {

			// Get id
#ifdef CAN_REV_2A
			*rxid = _IDT_2A_TO_ID(CANIDT);
#endif
#ifdef CAN_REV_2B
			*rxid = _IDT_2B_TO_ID(CANIDT);
#endif

			// Get message length
			*len = CANCDMOB & 0x0F;

			// Get message
			for (dat_i = 0; dat_i < *len; dat_i++) {
				*(dat+dat_i) = CANMSG;
			}

			// Reset reception bit and re-enable reception
			CANSTMOB &= ~_BV(RXOK);
			CANCDMOB = _BV(CONMOB1);
			break;
		}
	}
}

void can_transmit(uint8_t *dat, uint8_t len) {

	// Select MOb0
	CANPAGE = 0x00;

	// Set message
	uint8_t dat_i;
	for (dat_i = 0; dat_i < len; dat_i++) {
		CANMSG = *(dat+dat_i);
	}

	// Set message length and start transmission
	CANCDMOB = (CANCDMOB & _BV(IDE)) | _BV(CONMOB0) | (len & 0x0F);
}

// ------------------------------------------------- Interrupt Service Routines

ISR(CANIT_vect) {
}