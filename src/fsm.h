//
// Created by Domo2 on 10/20/2021.
//

#ifndef DAQ6_FSM_H
#define DAQ6_FSM_H

#define SET_STATE(newstate) {lastState = state; state=newstate;}
#define SET_STATE_IF(conditional, newstate) {if(conditional){SET_STATE(newstate); break;}}
#define SET_STATE_EXEC_IF(conditional, newstate, execute) {if(conditional){SET_STATE(newstate); execute; break;}}
#define ON_STATE_ENTER(execute) {if(state != lastState) {execute; SET_STATE(state);} }


#endif //DAQ6_FSM_H
