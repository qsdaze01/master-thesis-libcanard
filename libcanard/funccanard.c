#include <stdio.h>
#include <stdlib.h>
#include "canard.h"
#include <math.h>

void* memAllocate(CanardInstance* const canard, const size_t amount)
{
    (void) canard;
    return malloc(amount);//o1heapAllocate(my_allocator, amount);
}

void memFree(CanardInstance* const canard, void* const pointer)
{
    (void) canard;
    //o1heapFree(my_allocator, pointer);
    free(pointer);
}

uint16_t computeCRC(int payload_size, uint8_t * buffer_frame) {
    uint8_t byte_buffer[3 + payload_size];
    for (int i = 0; i < 3 + payload_size; i++){
        byte_buffer[i] = 0;
    }

    uint8_t start_frame_size = 19 + 8*payload_size;
    int temp = 0;
    for (int i = 0; i < start_frame_size; i++){
        if (i%8 == 0 && i != 0){
            temp++;
        }
        byte_buffer[temp] += buffer_frame[i]*pow(2,7-i%8);
    }
    uint16_t CRC = crcAdd(0, 3 + payload_size, byte_buffer);
    return CRC;
}

uint8_t insertStuffedBits (uint8_t* buffer_in, uint8_t* buffer_out, uint8_t payload_size) {
    uint8_t nb_stuffed_bits = 0;
    uint8_t count_successive_1 = 0;
    uint8_t count_successive_0 = 0;
    for (int i = 0; i < 44 + 8*payload_size; i++) {
        if (buffer_in[i] == 0) {
            count_successive_1 = 0;
            count_successive_0++;
        } 
        if (buffer_in[i] == 1) {
            count_successive_0 = 0;
            count_successive_1++;
        }
        buffer_out[i + nb_stuffed_bits] = buffer_in[i];
        if (count_successive_0 == 5 || count_successive_1 == 5) {
            nb_stuffed_bits++;
            count_successive_0 = 0;
            count_successive_1 = 0;
            if (buffer_in[i] == 0) {
                buffer_out[i + nb_stuffed_bits] = 1;
            } else {
                buffer_out[i + nb_stuffed_bits] = 0;
            }
        }
    }
    return nb_stuffed_bits;
}

uint8_t removeStuffedBits(uint8_t* buffer_in, uint8_t* buffer_out, uint8_t buffer_size){
    uint8_t nb_stuffed_bits = 0;
    uint8_t count_successive_1 = 0;
    uint8_t count_successive_0 = 0;
    for (int i = 0; i < buffer_size; i++) {
        buffer_out[i] = buffer_in[i + nb_stuffed_bits];
        if (buffer_in[i + nb_stuffed_bits] == 0) {
            count_successive_1 = 0;
            count_successive_0++;
        } 
        if (buffer_in[i + nb_stuffed_bits] == 1) {
            count_successive_0 = 0;
            count_successive_1++;
        }
        if (count_successive_0 == 5) {
            count_successive_0 = 0;
            count_successive_1 = 0;
            nb_stuffed_bits++;
        } 
        if (count_successive_1 == 5) {
            count_successive_0 = 0;
            count_successive_1 = 0;
            nb_stuffed_bits++;
        }
    }
    return nb_stuffed_bits;
}

/*
    SOF : 1 bit Dominant
    Identifier : 11 bits
    RTR : 1 bit Dominant in data frames, Recessive in remote frames
    IDE : 1 bit Recessive for extended format, Dominant for standard format
    r : 1 bit Dominant
    DLC : 4 bits
    Data field : 0 - 64 bits
    CRC : 15 bits
    DEL : 1 bit Recessive
    ACK : 1 bit
    DEL : 1 bit Recessive
    EOF : 7 bits Recessive
*/
void serializeFrame(int payload_size, uint8_t* payload, uint16_t identifier, uint8_t RTR, uint8_t* buffer) {
    uint8_t buffer_size = 44 + payload_size*8;
    uint16_t CRC;

    // SOF
    buffer[0] = 0;
    // Identifier
    for (int i = 10; i >= 0; i--) {
        uint16_t mask = 1 << i;
        uint8_t bit = (identifier & mask) ? 1 : 0;
        buffer[11 - i] = bit;
    }
    //RTR 
    buffer[12] = RTR;
    //IDE
    buffer[13] = 0;
    //r
    buffer[14] = 0;
    //DLC
    for (int i = 3; i >= 0; i--) {
        uint16_t mask = 1 << i;
        uint8_t bit = (payload_size & mask) ? 1 : 0;
        buffer[18 - i] = bit;
    }
    //Data field
    for (int j = 0; j < payload_size; j++) {
        for (int i = 8; i >= 0; i--) {
            uint16_t mask = 1 << i;
            uint8_t bit = (payload[j] & mask) ? 1 : 0;
            buffer[19 + 8*payload_size - i - 8*j] = bit;
        }
    }
    //CRC
    CRC = computeCRC(payload_size, buffer);
    for (int i = 14; i >= 0; i--) {
        uint16_t mask = 1 << i;
        uint8_t bit = (CRC & mask) ? 1 : 0;
        buffer[33 + 8*payload_size - i] = bit;
    }
    //DEL
    buffer[34 + 8*payload_size] = 1;
    //ACK
    buffer[35 + 8*payload_size] = 1;
    //DEL 
    buffer[36 + 8*payload_size] = 1;
    //EOF
    for (int i = 0; i < 7; i++){
        buffer[37 + 8*payload_size + i] = 1;
    }
    //Interframe space 
    for (int i = 0; i < 3; i++){
        buffer[44 + 8*payload_size + i] = 1;
    }
}

