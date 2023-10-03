#include <stdio.h>
#include <stdlib.h>
#include "canard.h"
#include "funccanard.h"

uint8_t isBusIdle();

uint8_t transmitOnBus (uint8_t* buffer, int buffer_size, uint8_t payload_size, uint8_t* overloadFlag, uint8_t* errorTransmitterFlag, uint8_t* errorReceiverFlag);

uint8_t readFrameOnBus (uint8_t* buffer, int buffer_size, uint8_t* payload_size, uint8_t* nbStuffedBits, uint8_t* overloadFlag, uint8_t* errorTransmitterFlag, uint8_t* errorReceiverFlag);
