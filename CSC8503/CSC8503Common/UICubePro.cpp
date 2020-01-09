#include "UICubePro.h"
namespace NCL {
	namespace CSC8503 {
		UICubePro::UICubePro(string name) {
			UICube = new GameObject(name);
			uifunc = 0;
			useFunction = 0;
		}
	}
}