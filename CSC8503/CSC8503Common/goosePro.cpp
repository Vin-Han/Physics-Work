#include "goosePro.h"

namespace NCL {
	namespace CSC8503 {
		goosePro::goosePro(string name) {
			goose = new GameObject(name);
			appleNum = 0;
			pointNum = 0;
		}
	}
}