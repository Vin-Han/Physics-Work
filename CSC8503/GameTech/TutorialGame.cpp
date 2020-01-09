#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/PositionConstraint.h"
#include <iostream>
#include <fstream>
#include "../../Common/Assets.h"
#include <algorithm>
#include<iostream>
#include"../../Common/Maths.h"

using namespace std;
using namespace NCL;
using namespace CSC8503;

#pragma region Basic Function

TutorialGame::TutorialGame() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	forceMagnitude = 10.0f;
	useGravity = false;
	GamingMode = false;

	this->mapSeed = NULL;
	this->mapLevel = NULL;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

TutorialGame::TutorialGame(int* mapSeed, int mapLevel) {
	this->mapSeed = mapSeed;
	this->mapLevel = mapLevel;
	WriteMapToFile();
	grid = new NavigationGrid("MapFile.txt");
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	forceMagnitude = 10.0f;

	jumpPass = 0.0f;
	jumpCD = true;

	accalPass = 0.0f;
	accalCD = true;
	nowUIstate = 0;

	UIState = 1;
	quitGame = 0;

	useGravity = false;
	GamingMode = false;
	cameraMode = false;
	stayPos = Vector3(0, 0, 0);
	originalColor = Vector4(0, 0, 0, 1);

	print1 = "enjoy game !" ;
	print2 = "relex ! have fun!";
	print3 = "select your mode!";

	otherGoose = nullptr;

	Debug::SetRenderer(renderer);

	InitialiseAssets();

}
/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh", &cubeMesh);
	loadFunc("sphere.msh", &sphereMesh);
	loadFunc("goose.msh", &gooseMesh);
	loadFunc("CharacterA.msh", &keeperMesh);
	loadFunc("CharacterM.msh", &charA);
	loadFunc("CharacterF.msh", &charB);
	loadFunc("Apple.msh", &appleMesh);

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete gooseMesh;
	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	appleArray.clear();
	enemyArray.clear();
	enemyStateMachine.clear();
	lakeArray.clear();
	groundArray.clear();
	UIFunction.clear();
	NetworkBase::Destroy();
}

#pragma endregion

#pragma region KeyWords & Function

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		appleArray.clear();
		enemyArray.clear();
		enemyStateMachine.clear();
		lakeArray.clear();
		groundArray.clear();
		UIFunction.clear();
		constraintList.clear();
		selectionObject = nullptr;
		physics->UseGravity(0);
		UIState = 1;
		GamingMode = 0;
		print1 = "enjoy game !";
		print2 = "relex ! have fun!";
		print3 = "select your mode!";
		NetworkBase::Destroy();
		InitWorld(); //We can reset the simulation at any time with F1

	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}
/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		GamingMode = !GamingMode;

		if (GamingMode) {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
		else {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
	}
	if (GamingMode) {
		renderer->DrawString("Q : change vision mode!", Vector2(10, 0));
		if (selectionObject){selectionObject->GetRenderObject()->SetColour(originalColor);}
		selectionObject = gooseOBJ;
		originalColor = selectionObject->GetRenderObject()->GetColour();
		selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

		}
	else {
		renderer->DrawString("Q : change vision mode!", Vector2(10, 0));
		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(originalColor);
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
			RayCollision closestCollision;

			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				originalColor = selectionObject->GetRenderObject()->GetColour();
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		renderer->DrawString("L : lock camera !", Vector2(10, 15));
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	return false;
}
/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force :" + std::to_string(forceMagnitude),
		Vector2(10, 30)); // Draw debug text at 10 ,20
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;// we haven ’t selected anything !
	}
	// Push the selected object !
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(
			*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(
					ray.GetDirection() * forceMagnitude,
					closestCollision.collidedAt);
			}
		}
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		selectionObject->GetPhysicsObject()->AddForce(-rightAxis*30);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		selectionObject->GetPhysicsObject()->AddForce(rightAxis*30);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis*30);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis*30);
	}
} 

void TutorialGame::LockedCameraMovement() {
	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetWorldPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.5f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
} 

void TutorialGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	if (GamingMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 30, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -30, 0));
		}
		Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
		tempAng.Normalise();
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			if (selectionObject->speedState == 1){
				selectionObject->GetPhysicsObject()->AddForce(tempAng * 10);
			}
			else {
				selectionObject->GetPhysicsObject()->AddForce(tempAng * 50);
			}

		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			if (selectionObject->speedState == 1) {
				selectionObject->GetPhysicsObject()->AddForce(-tempAng * 10);
			}
			else {
				selectionObject->GetPhysicsObject()->AddForce(-tempAng * 50);
			}
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 10, 0));
		}
	}
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	float scaleNum = 4.0f;
	totaltime = 0;
	stayPos = Vector3(0, 0, 0);
	originalColor = Vector4(0, 0, 0, 1);
	//pushdownmachine
	pushDownState();
	//goose
	for (int hor = 2; hor < mapLevel + 1; hor++) {
		int ver = 2;
		if (mapSeed[ver * (mapLevel + 2) + hor] == 0) {
			gooseOBJ = AddGooseProToWorld(Vector3(ver * 2 * scaleNum , 8 , 2 * hor * scaleNum ), scaleNum/2);
			gooseOBJ->originalPos = gooseOBJ->GetTransform().GetWorldPosition();
			break;
		}
	}
	//target
	for (int hor = mapLevel-1; hor > 2; hor--) {
		int ver = mapLevel-1;
		if (mapSeed[ver * (mapLevel + 2) + hor] == 0) {
			Vector3 position = Vector3(ver * 2 * scaleNum, 0, 2 * hor * scaleNum);
			//Vector3 position = Vector3(2 * hor * scaleNum, 15, (mapLevel - 1) * 2 * scaleNum);
			target = AddTargetCubeToWorld(position, Vector3(scaleNum, scaleNum, scaleNum), 1.0f);
			target->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
			target->originalPos = target->GetTransform().GetWorldPosition();
			target->OBJtype = 2;
			stayPos = position;
			break;
		}
	} 
	//map
	InitDIYWorld(2.0f, 2.0f, Vector3(1, 1, 1), mapSeed, mapLevel, scaleNum);
	//lake
	GenerateLake(2.0f, 2.0f, scaleNum);
	//floor
	AddFloorToWorld(Vector3(0, -3, 0));
	//StateMechine
	genStateMachine();
	//global
	addUICube();
	Window::GetWindow()->ShowOSPointer(true);
	Window::GetWindow()->LockMouseToWindow(false);
}

