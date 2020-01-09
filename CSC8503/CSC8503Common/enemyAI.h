#pragma once
#include "GameObject.h"
#include "NavigationGrid.h"
namespace NCL {
	namespace CSC8503 {
		class enemyAI :public GameObject{
		public:
			enemyAI(string name);

			NavigationGrid* grid;
			Vector3 goosePos;
			Vector3 nextPos;
			GameObject* enemy;

			float enemyNexLength;
			float enemyGooLength;
			float enemyOriLength;

		};
	}
}
