#include <stdio.h>
#include <stdlib.h>
#include "canard.h"
#include <math.h>
#include "funccanard.h"
#include <time.h> 

uint8_t temp_lock = 0; /* VARIABLE DE LOCK TEMPORAIRE EN ATTENDANT QUE LA COMMUNICATION MARCHE */
uint8_t temp_bus = 1; /* VARIABLE DE BUS TEMPORAIRE EN ATTENDANT QUE LA COMMUNICATION MARCHE */
uint64_t temp_count = 0; /* VARIABLE DE COMPTEUR TEMPORAIRE EN ATTENDANT QUE LA COMMUNICATION MARCHE */
clock_t temp_time_bit = 10000; /* VARIABLE DE TEMPS BIT TEMPORAIRE EN ATTENDANT QUE LA COMMUNICATION MARCHE */

uint8_t bufferIdle[10];
uint8_t bufferStart[11];
uint8_t bufferDLC[4];

uint8_t isBusIdle() {
    uint8_t busRead = 0;
    while (!busRead) {
        if (temp_lock != 0) {
            continue;
        }
        temp_lock = 1;
        for (int i = 0; i < 9; i++) {
            bufferIdle[i] = bufferIdle[i+1];
        }
        bufferIdle[9] = temp_bus;

        for (int i = 0; i < 10; i++) {
            if (bufferIdle[i] == 0) {
                temp_lock = 0;
                return 1;
            }
        }
        temp_lock = 0;
        busRead = 1;
    }
    return 0;
}

uint8_t endTimeBit(clock_t startTimeBit) {
    clock_t time = clock();
    if (time - startTimeBit < temp_time_bit) {
        return 1;
    }
    return 0;
}

uint8_t endHalfBit(clock_t startTimeBit) {
    clock_t time = clock();
    if (time - startTimeBit < temp_time_bit/2) {
        return 1;
    }
    return 0;
}

uint8_t sendBit (uint8_t* buffer, int buffer_size, int index, int limit) {
    if (index < limit) {
        uint8_t sent = 0;
        while (!sent) {
            if (temp_lock != 0) {
                continue;
            }
            temp_lock = 1;
            if (temp_bus == 1) {
                temp_bus = buffer[index];
            }
            temp_lock = 0;
            sent = 1;
        }
        return 1;
    } else {
        return 0;
    }
}

uint8_t nbStuffedBitsCount (uint8_t* buffer, int buffer_size, int index) {
    uint8_t count_0 = 0;
    uint8_t count_1 = 0;
    uint8_t nbStuffedBits = 0;
    for (int i = 0; i <= index; i++) {
        if (buffer[i] == 0) {
            count_1 = 0;
            count_0++;
            if (count_0 == 5) {
                nbStuffedBits++;
                count_0 = 0;
            }
        } else {
            count_0 = 0;
            count_1++;
            if (count_1 == 5) {
                nbStuffedBits++;
                count_1 = 0;
            }
        }
    }
    return nbStuffedBits;
}

uint8_t testErrorFixBits (uint8_t* buffer, int buffer_size, int index, uint8_t nbStuffedBits, uint8_t diffStuffedBits, uint8_t payload_size) {
    if (index == 0) {
        if (buffer[index] == 1) { 
            return 1;
        }
    }
    if (index == 13 + nbStuffedBits - diffStuffedBits) {
        if (buffer[index] == 1) {
            return 1;
        }
    }
    if (index == 14 + nbStuffedBits - diffStuffedBits) {
        if (buffer[index] == 1) {
            return 1;
        }
    }
    if (index == 34 + nbStuffedBits - diffStuffedBits + payload_size) {
        if (buffer[index] == 0) {
            return 1;
        }
    }
    if (index == 36 + nbStuffedBits - diffStuffedBits + payload_size) {
        if (buffer[index] == 0) {
            return 0;
        }
    }

    return 0;
}

uint8_t testConsecutiveBits (uint8_t* buffer, int buffer_size, int index) {
    uint8_t count_0 = 0;
    uint8_t count_1 = 0;
    for (int i = 0; i <= index; i++) {
        if (buffer[i] == 0) {
            count_1 = 0;
            count_0++;
            if (count_0 == 6) {
                return 1;
            }
        } else {
            count_0 = 0;
            count_1++;
            if (count_1 == 6) {
                return 1;
            }
        }
    }
    return 0;
}

