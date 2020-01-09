#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

#include "PhysicsObject.h"
#include "RenderObject.h"
#include "NetworkObject.h"

#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {
		class NetworkObject;


		class GameObject	{
		public:
			GameObject(string name = "");
			~GameObject();

			vector<GameObject*> appleList;
			int OBJtype;
			Vector3 originalPos;
			int* proState = NULL;
			bool speedState;


					 void SetBoundingVolume(CollisionVolume* vol) {boundingVolume = vol;}
   const CollisionVolume* GetBoundingVolume() const {return boundingVolume;}

					 void SetRenderObject(RenderObject* newObject) {renderObject = newObject;}
			RenderObject* GetRenderObject() const { return renderObject; }

					 void SetPhysicsObject(PhysicsObject* newObject) { physicsObject = newObject; }
		   PhysicsObject* GetPhysicsObject() const { return physicsObject; }

	     const Transform& GetConstTransform() const { return transform; }
			   Transform& GetTransform() { return transform; }

			const string& GetName() const { return name; }
					 void SetName(string newName) { name = newName; }

			virtual void OnCollisionBegin(GameObject* otherObject) {
				if (otherObject->OBJtype == 3){appleList.emplace_back(otherObject);}
				/*std::cout << "OnCollisionBegin event occured!\n";*/}

			virtual void OnCollisionSpeed(GameObject* otherObject) {
				if (otherObject->OBJtype == 6) { speedState = 1; }
				/*std::cout << "OnCollisionBegin event occured!\n";*/
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {/*std::cout << "OnCollisionEnd event occured!\n";*/}

			bool GetBroadphaseAABB(Vector3&outsize) const;
			void UpdateBroadphaseAABB();

			bool IsActive() const { return isActive; }
  NetworkObject* GetNetworkObject() const { return networkObject; }

			

		protected:
			Transform			transform;
			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;
			NetworkObject*		networkObject;


			string	name;
			bool isActive;
			Vector3 broadphaseAABB;
		};
	}
}

