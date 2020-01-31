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

// Check rx message buffer size definition
#if !defined CAN_RX_MSGBUF_SIZE || CAN_RX_MSG_SIZE > 16 || CAN_RX_MSG_SIZE < 0
#warning "CAN_RX_MSGBUF_SIZE not defined. May be 0 to 16. Using 14."
#define CAN_RX_MSGBUF_SIZE 14
#endif

// Check tx message buffer size definition
#if !defined CAN_TX_MSGBUF_SIZE || CAN_TX_MSGBUF_SIZE < 0
#warning "CAN_TX_MSGBUF_SIZE not defined. May be a positive integer. Using 8."
#define CAN_TX_MSGBUF_SIZE 8
#endif

// Convert an ID to the CAN revision 2.0A IDT-register format.
#define _ID_TO_IDT_2A(id) (uint32_t)id << 21

// Convert an ID to the CAN revision 2.0B IDT-register format.
#define _ID_TO_IDT_2B(id) (uint32_t)id << 3

// Convert the CAN revision 2.0A IDT-register format to an ID.
#define _IDT_2A_TO_ID(canidt) (uint32_t)canidt >> 21

// Convert the CAN revision 2.0B IDT-register format to an ID.
#define _IDT_2B_TO_ID(canidt) (uint32_t)canidt >> 3

// ---------------------------------------------------------------------- Types

typedef struct {
	uint16_t id;
	uint8_t size;
	uint8_t msg[8];
} message_t;

typedef struct {
	uint16_t rx_flags;
	uint8_t msgs_size;
	message_t msgs[CAN_RX_MSGBUF_SIZE];
} rx_message_buffer_t;

typedef struct {
	uint8_t write_pos;
	uint8_t read_pos;
	message_t msgs[CAN_TX_MSGBUF_SIZE];
} tx_message_buffer_t;

// --------------------------------------------------------------------- Memory

static volatile rx_message_buffer_t rx_msgbuf = {0};
static volatile tx_message_buffer_t tx_msgbuf = {0};

// --------------------------------------------------------- External Functions

void can_init(void) {
	uint8_t cp;

	// Configure controller
	CANGCON = _BV(SWRES);
	CANBT1 = 0x02;
	CANBT2 = 0x04;
	CANBT3 = 0x13;
	CANIE1 = 0x7F;
	CANIE2 = 0xFF;
	CANGIE = _BV(ENIT) | _BV(ENTX) | _BV(ENRX);

	// Clear mobs
	for (cp = 0x00; cp <= 0xE0; cp += 0x10) {
		CANPAGE = cp;
		CANSTMOB = 0x00;
		CANIDM = 0xFFFFFFFF;
		CANIDT = 0x00000000;
		CANCDMOB = 0x00;
	}

	// Enable controller
	CANGCON = _BV(ENASTB);
}

void can_filter(uint16_t rxid, uint16_t mask) {
	const uint8_t cp_tmp = CANPAGE;
	
	if (rx_msgbuf.msgs_size < CAN_RX_MSGBUF_SIZE) {

		// Init message buffer
		rx_msgbuf.msgs[rx_msgbuf.msgs_size].id = rxid;
		rx_msgbuf.msgs_size++;

		// Config mob
		CANPAGE = rx_msgbuf.msgs_size << 4;
#if defined CAN_REV_2A
		CANIDM ^= _ID_TO_IDT_2A(~mask);
		CANIDT = _ID_TO_IDT_2A(rxid);
		CANCDMOB = _BV(CONMOB1);
#elif defined CAN_REV_2B
		CANIDM ^= _ID_TO_IDT_2B(~mask);
		CANIDT = _ID_TO_IDT_2B(rxid);
		CANCDMOB = _BV(CONMOB1) | _BV(IDE);
#endif
	}

	CANPAGE = cp_tmp;
}

uint8_t can_message_available(void) {
	
	// Check message buffer for rx flags
	if (rx_msgbuf.rx_flags) {
		return 1;
	} else {
		return 0;
	}
}

void can_receive(uint16_t *rxid, uint8_t *msg, uint8_t *msg_size) {
	uint16_t flag_msk;
	uint8_t bufi, msgi;
	
	// Select highest priority buffer with a set rx flag
	flag_msk = 0x0001;
	for (bufi = 0; bufi < rx_msgbuf.msgs_size; bufi++) {
		if (rx_msgbuf.rx_flags & flag_msk) {

			// Reset rx flag and copy message
			do {
				rx_msgbuf.rx_flags &= ~flag_msk;
				*rxid = rx_msgbuf.msgs[bufi].id;
				*msg_size = rx_msgbuf.msgs[bufi].size;
				for (msgi = 0; msgi < rx_msgbuf.msgs[bufi].size; msgi++) {
					*(msg+msgi) = rx_msgbuf.msgs[bufi].msg[msgi];
				}

			// Retry if new data was buffered by the ISR during the copy
			} while (rx_msgbuf.rx_flags & flag_msk);
			break;
		}
		flag_msk <<= 1;
	}
}

