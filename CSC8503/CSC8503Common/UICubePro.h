#pragma once
#include "GameObject.h"
#include "PushdownState.h"
	namespace NCL {
	namespace CSC8503 {
		class UICubePro :public GameObject {
		public:
			UICubePro(string name);
			GameObject* UICube;
			int uifunc;
			bool useFunction;
			PushdownState* pushstate;
		};
	}
}