void TutorialGame::InitDIYWorld(float rowSpacing, float colSpacing, const Vector3& cubeDims, int* mapSeed, int mapLevel, float scaleNum) {
	rowSpacing *= scaleNum;
	colSpacing *= scaleNum;
	//range
	for (int tempSet = 0; tempSet < mapLevel + 2; )
	{
		for (int ver = 1; ver < mapLevel + 1; ver++) {
			Vector3 position = Vector3(ver * colSpacing, 0.0f, tempSet * colSpacing);
			GameObject* temp = AddCubeToWorld(position, Vector3(cubeDims.x * scaleNum, cubeDims.y * scaleNum, cubeDims.z * scaleNum), 1.0f);
			temp->originalPos = temp->GetTransform().GetWorldPosition();
		}
		for (int hor = 0; hor < mapLevel + 2; hor++) {
			Vector3 position = Vector3(tempSet * colSpacing, 0.0f, hor * rowSpacing);
			GameObject* temp = AddCubeToWorld(position, Vector3(cubeDims.x * scaleNum, cubeDims.y * scaleNum, cubeDims.z * scaleNum), 1.0f);
			temp->originalPos = temp->GetTransform().GetWorldPosition();
		}
		tempSet += (mapLevel + 1);
	}
	int tempNum = 0;
	//map
	for (int ver = 1; ver < mapLevel + 1; ++ver) {
		for (int hor = 1; hor < mapLevel + 1; ++hor) {
			if (mapSeed[ver * (mapLevel + 2) + hor] == 2
				) {
				Vector3 position = Vector3(ver * colSpacing, 0.0f, hor * rowSpacing);
				GameObject* temp = AddCubeToWorld(position, Vector3(cubeDims.x * scaleNum*0.6, cubeDims.y * scaleNum * 0.6, cubeDims.z * scaleNum * 0.6), 1.0f);
				temp->originalPos = temp->GetTransform().GetWorldPosition();
			}
			else if (mapSeed[ver * (mapLevel + 2) + hor] == 1
				) {
				if (tempNum == 6) {
					Vector3 position = Vector3(ver * colSpacing, 4.0f, hor * rowSpacing);
					enemyAI* temp = AddEnemyToWorld(position, scaleNum * 1.5);
					temp->goosePos = gooseOBJ->GetTransform().GetWorldPosition();
					temp->grid = grid;
					enemyArray.emplace_back(temp);
					temp->originalPos = temp->GetTransform().GetWorldPosition();
					std::vector<applePro*>::const_iterator lastapple = appleArray.end();
					temp->nextPos = (*(lastapple - 1))->GetTransform().GetWorldPosition();
					stayPos = position;
					tempNum = 0;
				}
				else if (tempNum == 4 ) {
					Vector3 position = Vector3(ver * colSpacing, 4.0f, hor * rowSpacing);
					applePro* temp = AddAppleProToWorld(position, scaleNum / 3);
					temp->GetRenderObject()->SetColour(Vector4(0.5,0.1, 0.1, 0));
					temp->applePoint = 2;
					appleArray.emplace_back(temp);
					temp->originalPos = temp->GetTransform().GetWorldPosition();
					tempNum += 1;
				}
				else  {
					Vector3 position = Vector3(ver * colSpacing, 4.0f, hor * rowSpacing);
					applePro* temp = AddAppleProToWorld(position, scaleNum / 4);
					temp->GetRenderObject()->SetColour(Vector4(0.3, 0.1,0.1, 0));
					appleArray.emplace_back(temp);
					temp->applePoint = 1;
					temp->originalPos = temp->GetTransform().GetWorldPosition();
					tempNum += 1;
				}
			}
		}
	}
}

void TutorialGame::GenerateLake(float rowSpacing, float colSpacing, float scaleNum) {
	rowSpacing *= scaleNum;
	colSpacing *= scaleNum;
	float hight = -0.1f;
	for (int i = 0; i < 4; i++) {
		int lakever = rand() % (mapLevel);
		int lakehor = rand() % (mapLevel);
		Vector3 lakePos = Vector3(lakever * colSpacing, hight + 0.01 * i, lakehor * rowSpacing);

		lakePro* tempLake = AddLakeToWorld(lakePos, Vector3(scaleNum * 4, 0.5, scaleNum * 4), 1.0f);
		tempLake->originalPos = tempLake->GetTransform().GetWorldPosition();
		tempLake->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));

		Vector3 GroundPos = Vector3(lakever * colSpacing, hight + 0.02 * i + 0.1, lakehor * rowSpacing);
		lakePro* tempGround = AddGroundToWorld(GroundPos, Vector3(scaleNum * 1.5, 1.0, scaleNum * 1.5), 1.0f);
		tempGround->originalPos = tempGround->GetTransform().GetWorldPosition();
		tempGround->GetRenderObject()->SetColour(Vector4(1.0f, 1.0f, 0.0f, 1));
	}
	Vector3 lakePos = gooseOBJ->GetTransform().GetWorldPosition();
	lakePos.y = hight + 0.05;
	lakePro* tempLake = AddLakeToWorld(lakePos, Vector3(scaleNum * 4, 0.5, scaleNum * 4), 1.0f);
	tempLake->originalPos = tempLake->GetTransform().GetWorldPosition();
	tempLake->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));

	Vector3 GroundPos = gooseOBJ->GetTransform().GetWorldPosition();
	GroundPos.y = hight + 0.10 + 0.1;
	lakePro* tempGround = AddGroundToWorld(GroundPos, Vector3(scaleNum * 1.5, 1.0, scaleNum * 1.5), 1.0f);
	tempGround->originalPos = tempGround->GetTransform().GetWorldPosition();
	tempGround->GetRenderObject()->SetColour(Vector4(1.0f, 1.0f, 0.0f, 1));

}

