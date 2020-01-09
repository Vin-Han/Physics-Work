#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/NavigationGrid.h"

#include "../CSC8503Common/StateMachine.h"
#include "../CSC8503Common/StateTransition.h"
#include "../CSC8503Common/State.h"

#include "../CSC8503Common/enemyAI.h"
#include "../CSC8503Common/applePro.h"
#include "../CSC8503Common/goosePro.h"
#include "../CSC8503Common/lakePro.h"
#include "../CSC8503Common/targetCube.h"
#include "../CSC8503Common/UICubePro.h"
#include "../CSC8503Common/PositionConstraint.h"

#include "../CSC8503Common/PushdownMachine.h"
#include "../CSC8503Common/PushdownState.h"

#include "../CSC8503Common/NetWookPro.h"
#include "../CSC8503Common/GameServer.h"
#include "../CSC8503Common/GameClient.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();
			TutorialGame(int* mapSeed ,int mapLevel);

			virtual void UpdateGame(float dt);

			bool quitGame;

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			
			void InitDIYWorld(float rowSpacing, float colSpacing, const Vector3& cubeDims, int* mapSeed, int mapLevel, float scaleNum);

			void BridgeConstraintTest();
			void SimpleGJKTest();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void LockedCameraMovement();

			void DrawBaseLine();
			void BindCameraToGoose();
			void WriteMapToFile();
			void AppleDirCheck();
			void EnemyAttackCheck();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			//IT'S HAPPENING
			GameObject* AddGooseToWorld(const Vector3& position, float gooseScale);
			GameObject* AddParkKeeperToWorld(const Vector3& position);
			GameObject* AddCharacterToWorld(const Vector3& position, float characterScale);
			GameObject* AddAppleToWorld(const Vector3& position, float appleScale);


			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool GamingMode;
			bool cameraMode;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;
			Vector4 originalColor;

			targetCube* target = nullptr;
			targetCube* AddTargetCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);

			goosePro* gooseOBJ = nullptr;
			goosePro* AddGooseProToWorld(const Vector3& position, float gooseScale);

			vector<applePro*> appleArray;
			applePro* AddAppleProToWorld(const Vector3& position, float appleScale);

			vector<enemyAI*> enemyArray;
			vector<StateMachine*> enemyStateMachine;
			enemyAI* AddEnemyToWorld(const Vector3& position, float characterScale);

			vector<lakePro*> lakeArray;
			vector<lakePro*> groundArray;
			void GenerateLake(float rowSpacing, float colSpacing, float scaleNum);
			lakePro* AddLakeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);
			lakePro* AddGroundToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);


			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	gooseMesh	= nullptr;
			OGLMesh*	keeperMesh	= nullptr;
			OGLMesh*	appleMesh	= nullptr;
			OGLMesh*	charA		= nullptr;
			OGLMesh*	charB		= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}


			int* mapSeed;
			int mapLevel;


			NavigationGrid* grid;

			void GoosePathfinding();
			void GooseSkills(float dt);

			float jumpPass;
			bool jumpCD;

			float accalPass;
			bool accalCD;

			void EnemyPathfinding();
			static Vector3 PathWayfinding(Vector3 startPos, Vector3 endPos, Vector4 colour, NavigationGrid* grid);

			void UpdataEnemy();
			void genStateMachine();

			Vector3 stayPos;
			static void ememyTrack(void* data);
			static void ememyBack(void* data);
			static void ememyStay(void* data);

			void InformationPrint();

			void GameStateCheck(float dt);
			float totaltime;
			int nowPoint;
			string print1;
			string print2;
			string print3;

			void UiFunction();
			void lockCamForUI();
			bool UIState;
			void addUICube();
			bool selectUICube();
			void begingame(UICubePro* usingFunction);
			vector<UICubePro*> UIFunction;
			UICubePro* AddUICubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass);

			vector <PositionConstraint* > constraintList;
			void UpdataConstraint(float dt);

			void pushDownState();
			PushdownMachine* UIMachine;
			PushdownState* openEnter;
			PushdownState* joinEnter;
			PushdownState* QuitGame;
			PushdownState* GameBegin;
			PushState OpenNetGame;
			PushState JoinNetGame;
			PushState Quitgame;
			PushState Begingame;
			bool nowUIstate;

			NetWookPro* newplayer;
			void UpdatePackageS();
			void UpdatePackageC();
			void RegisterNetServer();
			void RegisterNetClient();
			GameServer* server;
			GameClient* client;
			void PrintNetInfor();
			void AddOtherPlayers();
			goosePro* otherGoose;
			Vector3 otherGoosePos;

			GamePacket* playerInformation;
			GamePacket* timeInformation;
			GamePacket* positionInformation;
			
		};
	}
}

