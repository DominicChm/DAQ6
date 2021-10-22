//
// Created by Domo2 on 10/20/2021.
//
#include <queue.h>
#include "ChRt.h"
#include "Arduino.h"

#ifndef DAQ6_DATABLOCKER_H
#define DAQ6_DATABLOCKER_H

// "Blocks" data into chunks that are then handleable at your leisure.
template<typename T, int block_size, int num_blocks>
class DataBlocker {

private:
    Queue<T *> full_blocks = Queue<T *>(num_blocks);
    Queue<T *> empty_blocks = Queue<T *>(num_blocks);
    MUTEX_DECL(queue_mtx);
    int block_remainder{};
    T *current_block;
    int block_head{};
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

            chMtxLock(&queue_mtx);
            if (empty_blocks.count() > 1) {
                full_blocks.push(this->current_block);
                this->current_block = empty_blocks.pop();
            } else
                Serial.println("BUFFER OVERRUN - DATA LOST!!!");

            this->block_head = 0;
            chMtxUnlock(&queue_mtx);
        }
    }

    void write(T *value, int size) {
        for (int i = 0; i < size; i++)
            this->write(value[i]);
    }

    int available() {
        chMtxLock(&queue_mtx);
        int count = full_blocks.count();
        chMtxUnlock(&queue_mtx);
        return count;
    }

    int size() {
        return block_size;
    }

    T *checkout_block() {
        chMtxLock(&queue_mtx);
        T *block = full_blocks.pop();
        chMtxUnlock(&queue_mtx);
        return block;
    }

    void return_block(T *block) {
        chMtxLock(&queue_mtx);
        empty_blocks.push(block);
        chMtxUnlock(&queue_mtx);
    }

    T *checkout_remainder_block() {
        T *block = current_block;
        block_remainder = block_head;

        chMtxLock(&queue_mtx);
        this->current_block = empty_blocks.pop();
        block_head = 0;
        chMtxUnlock(&queue_mtx);
        return block;
    }

    int remainder_size() {
        return block_remainder;
    }
};

#endif //DAQ6_DATABLOCKER_H