void TutorialGame::UpdateGame(float dt) {
	//gooseOBJ->speedState = 0;
#pragma region CameraSetting
	if (!GamingMode) { world->GetMainCamera()->UpdateCamera(dt); }
	if (lockedObject != nullptr) { LockedCameraMovement(); }
#pragma endregion
#pragma region GravitySetting
	if (useGravity) { Debug::Print("(G)ravity on", Vector2(10, 40)); }
	else { Debug::Print("(G)ravity off", Vector2(10, 40)); }
#pragma endregion
#pragma region CoordinateLine
	//for (int ver = -100; ver < 100; ver += 4) {
	//	Debug::DrawLine(Vector3(ver, 1, -100), Vector3(ver, 1, 100), Vector4(0, 1, 0, 1));
	//}
	//for (int ver = -100; ver < 100; ver += 4) {
	//	Debug::DrawLine(Vector3(-100, 1, ver), Vector3(100, 1, ver), Vector4(1, 0, 0, 1));
	//}
	//Debug::DrawLine(Vector3(0, 0, 0), Vector3(0, 100, 0), Vector4(0, 0, 1, 1));
#pragma endregion
	UpdateKeys();
	UiFunction();


	SelectObject();
	DrawBaseLine();
	MoveSelectedObject();

	gooseOBJ->speedState = 0;

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	if (selectionObject == gooseOBJ) {
		GoosePathfinding();
		//EnemyPathfinding();
		if (GamingMode){
			BindCameraToGoose();
		}
		GooseSkills(dt);
		AppleDirCheck();
		UpdataEnemy();
		GameStateCheck(dt);
		UpdataConstraint(dt);
	}
	InformationPrint();
#pragma region TempSetGoose
	Quaternion tempQua = gooseOBJ->GetTransform().GetLocalOrientation();
	tempQua.z = 0;
	tempQua.x = 0;
	gooseOBJ->GetTransform().SetLocalOrientation(tempQua);
#pragma endregion

	Debug::FlushRenderables();
	renderer->Render();

	if (server)
	{
		UpdatePackageS();
		PrintNetInfor();
	}
	else if (client)
	{
		UpdatePackageC();
		PrintNetInfor();
	}
	if (otherGoose){
		otherGoose->GetTransform().SetWorldPosition(otherGoosePos);
	}
}

#pragma endregion

#pragma region Add Component to World

