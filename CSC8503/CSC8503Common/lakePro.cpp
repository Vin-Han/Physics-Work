#include "lakePro.h"
namespace NCL {
	namespace CSC8503 {
		lakePro::lakePro(string name) {
			lake = new GameObject(name);
			lakeState = 0;
		}
	}
}