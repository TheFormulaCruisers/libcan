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

typedef struct {
	uint16_t id;
	uint8_t msg_size;
	uint8_t msg[8];
} message_t;

typedef struct {
	uint16_t rx_flags;
	uint8_t msgs_size;
	message_t msgs[14];
} message_buffer_t;

// --------------------------------------------------------------------- Memory

static volatile message_buffer_t msgbuf = {0};

// --------------------------------------------------------- External Functions

void can_init(uint16_t txid) {
	uint8_t cp;

	// Config controller
	CANGCON = _BV(SWRES);
	CANBT1 = 0x02;
	CANBT2 = 0x04;
	CANBT3 = 0x13;
	CANGIE = _BV(ENIT) | _BV(ENRX);

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
	uint8_t cp_tmp;
	cp_tmp = CANPAGE;
	
	if (msgbuf.msgs_size < 14) {
		msgbuf.msgs_size++;

		// Init message buffer
		msgbuf.msgs[msgbuf.msgs_size].id = rxid;
		
		// Config mob
		CANPAGE = msgbuf.msgs_size << 4;
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
	if (msgbuf.rx_flags) {
		return 1;
	} else {
		return 0;
	}
}

void can_receive(uint16_t *rxid, uint8_t *msg, uint8_t *msg_len) {
	uint16_t flag_msk;
	uint8_t bufi, msgi;
	
	// Select highest priority buffer with a set rx flag
	flag_msk = 0x0001;
	for (bufi = 0; bufi < msgbuf.msgs_size; bufi++) {
		if (msgbuf.rx_flags & flag_msk) {

			// Reset rx flag and copy message
			do {
				msgbuf.rx_flags &= ~flag_msk;
				*rxid = msgbuf.msgs[bufi].id;
				*msg_len = msgbuf.msgs[bufi].msg_size;
				for (msgi = 0; msgi < msgbuf.msgs[bufi].msg_size; msgi++) {
					*(msg+msgi) = msgbuf.msgs[bufi].msg[msgi];
				}

			// Retry if new data was buffered by the ISR during the copy
			} while (msgbuf.rx_flags & flag_msk);
			break;
		}
		flag_msk <<= 1;
	}
}

void can_transmit(uint8_t *msg, uint8_t msg_len) {
	uint8_t msg_i, cp_tmp;
	cp_tmp = CANPAGE;

	// Select mob0, copy message and start transmission
	CANPAGE = 0x00;
	for (msg_i = 0; msg_i < msg_len; msg_i++) {
		CANMSG = *(msg+msg_i);
	}
#if defined CAN_REV_2A
	CANCDMOB = _BV(CONMOB0) | msg_len;
#elif defined CAN_REV_2B
	CANCDMOB = _BV(CONMOB0) | _BV(IDE) | msg_len;
#endif

	CANPAGE = cp_tmp;
}

// ------------------------------------------------- Interrupt Service Routines

ISR(CANIT_vect) {
	uint8_t cp_tmp, cp_max, cp, bufi, msgi;
	cp_tmp = CANPAGE;
	
	cp_max = msgbuf.msgs_size << 4;
	for (cp = 0x10; cp <= cp_max; cp += 0x10) {
		CANPAGE = cp;
		
		// On set rx flag, copy message to message buffer
		if (CANSTMOB & _BV(RXOK)) {
			bufi = (cp >> 4) - 1;
			msgbuf.rx_flags |= 1 << bufi;
			msgbuf.msgs[bufi].msg_size = CANCDMOB & 0x0F;
			for (msgi = 0; msgi < msgbuf.msgs[bufi].msg_size; msgi++) {
				msgbuf.msgs[bufi].msg[msgi] = CANMSG;
			}
			// Re-enable reception
			CANCDMOB |= _BV(CONMOB1);
			break;
		}
	}

	CANSTMOB = 0x00;
	CANPAGE = cp_tmp;
}