/*
A single function to add a large immoveable cube to the bottom of our world
*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject("floor");
	Vector3 floorSize = Vector3(100, 2, 100);
	floor->OBJtype = 5;
	Vector3 tempPos = Vector3(position.x + floorSize.x, position.y, position.z + floorSize.z);

	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(tempPos);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("cube");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	//cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

/*
Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.
*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject("sphere");

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();
	sphere->OBJtype = 100;

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddAppleToWorld(const Vector3& position ,float appleScale) {
	GameObject* apple = new GameObject("apple");
	Vector3 temp = Vector3(4* appleScale, 4 * appleScale, 4 * appleScale);
	SphereVolume* volume = new SphereVolume(0.7f * appleScale);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(temp);
	apple->GetTransform().SetWorldPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->OBJtype = 3;
	world->AddGameObject(apple);

	return apple;
}

GameObject* TutorialGame::AddGooseToWorld(const Vector3& position, float gooseScale)
{
	float inverseMass	= 1.0f;
	Vector3 cbsize = Vector3(0.8, 0.8, 0.8);
	float size = 0.8;
	GameObject* goose = new GameObject("goose");

	SphereVolume* volume = new SphereVolume(size* gooseScale);
	//SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(gooseScale, gooseScale, gooseScale) );
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();
	goose->OBJtype = 1;

	world->AddGameObject(goose);

	return goose;
}

GameObject* TutorialGame::AddParkKeeperToWorld(const Vector3& position)
{
	float meshSize = 4.0f;
	float inverseMass = 0.5f;

	GameObject* keeper = new GameObject("enemy");

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * meshSize);
	keeper->SetBoundingVolume((CollisionVolume*)volume);

	keeper->GetTransform().SetWorldScale(Vector3(meshSize, meshSize, meshSize));
	keeper->GetTransform().SetWorldPosition(position);

	keeper->SetRenderObject(new RenderObject(&keeper->GetTransform(), keeperMesh, nullptr, basicShader));
	keeper->SetPhysicsObject(new PhysicsObject(&keeper->GetTransform(), keeper->GetBoundingVolume()));

	keeper->GetPhysicsObject()->SetInverseMass(inverseMass);
	keeper->GetPhysicsObject()->InitCubeInertia();
	keeper->OBJtype = 4;

	world->AddGameObject(keeper);

	return keeper;
}

GameObject* TutorialGame::AddCharacterToWorld(const Vector3& position, float characterScale) {
	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	GameObject* character = new GameObject("enemy");

	float r = rand() / (float)RAND_MAX;


	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * characterScale);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(characterScale, characterScale, characterScale));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();
	character->OBJtype = 4;

	world->AddGameObject(character);

	return character;
}

enemyAI* TutorialGame::AddEnemyToWorld(const Vector3& position, float characterScale) {

	float inverseMass = 0.5f;

	auto pos = keeperMesh->GetPositionData();

	Vector3 minVal = pos[0];
	Vector3 maxVal = pos[0];

	for (auto& i : pos) {
		maxVal.y = max(maxVal.y, i.y);
		minVal.y = min(minVal.y, i.y);
	}

	enemyAI* character = new enemyAI("enemy");
	character->SetName("enemy");

	float r = rand() / (float)RAND_MAX;

	AABBVolume* volume = new AABBVolume(Vector3(0.3, 0.9f, 0.3) * characterScale);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform().SetWorldScale(Vector3(characterScale, characterScale, characterScale));
	character->GetTransform().SetWorldPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), r > 0.5f ? charA : charB, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitCubeInertia();
	character->OBJtype = 4;

	world->AddGameObject(character);

	return character;
}

applePro* TutorialGame::AddAppleProToWorld(const Vector3& position, float appleScale) {
	applePro* apple = new applePro("apple");
	apple->SetName("apple");

	Vector3 temp = Vector3(4 * appleScale, 4 * appleScale, 4 * appleScale);
	SphereVolume* volume = new SphereVolume(0.7f * appleScale);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform().SetWorldScale(temp);
	apple->GetTransform().SetWorldPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), appleMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();
	apple->OBJtype = 3;
	apple->proState = &(apple->appleState);

	world->AddGameObject(apple);

	return apple;
}

goosePro* TutorialGame::AddGooseProToWorld(const Vector3& position, float gooseScale){
	float inverseMass = 1.0f;
	Vector3 cbsize = Vector3(0.8, 0.8, 0.8);
	float size = 0.8;
	goosePro* goose = new goosePro("goose");
	goose->SetName("goose");

	SphereVolume* volume = new SphereVolume(size * gooseScale);
	//SphereVolume* volume = new SphereVolume(size);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform().SetWorldScale(Vector3(gooseScale, gooseScale, gooseScale));
	goose->GetTransform().SetWorldPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();
	goose->OBJtype = 1;

	world->AddGameObject(goose);

	return goose;
}

targetCube* TutorialGame::AddTargetCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	targetCube* cube = new targetCube("targetCube");
	cube->SetName("targetCube");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	//cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->OBJtype = 2;

	world->AddGameObject(cube);

	return cube;
}

lakePro* TutorialGame::AddLakeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	lakePro* cube = new lakePro("lake");
	cube->SetName("lake");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	//cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->OBJtype = 6;

	lakeArray.emplace_back(cube);
	world->AddGameObject(cube);

	return cube;
}

lakePro* TutorialGame::AddGroundToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	lakePro* cube = new lakePro("ground");
	cube->SetName("ground");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	//cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->OBJtype = 7;

	groundArray.emplace_back(cube);
	world->AddGameObject(cube);

	return cube;
}

UICubePro* TutorialGame::AddUICubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	UICubePro* cube = new UICubePro("UICube");
	cube->SetName("UICube");

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	//cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->OBJtype = 8;
	cube->proState = &(cube->uifunc);

	world->AddGameObject(cube);

	return cube;
}

#pragma endregion

#pragma region Setposition of Components

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

#pragma endregion
#pragma region Others

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(8, 8, 8);

	float	invCubeMass = 5;
	int		numLinks	= 25;
	float	maxDistance	= 30;
	float	cubeDistance = 20;

	Vector3 startPos = Vector3(500, 1000, 500);

	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);

	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);

	GameObject* previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}

	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}

void TutorialGame::DrawBaseLine() {
	if (selectionObject) {
		Vector3 tempVec = selectionObject->GetTransform().GetWorldPosition();
		Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
		tempAng.Normalise();
		Debug::DrawLine(Vector3(0, 0, 0), tempAng * 30, Vector4(0, 0, 1, 1));
		Debug::DrawLine(tempVec, tempVec + tempAng * 30, Vector4(0, 0, 1, 1));
	}
}

//two version of writingFile
#if 0
void TutorialGame::WriteMapToFile() {
	char data[100];
	ofstream outfile;
	outfile.open(Assets::DATADIR + "MapFile.txt");
	outfile << 8 << endl;
	outfile << (mapLevel + 2) << endl;
	outfile << (mapLevel + 2) << endl;
	std::string temp;
	for (int i = 0; i < (mapLevel + 2) * (mapLevel + 2); i++) {
		if (i < mapLevel + 1)
		{
			outfile << 'x';
		}
		else if ((i + 1) % (mapLevel + 2) == 0) {
			outfile << 'x';
			outfile << endl;
		}
		else if ((i + 1) / (mapLevel + 2) == (mapLevel + 1)) {
			outfile << 'x';
		}
		else if (i % (mapLevel + 2) == 0) {
			outfile << 'x';
		}
		else {
			if (mapSeed[i] != 2) {
				outfile << '.';
			}
			else {
				outfile << 'x';
			}
		}
	}
	outfile.close();
}

#else if 
void TutorialGame::WriteMapToFile() {
	int sizeNum = 2;
	char data[100];
	ofstream outfile;
	outfile.open(Assets::DATADIR + "MapFile.txt");
	outfile << 8 / sizeNum << endl;
	outfile << (mapLevel + 2) * sizeNum << endl;
	outfile << (mapLevel + 2) * sizeNum << endl;

	string fileLine = "";

	for (int ver = 0; ver < (mapLevel + 2); ver++) {
		for (int hor = 0; hor < (mapLevel + 2); hor++) {

			if (ver == 0 || hor == 0 || ver == (mapLevel + 1)) {
				fileLine += "xx";
				if (hor == (mapLevel + 1)) {
					for (int degrees = 0; degrees < sizeNum; degrees++) {
						outfile << fileLine << endl;
					}
					fileLine = "";
				}
			}

			else if (hor == (mapLevel + 1)) {
				fileLine += "xx";
				for (int degrees = 0; degrees < sizeNum; degrees++) {
					outfile << fileLine << endl;
				}
				fileLine = "";
			}
			else {
				if (mapSeed[ver * (mapLevel + 2) + hor] != 2) {
					fileLine += "..";
				}
				else {
					fileLine += "xx";
				}
			}
		}
	}
	outfile.close();
}
#endif

Vector3 TutorialGame::PathWayfinding(Vector3 startPos, Vector3 endPos, Vector4 colour, NavigationGrid* grid) {
	vector<Vector3> mapNodes;
	NavigationPath outPath;
	bool found = grid->FindPath(Vector3(startPos.z, startPos.y, startPos.x), Vector3(endPos.z, endPos.y, endPos.x), outPath);
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		mapNodes.push_back(pos);
	}
	Vector3 tempTarget = Vector3(0,0,0);

	if (mapNodes.size() > 1){
		//mapNodes[0] = startPos ;
		tempTarget = (mapNodes[1]- mapNodes[0]);
	}

	for (int i = 1; i < mapNodes.size(); ++i) {
		Vector3 a = mapNodes[i - 1];
		Vector3 b = mapNodes[i];
		Debug::DrawLine(a, b, colour);
	}
	return tempTarget;
}

void TutorialGame::BindCameraToGoose() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::CONTROL)) {
		cameraMode = !cameraMode;
	}
	if (!cameraMode)
	{
		Vector3 tempVec = selectionObject->GetTransform().GetWorldPosition();
		tempVec.y += 100;
		world->GetMainCamera()->SetPosition(tempVec);
		Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
		tempAng.Normalise();
		world->GetMainCamera()->SetPitch(-80);
		world->GetMainCamera()->SetYaw(selectionObject->GetTransform().GetWorldOrientation().ToEuler().y + 180);
	}
	else if (cameraMode)
	{
		world->GetMainCamera()->SetPitch(-30);
		world->GetMainCamera()->SetYaw(selectionObject->GetTransform().GetWorldOrientation().ToEuler().y + 180);
		Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
		tempAng.Normalise();
		world->GetMainCamera()->SetPosition(selectionObject->GetTransform().GetWorldPosition() - tempAng * 20 + Vector3(0, 20, 0));
	}

}

void TutorialGame::GoosePathfinding() {
	Vector3 startPos = selectionObject->GetTransform().GetWorldPosition();
	Vector3 endPos = target->GetTransform().GetWorldPosition();
	PathWayfinding(startPos, endPos, Vector4(0, 1, 0, 1),grid);
}

void TutorialGame::EnemyPathfinding() {
	std::vector<enemyAI*>::const_iterator first = enemyArray.begin();
	Vector3 goosePos = selectionObject->GetTransform().GetWorldPosition();
	for (; first < enemyArray.end(); first++) {
		Vector3 enemyPos = (*first)->GetTransform().GetWorldPosition();
		Vector3(goosePos - enemyPos).Length();

		if (Vector3(goosePos - enemyPos).Length() <= 30) {
			PathWayfinding(enemyPos, goosePos, Vector4(0, 0, 1, 1), grid);
		}
	}
}

//three version of check applecollision
/*
void TutorialGame::AppleDirCheck() {
	std::vector<applePro*>::const_iterator first = appleArray.begin();
	Vector3 goosePos = selectionObject->GetTransform().GetWorldPosition();
	Vector3 targetPos = target->GetTransform().GetWorldPosition();
	for (; first < appleArray.end(); first++) {
		if ((*first)->appleState == 0) {
			Vector3 applePos = (*first)->GetTransform().GetWorldPosition();
			applePos -= goosePos;
			if (applePos.Length() < 4) {
				(*first)->GetTransform().SetWorldPosition(Vector3(goosePos.x, goosePos.y + 10, goosePos.z));
				(*first)->appleState = 1;
				(*first)->OBJtype = 3;
				gooseOBJ->appleNum += 1;
			}
		}
		if ((*first)->appleState == 1) {
			(*first)->GetTransform().SetWorldPosition(Vector3(goosePos.x, goosePos.y + 20, goosePos.z));
			Vector3 applePos = (*first)->GetTransform().GetWorldPosition();
			applePos -= targetPos;
			if (applePos.Length() < 10) {
				(*first)->GetTransform().SetWorldPosition(Vector3(targetPos.x - 2 + (rand() / 50) / 100, targetPos.y + 10 + (rand() / 50) / 100, targetPos.z - 2 + (rand() / 50) / 100));
				(*first)->appleState = 2;
				(*first)->OBJtype = 0;
				gooseOBJ->appleNum -= 1;
				gooseOBJ->pointNum += 1;
			}
		}
	}
}
*/
/*
void TutorialGame::AppleDirCheck() {
	std::vector<GameObject*>::const_iterator first = gooseOBJ->appleList.begin();
	Vector3 goosePos = gooseOBJ->GetTransform().GetWorldPosition();
	for (; first < gooseOBJ->appleList.end(); first++) {
		if (*((*first)->proState) == 0){
			*((*first)->proState) = 1;
			(*first)->OBJtype = 0;
			gooseOBJ->appleNum += 1;
		}
	}
	gooseOBJ->appleList.clear();

	Vector3 targetPos = target->GetTransform().GetWorldPosition();
	std::vector<applePro*>::const_iterator firstApple = appleArray.begin();
	for (int i = 0; firstApple < appleArray.end(); firstApple++) {
		if (Vector3(goosePos - targetPos).Length() <= 8.0f) {
			if ((*firstApple)->appleState == 1) {
				(*firstApple)->GetTransform().SetWorldPosition(Vector3(targetPos.x - 2 + (rand() / 50) / 100, targetPos.y + 10 + (rand() / 50) / 100, targetPos.z - 2 + (rand() / 50) / 100));
				(*firstApple)->appleState = 2;
				(*firstApple)->OBJtype = 3;
				gooseOBJ->appleNum -= 1;
				gooseOBJ->pointNum += (*firstApple)->applePoint;
				(*firstApple)->GetRenderObject()->SetColour( Vector4(1,0,0,1) );
			}
		}
		else {
			if ((*firstApple)->appleState == 1) {
				(*firstApple)->GetTransform().SetWorldPosition(Vector3(goosePos.x, goosePos.y + 2*i + 5, goosePos.z));
				i++;
			}
		}
	}
}
*/
void TutorialGame::AppleDirCheck() {
	std::vector<GameObject*>::const_iterator first = gooseOBJ->appleList.begin();
	Vector3 goosePos = gooseOBJ->GetTransform().GetWorldPosition();
	for (; first < gooseOBJ->appleList.end(); first++) {
		if (*((*first)->proState) == 0) {
			*((*first)->proState) = 1;
			gooseOBJ->appleNum += 1;
			PositionConstraint* tempCon = new PositionConstraint(gooseOBJ, (*first), 3.0f);
			constraintList.emplace_back(tempCon);

		}
	}
	gooseOBJ->appleList.clear();

	Vector3 targetPos = target->GetTransform().GetWorldPosition();
	std::vector<applePro*>::const_iterator firstApple = appleArray.begin();
	for (int i = 0; firstApple < appleArray.end(); firstApple++) {
		if (Vector3(goosePos - targetPos).Length() <= 8.0f) {
			if ((*firstApple)->appleState == 1) {
				(*firstApple)->GetTransform().SetWorldPosition(Vector3(targetPos.x - 2 + (rand() / 50) / 100, targetPos.y + 10 + (rand() / 50) / 100, targetPos.z - 2 + (rand() / 50) / 100));
				(*firstApple)->appleState = 2;
				(*firstApple)->OBJtype = 3;
				gooseOBJ->appleNum -= 1;
				gooseOBJ->pointNum += (*firstApple)->applePoint;
				(*firstApple)->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
				constraintList.clear();
			}
		}
		else {
			if ((*firstApple)->appleState == 1) {
				i++;
			}
		}
	}
}

