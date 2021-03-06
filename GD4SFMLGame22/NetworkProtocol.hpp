#pragma once
#include <SFML/System/Vector2.hpp>

const unsigned short SERVER_PORT = 50000;

namespace Server
{
	//These are packets that come from the Server
	enum class PacketType
	{
		BroadcastMessage,
		InitialState,
		PlayerEvent,
		PlayerRealtimeChange,
		PlayerConnect,
		PlayerDisconnect,
		AcceptCoopPartner,
		SpawnPickup,
		SpawnObstacle,
		SpawnSelf,
		UpdateClientState,
		MissionSuccess,
		MissionFail,
		ServerStart,
		PlayerCountUpdate
	};
}

namespace Client
{
	//Messages sent from the Client
	enum class PacketType
	{
		PlayerEvent,
		PlayerRealtimeChange,
		RequestCoopPartner,
		PositionUpdate,
		GameEvent,
		Quit,
		PauseLobbyUpdate,
		ClientStart
	};
}

namespace GameActions
{
	enum Type
	{
		EnemyExplode
	};

	struct Action
	{
		Action()
		{
			
		}

		Action(Type type, sf::Vector2f position):type(type), position(position)
		{
			
		}

		Type type;
		sf::Vector2f position;
	};
}

