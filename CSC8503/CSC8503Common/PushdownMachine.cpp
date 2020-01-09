#include "PushdownMachine.h"
#include "PushdownState.h"
using namespace NCL::CSC8503;

PushdownMachine::PushdownMachine()
{
	activeState = nullptr;
}

PushdownMachine::~PushdownMachine()
{
}

void PushdownMachine::Update(PushdownState* compareState) {
	if (activeState) {
		PushdownState* newState = nullptr;
		PushdownState::PushdownResult result = activeState->PushdownUpdate(&compareState);

		switch (result) {
			case PushdownState::Pop: {
				activeState->OnSleep();
				stateStack.pop();
				if (stateStack.empty()) {
					activeState = nullptr; //??????
				}
				else {
					newState = stateStack.top();
					newState->OnAwake();
				}
			}break;
			case PushdownState::Push: {
				activeState->OnSleep();
				//activeState = compareState;
				//stateStack.push(compareState);
				//compareState->OnAwake();

			}break;
		}
	}
	else{
		compareState->OnAwake();
		activeState = compareState ;
	}
}