void TutorialGame::UpdataConstraint(float dt) {
	if (constraintList.size()) {
		std::vector<PositionConstraint*>::const_iterator first = constraintList.begin();
		for (; first < constraintList.end(); first++) {
			(*first)->UpdateConstraint(dt);
		}
	}
}

void TutorialGame::EnemyAttackCheck() {
	if (gooseOBJ->appleNum >= 0){
		std::vector<applePro*>::const_iterator first = appleArray.begin();
		for (; first < appleArray.end(); first++) {
			if ((*first)->appleState == 1){
				Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
				tempAng.Normalise();
				(*first)->GetPhysicsObject()->AddForce(-tempAng*1000);
				(*first)->appleState = 0;
				(*first)->OBJtype = 3;
				gooseOBJ->appleNum -= 1;
				constraintList.clear();
			}
		}
	}

}

void TutorialGame::GooseSkills(float dt) {
	if (jumpCD) {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * 1000);
			jumpPass = 0.0f;
			jumpCD = 0;
		}
	}
	if (!jumpCD) {
		if (jumpPass <= 3.0f) { jumpPass += dt; }
		if (jumpPass >= 3.0f) { jumpCD = 1; }


	}

	if (accalCD) {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Z)) {
			Vector3 tempAng = selectionObject->GetTransform().GetLocalOrientation() * Vector3(0, 0, 1);
			tempAng.Normalise();
			selectionObject->GetPhysicsObject()->AddForce(tempAng * 1000);
			accalPass = 0.0f;
			accalCD = 0;
		}
	}
	if (!accalCD) {
		if (accalPass <= 5.0f) { accalPass += dt; }
		if (accalPass >= 5.0f) { accalCD = 1; }
	}

}

