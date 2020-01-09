#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class targetCube :public GameObject {
		public:
			targetCube(string name);
			GameObject* target;
			int enemyNum;
		};
	}
}