uint8_t arbitrationLost (uint8_t* buffer, int buffer_size, int index) {
    uint8_t isBusRead = 0;
    while (!isBusRead) {
        if (temp_lock != 0) {
            continue;
        }
        temp_lock = 1;
        if (buffer[index] != temp_bus) {
            temp_lock = 0;
            return 1;
        }
        temp_lock = 0;
        isBusRead = 1;
    }
    return 0;
}

uint8_t testCorrectBit (uint8_t* buffer, int buffer_size, int index) {
    uint8_t isBusRead = 0;
    while(!isBusRead) {
        if (temp_lock != 0) 
            continue;
        temp_lock = 1;
        if (buffer[index] != temp_bus) {
            temp_lock = 0;
            return 1;
        }
        temp_lock = 0;
        isBusRead = 1;
    }
    return 0;
}

uint8_t testCRC (uint8_t* buffer, int buffer_size, int index, uint8_t nbStuffedBits, uint8_t payload_size) {
    uint8_t buffer_out[index+1];
    removeStuffedBits(buffer, buffer_out, index-nbStuffedBits);    
    uint16_t CRC_computed = computeCRC(payload_size, buffer_out);

    uint16_t CRC_received = 0;
    for (int i = 0; i < 15; i++){
        CRC_received += buffer_out[19 + payload_size + i]*pow(2,14-i);
    }

    if (!(CRC_computed == CRC_received || CRC_computed - pow(2,15) == CRC_received)) {
        printf("Error : CRC not equal, frame wrong \n");
        return 1;
    }
    return 0;
}

uint8_t transmitOnBus (uint8_t* buffer, int buffer_size, uint8_t payload_size, uint8_t* overloadFlag, uint8_t* errorTransmitterFlag, uint8_t* errorReceiverFlag) {
    int index = 0;
    uint8_t nbStuffedBits = 0;
    uint8_t lastNbStuffedBits = 0;
    clock_t startBit;
    for (int i = 0; i < 10; i++) {
        bufferIdle[i] = 0;
    }

    while(isBusIdle()) {
        startBit = clock();
        while (endTimeBit(startBit)) {
            continue;
        }
    }
    printf("Idle phase passed \n");
    /* Arbitration */
    int limit_arbitration = 12;
    while (sendBit(buffer, buffer_size, index, limit_arbitration)) {
        startBit = clock();
        lastNbStuffedBits = nbStuffedBits;
        nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_arbitration = 12 + nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, nbStuffedBits, nbStuffedBits-lastNbStuffedBits, payload_size)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (arbitrationLost (buffer, buffer_size, index)) {
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }
    
    printf("Arbitration passed : %d \n", index);

    /* Second Part */
    int limit_second_part = 34 + payload_size + nbStuffedBits;
    while (sendBit(buffer, buffer_size, index, limit_second_part)) {
        startBit = clock();
        lastNbStuffedBits = nbStuffedBits;
        nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_second_part = 34 + payload_size + nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, nbStuffedBits, nbStuffedBits-lastNbStuffedBits, payload_size)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        while (endHalfBit(startBit)){
            continue;
        }
        if (testCorrectBit (buffer, buffer_size, index)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }
    
    printf("Second Part passed : %d %d \n", index, nbStuffedBits);

    if (testCRC (buffer, buffer_size, index, nbStuffedBits, payload_size)) {
        *errorTransmitterFlag = 8;
        return 1;
    }

    printf("CRC passed \n");

    /* Third Part */
    int limit_third_part = 36 + payload_size + nbStuffedBits;
    uint8_t ACK = 1; 
    while (sendBit(buffer, buffer_size, index, limit_third_part)) {
        startBit = clock();
        lastNbStuffedBits = nbStuffedBits;
        nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_third_part = 36 + payload_size + nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, nbStuffedBits, nbStuffedBits-lastNbStuffedBits, payload_size)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        while (endHalfBit(startBit)){
            continue;
        }
        if (testCorrectBit (buffer, buffer_size, index)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            ACK = temp_bus;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }
    printf("Third part passed : %d \n", index);
    
    /* A ENLEVER DECOMMENTER LORS D'UNE COMMUNICATION */
    /*
    if (ACK != 0) {
        *errorTransmitterFlag = 8;
        return 1;
    }
    */

    printf("ACK OK \n");

    int limit_last_part = 37 + payload_size + nbStuffedBits;
    while (sendBit(buffer, buffer_size, index, limit_last_part)) {
        startBit = clock();
        lastNbStuffedBits = nbStuffedBits;
        nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_last_part = 37 + payload_size + nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, nbStuffedBits, nbStuffedBits-lastNbStuffedBits, payload_size)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        while (endHalfBit(startBit)){
            continue;
        }
        if (testCorrectBit (buffer, buffer_size, index)) {
            *errorTransmitterFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    /* EOF */
    uint8_t isBusWritten;
    uint8_t correctBit;
    for (int i = 0; i < 7; i++) {
        startBit = clock();
        isBusWritten = 0;
        while(!isBusWritten) {
            if (temp_lock != 0) {
                continue;
            }
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            isBusWritten = 1;
        }
        while (endHalfBit(startBit)){
            continue;
        }
        correctBit = 0;
        while (!correctBit) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            if (temp_bus != 1) {
                *errorReceiverFlag = 1;
                return 1;
            }
            temp_lock = 0;
            correctBit = 1;
        }
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    /* Interframe */
    for (int i = 0; i < 3; i++) {
        startBit = clock();
        isBusWritten = 0;
        while(!isBusWritten) {
            if (temp_lock != 0) {
                continue;
            }
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            isBusWritten = 1;
        }
        while (endHalfBit(startBit)){
            continue;
        }
        correctBit = 0;
        while (!correctBit) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            if (temp_bus != 1) {
                *overloadFlag = 1;
                return 0;
            }
            temp_lock = 0;
            correctBit = 1;
        }
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }
    return 0;
}