void TutorialGame::InformationPrint(){
	if (GamingMode){
		renderer->DrawString("points :" + std::to_string(gooseOBJ->pointNum) + "/" + std::to_string(appleArray.size()), Vector2(10, 650));
		int temptime = (int)totaltime;
		renderer->DrawString("time cost :" + std::to_string(temptime), Vector2(10, 635));
		
		if (!jumpCD) { renderer->DrawString("JUMP CD : " + std::to_string((3.0 - jumpPass)), Vector2(500, 650)); }
		else { renderer->DrawString("JUMP CD : 0", Vector2(500, 650)); }

		if (!accalCD) { renderer->DrawString("ACCEL CD : " + std::to_string((5.0 - accalPass)), Vector2(500, 630)); }
		else { renderer->DrawString("ACCEL CD : 0", Vector2(500, 630)); }
	}
	else{
		if (selectionObject)
		{
			renderer->DrawString("name    :" + selectionObject->GetName() , Vector2(10, 630));
			renderer->DrawString("posiiton:" + std::to_string((selectionObject->GetTransform().GetWorldPosition().x)) + "\t" +
											   std::to_string((selectionObject->GetTransform().GetWorldPosition().y)) + "\t" +
											   std::to_string((selectionObject->GetTransform().GetWorldPosition().x))
										 , Vector2(10, 615));													   
		}
	}

}

void  TutorialGame::GameStateCheck(float dt) {
	totaltime += dt;
	if (gooseOBJ->pointNum >= appleArray.size() || totaltime >= 180.0f) {

		int temp = (int)totaltime;
		nowPoint = gooseOBJ->pointNum;

		appleArray.clear();
		enemyArray.clear();
		enemyStateMachine.clear();
		lakeArray.clear();
		groundArray.clear();
		UIFunction.clear();
		constraintList.clear();
		selectionObject = nullptr;
		physics->UseGravity(0);
		UIState = 1;
		GamingMode = 0;
		InitWorld();

		if (nowPoint >= appleArray.size()) {
			print1 = "Congratulation £¡";
			print2 = "You win the game";
			print3 = "Time cost :" + std::to_string(temp) + " s";
		}
		if (temp >= 180) {
			print1 = "Sorry this time £¡";
			print2 = "You lost game !";
			print3 = "Time is over!";
		}
	}
}

#pragma endregion

#pragma region StateMechine

void TutorialGame::ememyTrack(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();

	Vector3 forceDir = PathWayfinding(temp->GetTransform().GetWorldPosition(), (temp->goosePos), Vector4(1, 0, 0, 0), temp->grid);
	forceDir.Normalise();
	float len = forceDir.Length();
	float angle = acos(forceDir.z / len);
	if (forceDir.x < 0) angle = -angle;
	Quaternion currentDir = temp->GetTransform().GetLocalOrientation();
	Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
	Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
	if (forceDir != Vector3(0, 0, 0)) {
		temp->GetTransform().SetLocalOrientation(finalDir);
		temp->GetPhysicsObject()->AddForce(forceDir * 25);
	}
}
/*
void TutorialGame::ememyTrack(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();
	Vector3 tempTarget = PathWayfinding(temp->GetTransform().GetWorldPosition(), (temp->goosePos), Vector4(1, 0, 0, 0), temp->grid);
	if (tempTarget != Vector3(0, 0, 0)) {
		temp->GetTransform().SetWorldPosition(temp->GetTransform().GetWorldPosition() + tempTarget / 50);
	}
}
*/

void TutorialGame::ememyBack(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();

	Vector3 forceDir = PathWayfinding(temp->GetTransform().GetWorldPosition(), temp->originalPos, Vector4(1, 0, 0, 0), temp->grid);
	forceDir.Normalise();
	float len = forceDir.Length();
	float angle = acos(forceDir.z / len);
	if (forceDir.x < 0) angle = -angle;
	Quaternion currentDir = temp->GetTransform().GetLocalOrientation();
	Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
	Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
	if (forceDir != Vector3(0, 0, 0)) {
		temp->GetTransform().SetLocalOrientation(finalDir);
		temp->GetPhysicsObject()->AddForce(forceDir * 25);
	}
}
/*
void TutorialGame::ememyBack(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();
	Vector3 tempTarget = PathWayfinding(temp->GetTransform().GetWorldPosition(), temp->originalPos, Vector4(1, 0, 0, 0), temp->grid);
	if (tempTarget != Vector3(0, 0, 0)) {
		temp->GetTransform().SetWorldPosition(temp->GetTransform().GetWorldPosition() + tempTarget/50);
	}
}
*/

void TutorialGame::ememyStay(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();

	Vector3 forceDir = PathWayfinding(temp->GetTransform().GetWorldPosition(), (temp->nextPos), Vector4(1, 0, 0, 0), temp->grid);
	forceDir.Normalise();
	float len = forceDir.Length();
	float angle = acos(forceDir.z / len);
	if (forceDir.x < 0) angle = -angle;
	Quaternion currentDir = temp->GetTransform().GetLocalOrientation();
	Quaternion dirShouldBe = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), Maths::RadiansToDegrees(angle));
	Quaternion finalDir = Quaternion::Lerp(currentDir, dirShouldBe, 0.20f);
	if (forceDir != Vector3(0, 0, 0)) {
		temp->GetTransform().SetLocalOrientation(finalDir);
		temp->GetPhysicsObject()->AddForce(forceDir * 25);
	}
}
/*
void TutorialGame::ememyStay(void* data) {
	enemyAI* temp = (enemyAI*)data;
	temp->enemyGooLength = ((temp->goosePos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyOriLength = ((temp->originalPos) - temp->GetTransform().GetWorldPosition()).Length();
	temp->enemyNexLength = ((temp->nextPos) - temp->GetTransform().GetWorldPosition()).Length();
	Vector3 tempTarget = PathWayfinding(temp->GetTransform().GetWorldPosition(), (temp->nextPos), Vector4(1, 0, 0, 0), temp->grid);
	if (tempTarget != Vector3(0, 0, 0)) {
		temp->GetTransform().SetWorldPosition(temp->GetTransform().GetWorldPosition() + tempTarget / 50);
	}
}
*/

