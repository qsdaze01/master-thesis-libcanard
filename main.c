#include <stdio.h>
#include <stdlib.h>
#include "libcanard/canard.h"
#include "libcanard/funccanard.h"

//uint8_t lock __attribute__((section(".my_section1"), aligned(1))) = 24;
//uint8_t bus __attribute__((section(".my_section2"), aligned(1))) = 23;

int main() {
    /*
    uint8_t * lock = malloc(sizeof(1));
    printf("%p \n", lock);

    uint8_t * bus = malloc(sizeof(1));
    printf("%p \n", bus);

    int *lock = mmap (address_lock, 1, PROT_NONE, MAP_FIXED | MAP_ANONYMOUS, 0, 0);

    if (lock == MAP_FAILED) {
        perror("Error : mapping lock");
        exit(1);
    }

    int *bus = mmap (address_bus, 1, PROT_NONE, MAP_FIXED | MAP_ANONYMOUS, 0, 0);

    if (bus == MAP_FAILED) {
        perror("Error : mapping bus");
        exit(1);
    }

    lock = 26;
    bus = 47;
    
    
    uint8_t buffer[44];
    serializeFrame(0, NULL, 11, 1, buffer);
    uint16_t identifer = 0;
    uint64_t data = 0;
    deserializeFrame(buffer, &identifer, 44, &data);
    */
    
    CanardInstance canard = canardInit(&memAllocate, &memFree);
    canard.node_id = 42;                        // Defaults to anonymous; can be set up later at any point.

    CanardTxQueue queue = canardTxInit(100,                 // Limit the size of the queue at 100 frames.
                                   CANARD_MTU_CAN_FD);  // Set MTU = 64 bytes. There is also CANARD_MTU_CAN_CLASSIC.


    uint8_t tx_deadline_usec = 0;

    static uint8_t my_message_transfer_id;  // Must be static or heap-allocated to retain state between calls.

    char* payload = "\x2D\x00" "Sanchoe, it strikes me thou art in great fear.";

    transmitFrame(queue, canard, tx_deadline_usec, &my_message_transfer_id, 1234, 48, payload);

    //char* payload2 = "Hello there";

    //transmitFrame(queue, canard, tx_deadline_usec, &my_message_transfer_id, 1234, 11, payload2);
    
    return 0;
}