

#include <stdint.h>

#define PROTOCOL_MAGIC (0xabeefbee)

typedef enum {
    /** Echo request for round-trip-time indication
     * 
     * The receiver reflect message that includes a
     * client side timestamp.
     */
    MSG_TYPE_ECHO_REQUEST,

    /** Echo response for round-trip-time indication */
    MSG_TYPE_ECHO_RESPONSE,

    /** Total number of messages */
    TOTAL_NR_MESSAGE_TYPES
} message_type_t;

typedef struct {
    /** Protocol magic: filter out everything else */
    uint32_t magic;

    /** Allocated client number of sender */
    uint16_t client_id;

    /** Make it possible to detect out of order delivery */
    uint16_t seq_nr;

    /** Type of message */
    uint16_t msg_type;

    /** Checksum of header + data for bit error detection
     * 
     * Checksum field is zero when calculated.
     */
    uint16_t checksum;

    /** Number of bytes for message payload
     * 
     * It is allowed to be zero.
     */
    uint32_t len;
    
    /** Start of payload */
    uint8_t payload[0];
} header_t;