void TutorialGame::genStateMachine() {
	std::vector<enemyAI*>::const_iterator first = enemyArray.begin();
	for (; first < enemyArray.end(); first++) {
		StateMachine* emenyMachine = new StateMachine();
		//do what
		StateFunc TrackEmemy = &TutorialGame::ememyTrack;
		StateFunc BackEmemy = &TutorialGame::ememyBack;
		StateFunc StayEmemy = &TutorialGame::ememyStay;
		//gen state
		GenericState* Track = new GenericState(TrackEmemy, (void*)(*first));
		GenericState* Back = new GenericState(BackEmemy, (void*)(*first));
		GenericState* Stay = new GenericState(StayEmemy, (void*)(*first));
		//add state
		emenyMachine->AddState(Track);
		emenyMachine->AddState(Back);
		emenyMachine->AddState(Stay);
		//gen transit
		GenericTransition <float&, float >* transitionA =
			new GenericTransition <float&, float >(
				GenericTransition <float&, float >::LessThanTransition,
				(*first)->enemyGooLength, 15.0f, Stay, Track);

		GenericTransition <float&, float >* transitionB =
			new GenericTransition <float&, float >(
				GenericTransition <float&, float >::GreaterThanTransition,
				(*first)->enemyOriLength, 25.0f, Track, Back);

		//GenericTransition <float&, float >* transitionC =
		//	new GenericTransition <float&, float >(
		//		GenericTransition <float&, float >::GreaterThanTransition,
		//		(*first)->enemyGooLength, 30.0f, Track, Stay);

		GenericTransition <float&, float >* transitionD =
			new GenericTransition <float&, float >(
				GenericTransition <float&, float >::LessThanTransition,
				(*first)->enemyOriLength, 5.0f, Back, Stay);

		GenericTransition <float&, float >* transitionE =
			new GenericTransition <float&, float >(
				GenericTransition <float&, float >::LessThanTransition,
				(*first)->enemyGooLength, 15.0f, Back, Track);

		GenericTransition <float&, float >* transitionF =
			new GenericTransition <float&, float >(
				GenericTransition <float&, float >::LessThanTransition,
				(*first)->enemyNexLength, 5.0f, Stay, Back);

		//add transit
		emenyMachine->AddTransition(transitionA);
		emenyMachine->AddTransition(transitionB);
		//emenyMachine->AddTransition(transitionC);
		emenyMachine->AddTransition(transitionD);
		emenyMachine->AddTransition(transitionE);
		emenyMachine->AddTransition(transitionF);

		enemyStateMachine.emplace_back(emenyMachine);
	}
}

void TutorialGame::UpdataEnemy() {
	std::vector<StateMachine*>::const_iterator first = enemyStateMachine.begin();
	Vector3 goosePos = selectionObject->GetTransform().GetWorldPosition();
	for (; first < enemyStateMachine.end(); first++) {
		(*first)->Update();
	}
	std::vector<enemyAI*>::const_iterator firstEnemy = enemyArray.begin();
	for (; firstEnemy != enemyArray.end(); firstEnemy++) {

		(*firstEnemy)->goosePos = goosePos;
		Vector3 tmp = (*firstEnemy)->goosePos;
		if ((*firstEnemy)->enemyGooLength <= 6.0f){
			EnemyAttackCheck();
		}
	}
}

#pragma endregion

#pragma region UIFunction

void TutorialGame::UiFunction() {

	if (UIFunction.size() > 0) {
		std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
		for (; first != UIFunction.end(); first++)
		{
			if ((*first)->GetTransform().GetWorldPosition().y <= -20.0f)
			{
				(*first)->GetPhysicsObject()->SetInverseMass(0);
			}
		}
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::X)) {
		UIState = !UIState;
	}
	if (UIState != 0)
	{
		lockCamForUI();
		selectUICube();
		renderer->DrawString("game start", Vector2(700, 440));
		renderer->DrawString("quit game", Vector2(700, 335));
		renderer->DrawString("open a netgame", Vector2(650, 230));
		renderer->DrawString("join a netgame", Vector2(650, 125));
		renderer->DrawString(print1, Vector2(100, 400));
		renderer->DrawString(print2, Vector2(100, 350));
		renderer->DrawString(print3, Vector2(100, 300));
	}
}

void TutorialGame::lockCamForUI() {
	Vector3 tempVec = Vector3(30, 30, 30);
	world->GetMainCamera()->SetPosition(tempVec);
	world->GetMainCamera()->SetPitch(0);
	world->GetMainCamera()->SetYaw(90);
}

void TutorialGame::addUICube() {
	Vector3 cubePos = Vector3(-10, 35, 20);
	Vector3 cubeScale = Vector3(2, 2, 10);
	UICubePro* cubeTemp1 = AddUICubeToWorld(cubePos, cubeScale, 1.0);
	*(cubeTemp1->proState) = 1;
	UIFunction.emplace_back(cubeTemp1);
	cubeTemp1->pushstate = GameBegin;

	cubePos.y -= 5;
	UICubePro* cubeTemp2 = AddUICubeToWorld(cubePos, cubeScale, 1.0);
	*(cubeTemp2->proState) = 2;
	UIFunction.emplace_back(cubeTemp2);
	cubeTemp2->pushstate = QuitGame;

	cubePos.y -= 5;
	UICubePro* cubeTemp3 = AddUICubeToWorld(cubePos, cubeScale, 1.0);
	*(cubeTemp3->proState) = 3;
	UIFunction.emplace_back(cubeTemp3);
	cubeTemp3->pushstate = openEnter;

	cubePos.y -= 5;
	UICubePro* cubeTemp4 = AddUICubeToWorld(cubePos, cubeScale, 1.0);
	*(cubeTemp4->proState) = 4;
	UIFunction.emplace_back(cubeTemp4);
	cubeTemp4->pushstate = joinEnter;
}

void TutorialGame::pushDownState() {

	OpenNetGame = [&]() {
#pragma region NormalSetting
		UIState = !UIState;
		GamingMode = 1;
		physics->UseGravity(1);
		Window::GetWindow()->ShowOSPointer(false);
		Window::GetWindow()->LockMouseToWindow(true);
		std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
		for (; first != UIFunction.end(); first++)
		{
			(*first)->GetPhysicsObject()->AddForce(Vector3((rand() % 20), (rand() % 20), (rand() % 20) - 20) * (rand() % 50) * 5);

			(*first)->OBJtype = 0;
		}
#pragma endregion
		RegisterNetServer();
		AddOtherPlayers();
	};

	JoinNetGame = [&]() {
#pragma region NormalSetting
		UIState = !UIState;
		GamingMode = 1;
		physics->UseGravity(1);
		Window::GetWindow()->ShowOSPointer(false);
		Window::GetWindow()->LockMouseToWindow(true);
		std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
		for (; first != UIFunction.end(); first++)
		{
			(*first)->GetPhysicsObject()->AddForce(Vector3((rand() % 20), (rand() % 20), (rand() % 20) - 20) * (rand() % 50) * 5);

			(*first)->OBJtype = 0;
		}
#pragma endregion
		RegisterNetClient();
		AddOtherPlayers();
	};

	Quitgame = [&]() {
		quitGame = 1;
	};

	Begingame = [&]() {
		UIState = !UIState;
		GamingMode = 1;
		physics->UseGravity(1);
		Window::GetWindow()->ShowOSPointer(false);
		Window::GetWindow()->LockMouseToWindow(true);
		std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
		for (; first != UIFunction.end(); first++)
		{
			(*first)->GetPhysicsObject()->AddForce(Vector3((rand() % 20), (rand() % 20), -(rand() % 20) - 20) * (rand() % 50) * 5);
			//(*first)->OBJtype = 0;
		}
	};

	GameBegin = new PushdownState(Begingame, 1);
	QuitGame = new PushdownState(Quitgame, 2);
	openEnter = new PushdownState(OpenNetGame, 3);
	joinEnter = new PushdownState(JoinNetGame, 4);
	UIMachine = new PushdownMachine();
}