uint8_t readStartFrame() {
    uint8_t busRead = 0;
    while (!busRead) {
        if (temp_lock != 0) {
            continue;
        }
        temp_lock = 1;
        for (int i = 0; i < 10; i++) {
            bufferStart[i] = bufferStart[i+1];
        }
        bufferStart[10] = temp_bus;

        for (int i = 0; i < 10; i++) {
            if (bufferStart[i] == 0) {
                temp_lock = 0;
                return 1;
            }
        }
        if (bufferStart[10] == 1) {
            temp_lock = 0;
            return 1;
        }
        temp_lock = 0;
        busRead = 1;
    }
    return 0;
}

uint8_t getDLC(uint8_t* buffer, int buffer_size, int index, uint8_t nbStuffedBits, uint8_t diffStuffedBits) {
    if (index == 15 + nbStuffedBits - diffStuffedBits) {
        bufferDLC[0] = buffer[index];
    }
    if (index == 16 + nbStuffedBits - diffStuffedBits) {
        bufferDLC[1] = buffer[index];
    }
    if (index == 17 + nbStuffedBits - diffStuffedBits) {
        bufferDLC[2] = buffer[index];
    }
    if (index == 18 + nbStuffedBits - diffStuffedBits) {
        bufferDLC[3] = buffer[index];
    }
    return 0;
}

uint8_t computeDLC() {
    uint8_t DLC = 0;
    DLC += bufferDLC[0]*8;
    DLC += bufferDLC[1]*4;
    DLC += bufferDLC[2]*2;
    DLC += bufferDLC[3];

    return DLC;
}

uint8_t receiveBit(uint8_t* buffer, int buffer_size, int index, int limit, clock_t startBit) {
    if (index < limit) {
        while (endHalfBit(startBit)){
            continue;
        }
        uint8_t received = 0;
        while (!received) {
            if (temp_lock != 0) {
                continue;
            }
            temp_lock = 1;
            buffer[index] = temp_bus;
            temp_lock = 0;
            received = 1;
        }
        return 1;
    } else {
        return 0;
    }
}

uint8_t sendACK(uint8_t* buffer, int buffer_size, int index, int limit, clock_t startBit) {
    if (index < limit) {
        while (endHalfBit(startBit)){
            continue;
        }
        uint8_t sent = 0;
        while (!sent) {
            if (temp_lock != 0) {
                continue;
            }
            temp_lock = 1;
            buffer[index] = 0;
            temp_bus = 0;
            temp_lock = 0;
            sent = 1;
        }
        return 1;
    } else {
        return 0;
    }
}

