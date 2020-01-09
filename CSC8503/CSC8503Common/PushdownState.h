#pragma once
#include "State.h"

namespace NCL {
	namespace CSC8503 {

		typedef std::function<void()> PushState;

		class PushdownState //:public State
		{
		public:
			enum PushdownResult {
				Push, Pop, NoChange
			};
			PushdownState(PushState somefunc,int stateNum);
			~PushdownState();
			
			PushState somefunc;
			int stateNum;
			bool isActive;

			PushdownResult PushdownUpdate(PushdownState** pushResult);

			virtual void OnAwake() {
				if (somefunc)
				{
					isActive = 1;
				}
			} //By default do nothing
			virtual void OnSleep() {
				isActive = 0;
			} //By default do nothing
		};
	}
}