bool TutorialGame::selectUICube() {
	Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
	RayCollision closestCollision;
	if (!Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
		if (world->Raycast(ray, closestCollision, true)) {
			if (selectionObject != (GameObject*)closestCollision.node) {
				originalColor = ((GameObject*)closestCollision.node)->GetRenderObject()->GetColour();
			}
			selectionObject = (GameObject*)closestCollision.node;

			selectionObject->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
			return true;
		}
		else {
			if (selectionObject)
			{
				selectionObject->GetRenderObject()->SetColour(originalColor);
				selectionObject = nullptr;
			}
			return false;
		}
	}

	if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
		if (world->Raycast(ray, closestCollision, true)) {
			if (selectionObject != (GameObject*)closestCollision.node) {
				originalColor = ((GameObject*)closestCollision.node)->GetRenderObject()->GetColour();
			}
			selectionObject = (GameObject*)closestCollision.node;

			selectionObject->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));

			if (selectionObject->OBJtype == 8)
			{
				std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
				for (; first != UIFunction.end(); first++)
				{
					if ((*first)->uifunc == *(selectionObject->proState))
					{
						UIMachine->Update((*first)->pushstate);
						if ((*first)->pushstate->isActive == 1) {
							((*first)->pushstate->somefunc)();
						}
						break;
						//begingame((*first));
					}
				}
			}
			return true;
		}
		else {
			if (selectionObject)
			{
				selectionObject->GetRenderObject()->SetColour(originalColor);
				selectionObject = nullptr;
			}
			return false;
		}
	}
}

//oldversion
void TutorialGame::begingame(UICubePro* usingFunction) {
	//state 1
	if (usingFunction->uifunc == 1) {
		UIState = !UIState;
		GamingMode = 1;
		physics->UseGravity(1);
		Window::GetWindow()->ShowOSPointer(false);
		Window::GetWindow()->LockMouseToWindow(true);
		std::vector<UICubePro*>::const_iterator first = UIFunction.begin();
		for (; first != UIFunction.end(); first++)
		{
			(*first)->GetPhysicsObject()->AddForce(Vector3((rand() % 20), (rand() % 20), (rand() % 20) - 20) * (rand() % 50) * 5);

			(*first)->OBJtype = 0;
		}
	}
	//state 2
	else if (usingFunction->uifunc == 2)
	{
		quitGame = 1;
	}
	//state 3
	else if (usingFunction->uifunc == 3)
	{
	}
}

void TutorialGame::AddOtherPlayers() {
	otherGoose = AddGooseProToWorld(Vector3(0, 10, 0), 2);
	otherGoose->originalPos = gooseOBJ->GetTransform().GetWorldPosition();
	otherGoose->GetRenderObject()->SetColour(Vector4(1,0,0,1));
	otherGoose->OBJtype = 10;
	otherGoose->GetPhysicsObject()->SetInverseMass(0);
}
#pragma endregion
#pragma region NetWork

void TutorialGame::RegisterNetServer() {
	NetworkBase::Initialise();
	newplayer = new NetWookPro("Server");
	int port = NetworkBase::GetDefaultPort();
	server = new GameServer(port, 1);

	server->RegisterPacketHandler(String_Message, newplayer);
	server->RegisterPacketHandler(Time_Message, newplayer);
	server->RegisterPacketHandler(Position_Message, newplayer);

	client = nullptr;

	playerInformation = new GamePacket(String_Message);
	timeInformation = new GamePacket(Time_Message);
	positionInformation = new GamePacket(Position_Message);
}

void TutorialGame::RegisterNetClient() {
	NetworkBase::Initialise();
	newplayer = new NetWookPro("Client");
	int port = NetworkBase::GetDefaultPort();
	client = new GameClient();

	client->RegisterPacketHandler(String_Message, newplayer);
	client->RegisterPacketHandler(Time_Message, newplayer);
	client->RegisterPacketHandler(Position_Message, newplayer);

	bool canConnect = client->Connect(127, 0, 0, 1, port);
	if (!canConnect) { client = nullptr; }
	server = nullptr;

	playerInformation = new GamePacket(String_Message);
	timeInformation = new GamePacket(Time_Message);
	positionInformation = new GamePacket(Position_Message);
}

void TutorialGame::UpdatePackageS() {

	server->SendGlobalPacket(StringPacket(newplayer->name + " points:" + std::to_string(gooseOBJ->pointNum) + "/" + std::to_string(appleArray.size())));
	
	int temptime = (int)totaltime;
	server->SendGlobalPacket(TimePacket("time cost :" + std::to_string(temptime)));
	
	positionStruct tempPos;
	tempPos.X = (gooseOBJ->GetTransform().GetWorldPosition().x);
	tempPos.Y = (gooseOBJ->GetTransform().GetWorldPosition().z);
	tempPos.Z = (gooseOBJ->GetTransform().GetWorldPosition().y);
	server->SendGlobalPacket(PositionPacket(tempPos));
	
	server->UpdateServer();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void TutorialGame::UpdatePackageC() {
	client->SendPacket(StringPacket(newplayer->name + " points:" + std::to_string(gooseOBJ->pointNum) + "/" + std::to_string(appleArray.size())));
	int temptime = (int)totaltime;

	client->SendPacket(TimePacket("time cost :" + std::to_string(temptime)));
	
	positionStruct tempPos;
	tempPos.X = (gooseOBJ->GetTransform().GetWorldPosition().x);
	tempPos.Y = (gooseOBJ->GetTransform().GetWorldPosition().z);
	tempPos.Z = (gooseOBJ->GetTransform().GetWorldPosition().y);
	client->SendPacket(PositionPacket(tempPos));
	
	client->UpdateClient();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void TutorialGame::PrintNetInfor() {
	newplayer->ReceivePacket(String_Message, playerInformation, -1);
	newplayer->ReceivePacket(Time_Message, timeInformation, -1);
	renderer->DrawString(newplayer->playInformation, Vector2(10, 500));
	renderer->DrawString(newplayer->timeInfotmation, Vector2(50, 480));
	
	otherGoosePos = Vector3((newplayer->playerPosition.X), (newplayer->playerPosition.Y), (newplayer->playerPosition.Z));
	renderer->DrawString(std::to_string((int)otherGoosePos.x) + "/"
		+ std::to_string((int)otherGoosePos.y) + "/"
		+ std::to_string((int)otherGoosePos.z) + "/"
		, Vector2(50, 460));

}

#pragma endregion

