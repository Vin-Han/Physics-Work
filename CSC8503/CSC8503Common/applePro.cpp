#include "applePro.h"
namespace NCL {
	namespace CSC8503 {
		applePro::applePro(string name) {
			apple = new GameObject(name);
			appleState = 0;
			applePoint = 1;
		}
	}
}