uint8_t deserializeFrame(uint8_t *buffer, uint16_t* identifier, uint8_t buffer_size){
    uint8_t payload_size = (buffer_size - 44)/8;
    uint16_t CRC_computed = computeCRC(payload_size, buffer);

    for (int i = 0; i < 11; i++) {
        *identifier += buffer[1 + i]*pow(2,10-i);
    }

    uint64_t data = 0;
    for (int i = 0; i < 8*payload_size; i++){
        data += buffer[19 + i]*pow(2,10-i);
    }

    uint16_t CRC_received = 0;
    for (int i = 0; i < 15; i++){
        CRC_received += buffer[19 + 8*payload_size + i]*pow(2,14-i);
    }

    if (!(CRC_computed == CRC_received || CRC_computed - pow(2,15) == CRC_received)) {
        printf("error \n");
        return -1;
    }
    return 0;
}

int pleaseTransmit(CanardTxQueueItem* ti, CanardTransferMetadata transfer_metadata) {
    const uint8_t* payload_data = (const uint8_t*)ti->frame.payload;
    uint8_t buffer_frame[44 + ti->frame.payload_size*8];

    serializeFrame(ti->frame.payload_size, payload_data, transfer_metadata.port_id, 1, buffer_frame);

    uint8_t buffer_stuffed[44 + ti->frame.payload_size*8 + 21];
    uint8_t stuffed_bits = insertStuffedBits(buffer_frame, buffer_stuffed, ti->frame.payload_size);
    uint8_t frame_len = 44 + ti->frame.payload_size*8 + stuffed_bits;

    return 0;
}

int transmitFrame(CanardTxQueue queue, CanardInstance canard, uint8_t tx_deadline_usec, uint8_t *my_message_transfer_id, int port_id, int payload_size, char* payload) {
    const CanardTransferMetadata transfer_metadata = {
        .priority       = CanardPriorityNominal,
        .transfer_kind  = CanardTransferKindMessage,
        .port_id        = port_id,                       // This is the subject-ID.
        .remote_node_id = CANARD_NODE_ID_UNSET,       // Messages cannot be unicast, so use UNSET.
        .transfer_id    = *my_message_transfer_id,
    };
    ++*my_message_transfer_id;  // The transfer-ID shall be incremented after every transmission on this subject.
    int32_t result = canardTxPush(&queue,               // Call this once per redundant CAN interface (queue).
                                &canard,
                                tx_deadline_usec,     // Zero if transmission deadline is not limited.
                                &transfer_metadata,
                                payload_size,                   // Size of the message payload (see Nunavut transpiler).
                                payload);
    if (result < 0)
    {
        // An error has occurred: either an argument is invalid, the TX queue is full, or we've run out of memory.
        // It is possible to statically prove that an out-of-memory will never occur for a given application if the
        // heap is sized correctly; for background, refer to the Robson's Proof and the documentation for O1Heap.
        printf("Error \n");
        return 1;
    }

    for (const CanardTxQueueItem* ti = NULL; (ti = canardTxPeek(&queue)) != NULL;)  // Peek at the top of the queue.
    {
        if (!pleaseTransmit(ti, transfer_metadata))               // Send the frame over this redundant CAN iface.
        {
            break;                             // If the driver is busy, break and retry later.
        }
        // After the frame is transmitted or if it has timed out while waiting, pop it from the queue and deallocate:
        canard.memory_free(&canard, canardTxPop(&queue, ti));
    }
    return 0;
}
