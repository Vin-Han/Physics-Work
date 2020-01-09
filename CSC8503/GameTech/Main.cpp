#include "../../Common/Window.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"

#include "../CSC8503Common/NavigationGrid.h"

#include "NetworkedGame.h"
using namespace NCL;
using namespace CSC8503;

#include "TutorialGame.h"
#include <time.h>

int* GenRanMap(int mapLevel);

#define Map_Level 10

int main() {
	Window*w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);
	if (!w->HasInitialised()) {return -1;}	

	//TestNetworking();

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);
	int mapLevel = Map_Level;
	int* mapSeed = GenRanMap(mapLevel);

	//TutorialGame* g = new TutorialGame();
	TutorialGame* g = new TutorialGame(mapSeed , mapLevel);

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 1.0f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}
		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		g->UpdateGame(dt);

		if (g->quitGame == 1)
		{
			break;
		}
	}
	Window::DestroyGameWindow();
}

//random map generate
enum BlockType {
	UNSET_BLOCK = 0,
	FIRST_BLOCK = 1,
	SECOND_BLOCK = 2,
};
int* GenRanMap(int mapLevel) {
	srand((unsigned)time(NULL));
	int mapLength = mapLevel + 2;
	int maxSize = mapLength - 1;
	int* mapSize = new int[mapLength * mapLength];

	//initialize all componenet
	for (int i = 0; i < mapLength * mapLength; i++){
		mapSize[i] = UNSET_BLOCK;
	}
	//set border of the map
	for (int ver = 0; ver < mapLength; ver++){
		mapSize[ver * mapLength + maxSize] = FIRST_BLOCK;
		mapSize[ver * mapLength] = FIRST_BLOCK;
	}
	for (int hor = 1; hor < maxSize; hor++) {
		mapSize[mapLength * maxSize + hor] = FIRST_BLOCK;
		mapSize[hor] = FIRST_BLOCK;
	}
	//set random situation for all block
	for (int ver = 1; ver < maxSize; ver++){
		for (int hor = 1; hor < maxSize; hor++) {
			mapSize[ver * mapLength + hor] = rand() % 2;

			if (mapSize[ver * mapLength + hor] == FIRST_BLOCK) {
				if (mapSize[ver * mapLength + hor - 1] == SECOND_BLOCK ||
					mapSize[(ver-1) * mapLength + hor] == SECOND_BLOCK ||
					mapSize[ver * mapLength + hor + 1] == SECOND_BLOCK ||
					mapSize[(ver+1) * mapLength + hor] == SECOND_BLOCK ||
					mapSize[(ver + 1) * mapLength + hor + 1] == SECOND_BLOCK ||
					mapSize[(ver + 1) * mapLength + hor - 1] == SECOND_BLOCK ||
					mapSize[(ver - 1) * mapLength + hor + 1] == SECOND_BLOCK ||
					mapSize[(ver - 1) * mapLength + hor - 1] == SECOND_BLOCK
					) {
					mapSize[ver * mapLength + hor] = UNSET_BLOCK;
				}
				else if (mapSize[ver * mapLength + hor - 1] == FIRST_BLOCK ||
						mapSize[(ver - 1) * mapLength + hor] == FIRST_BLOCK ||
						mapSize[ver * mapLength + hor + 1] == FIRST_BLOCK ||
						mapSize[(ver + 1) * mapLength + hor] == FIRST_BLOCK
					) {
						 mapSize[ver * mapLength + hor] = SECOND_BLOCK;
					 }
			}
		}
	}
	//reverse check
#if 1
	for (int ver = 2; ver < maxSize - 1; ver++) {
		for (int hor = 2; hor < maxSize - 1; hor++) {
			if (mapSize[ver * mapLength + hor - 1] != UNSET_BLOCK &&
				mapSize[(ver - 1) * mapLength + hor] != UNSET_BLOCK &&
				mapSize[ver * mapLength + hor + 1] != UNSET_BLOCK &&
				mapSize[(ver + 1) * mapLength + hor] != UNSET_BLOCK) {
				for (;;) {
					int tempTest = rand() % 4;
					if (tempTest == 0) {
						if (hor - 1 > 0){
						mapSize[ver * mapLength + hor - 1] = UNSET_BLOCK;
						break;
						}
					}
					else if (tempTest == 1) { 
						if ((ver - 1) > 0){
							mapSize[(ver - 1) * mapLength + hor] = UNSET_BLOCK;
							break;
						}
						
					}
					else if (tempTest == 2) { 
						 if (hor + 1 < mapLength){
							mapSize[ver * mapLength + hor + 1] = UNSET_BLOCK;
							break;
						 }
						
					}
					else if (tempTest == 3) { 
						if ((ver + 1) < mapLength){
							mapSize[(ver + 1) * mapLength + hor] = UNSET_BLOCK;
							break;
						}
						
					}
				}
			}
		}
	}
#endif
	//print the map
	for (int tempver = 0; tempver < mapLength; tempver++) {
		for (int temphor = 0; temphor < mapLength; temphor++) {
			std::cout << mapSize[tempver * mapLength + temphor] << "   ";
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;
	return mapSize;
}