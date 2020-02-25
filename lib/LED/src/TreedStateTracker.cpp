#include "TreedStateTracker.h"


TreedStateTracker::TreedStateTracker(int stackSize = 10)
{
    this->stackSize = stackSize;
    stateStack = new int[stackSize];
}

TreedStateTracker::~TreedStateTracker()
{
    delete[] stateStack;
}

int TreedStateTracker::pop() {
    stateHead--;
    return stateStack[stateHead];
}

int TreedStateTracker::current() {
    return stateStack[stateHead - 1];
}

void TreedStateTracker::push(int newState) {
    stateStack[stateHead] = newState;
    stateStack++;
}

void TreedStateTracker::reset() {
    stateHead = 0;
}

