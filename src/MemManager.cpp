#include <Arduino.h>
#include "MemManager.h"

BlockMemManager::BlockMemManager(uint8_t* buf, uint32_t bufferSize, uint32_t blockSize, uint32_t blockCount) {
    BUFFER = buf;
    BUFFER_SIZE = bufferSize;
    
    BLOCK_COUNT = blockCount;
    BLOCK_SIZE = blockSize;
}