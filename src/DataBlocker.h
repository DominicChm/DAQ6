//
// Created by Domo2 on 10/20/2021.
//
#include <queue.h>
#include "ChRt.h"
#include "Arduino.h"

#ifndef DAQ6_DATABLOCKER_H
#define DAQ6_DATABLOCKER_H

// "Blocks" data into chunks that are then handleable at your leisure.
template<typename T, size_t block_size, size_t num_blocks>
class DataBlocker {

private:
    Queue<uint8_t *> full_blocks = Queue<T *>(num_blocks);
    Queue<uint8_t *> empty_blocks = Queue<T *>(num_blocks);
    MUTEX_DECL(queue_mtx);

    T *current_block;
    size_t block_head{};
public:
    DataBlocker() {
        //Allocate blocks on class creation.
        for (int i = 0; i < num_blocks; i++)
            empty_blocks.push(new T[block_size]);

        current_block = empty_blocks.pop();
    }

    void write(T value) {
        this->current_block[block_head++] = value;

        if (block_head >= block_size) {
            //If full, move the current block pointer to full blocks queue.

            chMtxLock(queue_mtx);
            if (full_blocks.count() < num_blocks) {
                full_blocks.push(this->current_block);
                this->current_block = empty_blocks.pop();
            } else
                Serial.println("BUFFER OVERRUN - DATA LOST!!!");

            this->block_head = 0;
            chMtxUnlock(queue_mtx);
        }
    }

    void write(T *value, size_t size) {
        for (int i = 0; i < size; i++)
            this->write(value[i]);
    }

    size_t available() {
        chMtxLock(queue_mtx);
        return full_blocks.count();
        chMtxUnlock(queue_mtx);
    }

    size_t size() {
        return block_size;
    }

    T *readBlock() {
        chMtxLock(queue_mtx);
        T *block = full_blocks.pop();
        empty_blocks.push(block);
        chMtxUnlock(queue_mtx);
    }
};

#endif //DAQ6_DATABLOCKER_H
