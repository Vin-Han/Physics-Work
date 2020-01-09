#include "enemyAI.h"
namespace NCL {
	namespace CSC8503 {
		enemyAI::enemyAI(string name) {
			enemy = new GameObject(name) ;
			enemyGooLength = 0;
			enemyOriLength = 0;
			enemyNexLength = 0;
			nextPos = Vector3();
		}
	}
}