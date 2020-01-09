#pragma once
#include"NetworkBase.h"
#include<iostream>

using namespace std;

namespace NCL {
	namespace CSC8503 {
		class NetWookPro: public PacketReceiver{
		public:
			NetWookPro(string name) {
				this -> name = name;
			}
			string playInformation = "no other players";
			string name;
			string timeInfotmation ;
			positionStruct playerPosition;

			void ReceivePacket(int type, GamePacket* payload, int source) {
				if (type == String_Message) {
					StringPacket* realPacket = (StringPacket*)payload;
					string msg = realPacket -> GetStringFromData();
					if (msg != ""){playInformation = msg;}
				}
				else if (type == Time_Message) {
					TimePacket* realPacket = (TimePacket*)payload;
					string time = realPacket->GetStringFromData();
					if (time != "") { timeInfotmation = time; }
				}
				else if (type == Position_Message) {
					PositionPacket* realPacket = (PositionPacket*)payload;
					positionStruct tempPos = realPacket->GetPositionFromData();
					playerPosition.X = realPacket->PlayerPosition.X;
					playerPosition.Y = realPacket->PlayerPosition.Z;
					playerPosition.Z = realPacket->PlayerPosition.Y;
				}
			}
		protected:

		};
	}
}
