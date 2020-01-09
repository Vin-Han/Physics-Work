#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class applePro :public GameObject {
		public:
			applePro(string name);
			GameObject* apple;
			int appleState;
			int applePoint;
		};
	}
}
