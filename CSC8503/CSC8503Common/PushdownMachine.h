#pragma once
#include <stack>

namespace NCL {
	namespace CSC8503 {
		class PushdownState;

		class PushdownMachine
		{
		public:
			PushdownMachine();
			~PushdownMachine();

			void Update(PushdownState* compareState);

		protected:

			PushdownState * activeState;

			std::stack<PushdownState*> stateStack;

		};
	}
}