uint8_t readFrameOnBus (uint8_t* buffer, int buffer_size, uint8_t* payload_size, uint8_t* nbStuffedBits, uint8_t* overloadFlag, uint8_t* errorTransmitterFlag, uint8_t* errorReceiverFlag) {
    int index = 0;
    *nbStuffedBits = 0;
    uint8_t lastNbStuffedBits = 0;
    clock_t startBit;
    uint8_t firstLoop;

    for (int i = 0; i < 11; i++) {
        bufferStart[i] = 0;
    }
    
    while(readStartFrame()) {
        startBit = clock();
        while (endTimeBit(startBit)) {
            continue;
        }
    }

    /* First part */
    int limit_first_part = 15 + *nbStuffedBits;
    firstLoop = 1;
    startBit = clock();
    while (receiveBit(buffer, buffer_size, index, limit_first_part, startBit)) {
        if (firstLoop) {
            firstLoop = 0;
            startBit = clock();
        }
        lastNbStuffedBits = *nbStuffedBits;
        *nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_first_part = 15 + *nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits, 255)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    for (int i = 0; i < 4; i++) {
        bufferDLC[i] = 0;
    }

    /* Data Length Code (DLC) */
    int limit_DLC = 19 + *nbStuffedBits;
    firstLoop = 1;
    startBit = clock();
    while (receiveBit(buffer, buffer_size, index, limit_DLC, startBit)) {
        if (firstLoop) {
            firstLoop = 0;
            startBit = clock();
        }
        lastNbStuffedBits = *nbStuffedBits;
        *nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_DLC = 19 + *nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits, 255)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        getDLC(buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits);
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }
    
    *payload_size = 8*computeDLC();

    /* Data + CRC */
    int limit_second_part = 34 + *payload_size + *nbStuffedBits;
    firstLoop = 1;
    startBit = clock();
    while (receiveBit(buffer, buffer_size, index, limit_second_part, startBit)) {
        if (firstLoop) {
            firstLoop = 0;
            startBit = clock();
        }
        lastNbStuffedBits = *nbStuffedBits;
        *nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_second_part = 34 + *payload_size + *nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits, *payload_size)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    if (testCRC (buffer, buffer_size, index, *nbStuffedBits, *payload_size)) {
        *errorTransmitterFlag = 8;
        return 1;
    }

    /* CRC delimiter */
    int limit_CRC_delimiter = 35 + *payload_size + *nbStuffedBits;
    firstLoop = 1;
    startBit = clock();
    while (receiveBit(buffer, buffer_size, index, limit_CRC_delimiter, startBit)) {
        if (firstLoop) {
            firstLoop = 0;
            startBit = clock();
        }
        lastNbStuffedBits = *nbStuffedBits;
        *nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_CRC_delimiter = 35 + *payload_size + *nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits, *payload_size)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    /* ACK */
    int limit_ACK = 36 + *payload_size + *nbStuffedBits;
    startBit = clock();
    while (sendACK(buffer, buffer_size, index, limit_ACK, startBit)) {
        lastNbStuffedBits = *nbStuffedBits;
        *nbStuffedBits = nbStuffedBitsCount(buffer, buffer_size, index);
        limit_ACK = 36 + *payload_size + *nbStuffedBits;
        if (testErrorFixBits (buffer, buffer_size, index, *nbStuffedBits, *nbStuffedBits-lastNbStuffedBits, *payload_size)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        if (testConsecutiveBits (buffer, buffer_size, index)) {
            *errorReceiverFlag = 1;
            return 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    /* ACK delimiter + EOF */
    uint8_t correctBit;
    for (int i = 0; i < 8; i++) {
        startBit = clock();
        while (endHalfBit(startBit)){
            continue;
        }
        correctBit = 0;
        while (!correctBit) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            if (temp_bus != 1) {
                *errorReceiverFlag = 1;
                return 1;
            }
            temp_lock = 0;
            correctBit = 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    /* Interframe */
   for (int i = 0; i < 8; i++) {
        startBit = clock();
        while (endHalfBit(startBit)){
            continue;
        }
        correctBit = 0;
        while (!correctBit) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            if (temp_bus != 1) {
                *overloadFlag = 1;
                return 0;
            }
            temp_lock = 0;
            correctBit = 1;
        }
        index++;
        while (endTimeBit(startBit)) {
            continue;
        }
        uint8_t busAtOne = 0;
        while (!busAtOne) {
            if (temp_lock != 0) {
                continue;
            } 
            temp_lock = 1;
            temp_bus = 1;
            temp_lock = 0;
            busAtOne = 1;
        }
    }

    return 0;
}
