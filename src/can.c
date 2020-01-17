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

// TX Message Buffer Size
#define TX_MSGBUF_SIZE 8

// ---------------------------------------------------------------------- Types

typedef struct {
	uint16_t id;
	uint8_t msg_size;
	uint8_t msg[8];
} rx_message_t;

typedef struct {
	uint16_t rx_flags;
	uint8_t msgs_size;
	rx_message_t msgs[14];
} rx_message_buffer_t;

typedef struct {
	uint8_t msg_size;
	uint8_t msg[8];
} tx_message_t;

typedef struct {
	uint8_t write_pos;
	uint8_t read_pos;
	tx_message_t msgs[TX_MSGBUF_SIZE];
} tx_message_buffer_t;

// --------------------------------------------------------------------- Memory

static volatile rx_message_buffer_t rx_msgbuf = {0};
static volatile tx_message_buffer_t tx_msgbuf = {0};

// --------------------------------------------------------- External Functions

void can_init(uint16_t txid) {
	uint8_t cp;

	// Config controller
	CANGCON = _BV(SWRES);
	CANBT1 = 0x02;
	CANBT2 = 0x04;
	CANBT3 = 0x13;
	CANGIE = _BV(ENIT) | _BV(ENTX) | _BV(ENRX);

	// Config mob0 for tx
	CANPAGE = 0x00;
	CANSTMOB = 0x00;
	CANIDM = 0xFFFFFFFF;
#if defined CAN_REV_2A
	CANIDT = _ID_TO_IDT_2A(txid);
	CANCDMOB = 0x00;
#elif defined CAN_REV_2B
	CANIDT = _ID_TO_IDT_2B(txid);
	CANCDMOB = _BV(IDE);
#endif

	// Init mob1 to mob14 for rx
	for (cp = 0x10; cp <= 0xE0; cp += 0x10) {
		CANPAGE = cp;
		CANSTMOB = 0x00;
		CANIDM = 0xFFFFFFFF;
		CANIDT = 0x00000000;
#if defined CAN_REV_2A
		CANCDMOB = 0x00;
#elif defined CAN_REV_2B
		CANCDMOB = _BV(IDE);
#endif
	}

	// Enable controller
	CANGCON = _BV(ENASTB);
}

void can_filter(uint16_t rxid) {
	const uint8_t cp_tmp = CANPAGE;
	
	if (rx_msgbuf.msgs_size < 14) {
		rx_msgbuf.msgs_size++;

		// Init message buffer
		rx_msgbuf.msgs[rx_msgbuf.msgs_size].id = rxid;
		
		// Config mob
		CANPAGE = rx_msgbuf.msgs_size << 4;
#if defined CAN_REV_2A
		CANIDT = _ID_TO_IDT_2A(rxid);
#elif defined CAN_REV_2B
		CANIDT = _ID_TO_IDT_2B(rxid);
#endif
		CANCDMOB |= _BV(CONMOB1);
	}

	CANPAGE = cp_tmp;
}

uint8_t can_message_available(void) {
	
	// Check message buffer rx flags
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
				*msg_size = rx_msgbuf.msgs[bufi].msg_size;
				for (msgi = 0; msgi < rx_msgbuf.msgs[bufi].msg_size; msgi++) {
					*(msg+msgi) = rx_msgbuf.msgs[bufi].msg[msgi];
				}

			// Retry if new data was buffered by the ISR during the copy
			} while (rx_msgbuf.rx_flags & flag_msk);
			break;
		}
		flag_msk <<= 1;
	}
}

void can_transmit(uint8_t *msg, uint8_t msg_size) {
	const uint8_t cp_tmp = CANPAGE;
	uint8_t msgi;

	CANPAGE = 0x00;
	
	// Write to mob if it's disabled and start transmission
	if (!(CANEN2 & _BV(ENMOB0))) {
		for (msgi = 0; msgi < msg_size; msgi++) {
			CANMSG = *(msg+msgi);
		}
#if defined CAN_REV_2A
		CANCDMOB = _BV(CONMOB0) | msg_size;
#elif defined CAN_REV_2B
		CANCDMOB = _BV(CONMOB0) | _BV(IDE) | msg_size;
#endif

	// Write to tx message buffer otherwise
	} else {
		tx_msgbuf.msgs[tx_msgbuf.write_pos].msg_size;
		for (msgi = 0; msgi < msg_size; msgi++) {
			tx_msgbuf.msgs[tx_msgbuf.write_pos].msg[msgi] = *(msg+msgi);
		}
		if (tx_msgbuf.write_pos < TX_MSGBUF_SIZE-1) {
			tx_msgbuf.write_pos++;
		} else {
			tx_msgbuf.write_pos = 0;
		}
	}
	
	CANPAGE = cp_tmp;
}

// ------------------------------------------------- Interrupt Service Routines

ISR(CANIT_vect) {
	const uint8_t cp_tmp = CANPAGE;
	uint8_t bufi, cp_max, cp, msgi;

	// On transmission OK, search tx buffer for new message to send
	CANPAGE = 0x00;
	if (CANSTMOB & _BV(TXOK) && tx_msgbuf.read_pos != tx_msgbuf.write_pos) {
		bufi = tx_msgbuf.read_pos;
		for (msgi = 0; msgi < tx_msgbuf.msgs[bufi].msg_size; msgi++) {
			CANMSG = tx_msgbuf.msgs[bufi].msg[msgi];
		}
#if defined CAN_REV_2A
		CANCDMOB = _BV(CONMOB0) | tx_msgbuf.msgs[bufi].msg_size;
#elif defined CAN_REV_2B
		CANCDMOB = _BV(CONMOB0) | _BV(IDE) | tx_msgbuf.msgs[bufi].msg_size;
#endif
		if (tx_msgbuf.read_pos < TX_MSGBUF_SIZE-1) {
			tx_msgbuf.read_pos++;
		} else {
			tx_msgbuf.read_pos = 0;
		}

	} else {
		cp_max = rx_msgbuf.msgs_size << 4;
		for (cp = 0x01; cp <= cp_max; cp += 0x10) {
			CANPAGE = cp;
			
			// On set rx flag, copy message to buffer and re-enable reception
			if (CANSTMOB & _BV(RXOK)) {
				bufi = (cp >> 4) - 1;
				rx_msgbuf.rx_flags |= 1 << bufi;
				rx_msgbuf.msgs[bufi].msg_size = CANCDMOB & 0x0F;
				for (msgi = 0; msgi < rx_msgbuf.msgs[bufi].msg_size; msgi++) {
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