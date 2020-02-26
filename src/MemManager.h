#include <Arduino.h>

class BlockMemManager
{
private:
    uint8_t* BUFFER;
    uint32_t BLOCK_SIZE;
    uint32_t BUFFER_SIZE;
    uint32_t BLOCK_COUNT;

    uint32_t totalReads = 0;
    uint32_t totalWrites = 0;
public:
    BlockMemManager(uint8_t* buf, uint32_t bufferSize, uint32_t blockSize, uint32_t blockCount);
    ~BlockMemManager();

    uint32_t getBlockSize();

    uint32_t blocksAvailible();
    

    //In both these cases, passed buf is assumed to be of size BLOCK_SIZE
    bool readBlock(uint8_t* buf);
    bool pushBlock(uint8_t* buf);


    bool pushData(uint8_t* buf, uint32_t size);
};