void can_transmit(uint16_t txid, uint8_t *msg, uint8_t msg_size) {
	const uint8_t cp_tmp = CANPAGE;
	uint8_t bufi, msgi;

	CANPAGE = 0x00;
	
	// Write to mob if it's not busy and start transmission
	if (!(CANEN2 & _BV(ENMOB0))) {
		for (msgi = 0; msgi < msg_size; msgi++) {
			CANMSG = *(msg+msgi);
		}
#if defined CAN_REV_2A
		CANIDT = _ID_TO_IDT_2A(txid);
		CANCDMOB = _BV(CONMOB0) | msg_size;
#elif defined CAN_REV_2B
		CANIDT = _ID_TO_IDT_2B(txid);
		CANCDMOB = _BV(CONMOB0) | _BV(IDE) | msg_size;
#endif

	// Write to tx message buffer otherwise
	} else {
		bufi = tx_msgbuf.write_pos;
		tx_msgbuf.msgs[bufi].id = txid;
		tx_msgbuf.msgs[bufi].size = msg_size;
		for (msgi = 0; msgi < msg_size; msgi++) {
			tx_msgbuf.msgs[bufi].msg[msgi] = *(msg+msgi);
		}
		if (bufi < CAN_TX_MSGBUF_SIZE-1) {
			tx_msgbuf.write_pos = bufi + 1;
		} else {
			tx_msgbuf.write_pos = 0;
		}
	}
	
	CANPAGE = cp_tmp;
}

// ------------------------------------------------- Interrupt Service Routines

#if defined CANIT_vect
ISR(CANIT_vect) {
#elif defined CAN_INT_vect
ISR(CAN_INT_vect) {
#endif
	const uint8_t cp_tmp = CANPAGE;
	uint8_t bufi, cp_max, cp, msgi;

	// On transmission OK, look for new message in tx buffer and transmit
	CANPAGE = 0x00;
	bufi = tx_msgbuf.read_pos;
	if (CANSTMOB & _BV(TXOK) && bufi != tx_msgbuf.write_pos) {
		for (msgi = 0; msgi < tx_msgbuf.msgs[bufi].size; msgi++) {
			CANMSG = tx_msgbuf.msgs[bufi].msg[msgi];
		}
#if defined CAN_REV_2A
		CANIDT = _ID_TO_IDT_2A(tx_msgbuf.msgs[bufi].id);
		CANCDMOB = _BV(CONMOB0) | tx_msgbuf.msgs[bufi].size;
#elif defined CAN_REV_2B
		CANIDT = _ID_TO_IDT_2B(tx_msgbuf.msgs[bufi].id);
		CANCDMOB = _BV(CONMOB0) | _BV(IDE) | tx_msgbuf.msgs[bufi].size;
#endif
		if (bufi < CAN_TX_MSGBUF_SIZE-1) {
			tx_msgbuf.read_pos = bufi + 1;
		} else {
			tx_msgbuf.read_pos = 0;
		}

	// Otherwise, find which other mob causes the interrupt
	} else {
		cp_max = rx_msgbuf.msgs_size << 4;
		for (cp = 0x10; cp <= cp_max; cp += 0x10) {
			CANPAGE = cp;
			
			// On set rx flag, copy message to buffer and re-enable reception
			if (CANSTMOB & _BV(RXOK)) {
				bufi = (cp >> 4) - 1;
				rx_msgbuf.rx_flags |= 1 << bufi;
#if defined CAN_REV_2A
				rx_msgbuf.msgs[bufi].id = _IDT_2A_TO_ID(CANIDT);
#elif defined CAN_REV_2B
				rx_msgbuf.msgs[bufi].id = _IDT_2B_TO_ID(CANIDT);
#endif
				rx_msgbuf.msgs[bufi].size = CANCDMOB & 0x0F;
				for (msgi = 0; msgi < rx_msgbuf.msgs[bufi].size; msgi++) {
					rx_msgbuf.msgs[bufi].msg[msgi] = CANMSG;
				}
				CANCDMOB |= _BV(CONMOB1);
				break;
			}
		}
	}

	CANSTMOB = 0x00;
	CANPAGE = cp_tmp;
}