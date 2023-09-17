#include <stdio.h>
#include <stdlib.h>
#include "libcanard/canard.h"
#include "libcanard/funccanard.h"

int main() {
    uint8_t buffer[44];
    serializeFrame(0, NULL, 11, 1, buffer);
    uint16_t identifer = 0;
    deserializeFrame(buffer, &identifer, 44);
    /*
    CanardInstance canard = canardInit(&memAllocate, &memFree);
    canard.node_id = 42;                        // Defaults to anonymous; can be set up later at any point.

    CanardTxQueue queue = canardTxInit(100,                 // Limit the size of the queue at 100 frames.
                                   CANARD_MTU_CAN_FD);  // Set MTU = 64 bytes. There is also CANARD_MTU_CAN_CLASSIC.


    uint8_t tx_deadline_usec = 0;

    static uint8_t my_message_transfer_id;  // Must be static or heap-allocated to retain state between calls.

    char* payload = "\x2D\x00" "Sanchoe, it strikes me thou art in great fear.";

    transmitFrame(queue, canard, tx_deadline_usec, &my_message_transfer_id, 1234, 48, payload);

    char* payload2 = "Hello there";

    transmitFrame(queue, canard, tx_deadline_usec, &my_message_transfer_id, 1234, 11, payload2);
    */
    return 0;
}