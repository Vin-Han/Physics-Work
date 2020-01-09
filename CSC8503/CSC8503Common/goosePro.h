#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class goosePro :public GameObject {
		public:
			goosePro(string name);
			GameObject* goose;
			int pointNum;
			int gooseNum;
			int appleNum;
		};
	}
}