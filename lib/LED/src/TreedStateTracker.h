#ifndef TREED_STATE_TRACKER_H
#define TREED_STATE_TRACKER_H

class TreedStateTracker
{
private:
    int *stateStack;
    int stackSize = 0;
    int stateHead = 0;
public:
    TreedStateTracker(int stackSize);
    ~TreedStateTracker();

    int pop();
    int current();
    void reset();
    void push(int newState);
};

#endif
