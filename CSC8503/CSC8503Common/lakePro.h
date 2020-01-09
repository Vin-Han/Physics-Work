#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class lakePro :public GameObject {
		public:
			lakePro(string name);
			GameObject* lake;
			int lakeState;
		};
	}
}
