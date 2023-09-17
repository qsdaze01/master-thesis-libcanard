#include <stdio.h>
#include <stdlib.h>
#include "canard.h"

void* memAllocate(CanardInstance* const canard, const size_t amount);

void memFree(CanardInstance* const canard, void* const pointer);

uint8_t serializeFrame(int payload_size, uint8_t* payload, uint16_t identifier, uint8_t RTR, uint8_t* buffer);

uint8_t deserializeFrame(uint8_t *buffer, uint16_t* identifier, uint8_t buffer_size);

uint8_t insertStuffedBits (uint8_t* buffer_in, uint8_t* buffer_out, uint8_t payload_size);

uint8_t removeStuffedBits(uint8_t* buffer_in, uint8_t* buffer_out, uint8_t buffer_size);

uint8_t pleaseTransmit(CanardTxQueueItem* ti, CanardTransferMetadata transfer_metadata);

uint8_t transmitFrame(CanardTxQueue queue, CanardInstance canard, uint8_t tx_deadline_usec, uint8_t *my_message_transfer_id, int port_id, int payload_size, char* payload);
