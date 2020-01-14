#ifndef _CAN_H
#define _CAN_H

/**
 * @brief Initialize the CAN controller.
 * @param txid The message ID used for transmission.
 * @return void
 *
 * Resets the CAN controller, sets its transfer speed and its protocol revision
 * (2.0A or 2.0B) for transmission, enables interrupts, clears all message
 * objects and enables the CAN controller.
 *
 * Currently, the transfer speed is set to 1MBps, the protocol revision is set
 * to 2.0B and no interrupts are enabled. 16-bit ID's are used instead of the
 * full 29-bits of the 2.0B revision.
 */
void can_init(uint16_t txid);

/**
 * @brief Configure a message object to filter on a specific message ID.
 * @param rxid The message ID to filter on.
 * @return void
 *
 * Searches for an unused message object and configures it to filter the bus on
 * messages with the message ID specified. Message objects are utilized
 * chronologically and reception is enabled immediately.
 * 
 * The first message object is reserved for transmission and will not be
 * available for reception. The transmission message object uses message ID 0
 * and has the highest priority on the bus.
 *
 * Best practice is to filter messages in descending order of priority. That
 * way, the receive function will always favor a high priority message over a
 * low priority message.
 *
 * @bug Currently, filtered messages with an id of 0 are discarded when also
 * filtering for messages with higher ids, since an id of 0 indicates that the
 * message object is not initialized.
 */
void can_filter(uint16_t rxid);

/**
 * @brief Register the receive handler.
 * @param receive_handler
 * @return void
 *
 * The receive handler is called right after a CAN receive interrupt occurs.
 * The callback function is expected to take three parameters: the node id, a
 * pointer to the data and the number of bytes received.
 */
void can_register_receive_handler(void (*receive_handler)(uint16_t id, uint8_t *dat, uint8_t len));

/**
 * @brief Retrieve a message from the first message object with a set rx flag.
 * @param rxid A pointer to where the message ID will be copied.
 * @param dat A pointer to where the message will be copied.
 * @param len A pointer to where the message length will be copied.
 * @return void
 *
 * Searches through all the message objects and returns the message of the
 * first one it finds with a set reception flag. The id, message and message
 * length of the message object are then copied to the respective memory
 * locations pointed to by the function's parameters. Make sure to reserve
 * enough memory at these locations.
 */
void can_receive(uint16_t *rxid, uint8_t *dat, uint8_t *len);

/**
 * @brief Transmit a message on the bus.
 * @param dat A pointer to where the message is stored.
 * @param len The number of bytes to transmit.
 * @return void
 *
 * Copies the number of bytes specified by len from the memory pointed to by
 * dat to the transmission message object and starts transmission.
 */
void can_transmit(uint8_t *dat, uint8_t len);

#endif // _CAN_H