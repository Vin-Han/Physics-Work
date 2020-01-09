#include "PushdownState.h"

using namespace NCL::CSC8503;

PushdownState::PushdownState(PushState somefunc, int stateNum)
{
	this->somefunc = somefunc;
	this->stateNum = stateNum;
	isActive = 0;
}


PushdownState::~PushdownState()
{
}

PushdownState::PushdownResult PushdownState::PushdownUpdate(PushdownState** pushResult) {
	if (stateNum == (*pushResult)->stateNum)
	{
		return PushdownResult::Push;
	}
	else
	{
		return PushdownResult::Pop;
	}

}