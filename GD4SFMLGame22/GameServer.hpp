#pragma once
#include <map>
#include <memory>
#include <string>
#include <SFML/Config.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Thread.hpp>

class GameServer
{
public:
	explicit GameServer(sf::Vector2f battlefield_size);
	~GameServer();
	void NotifyPlayerSpawn(sf::Int32 bike_identifier);
	void NotifyPlayerRealtimeChange(sf::Int32 bike_identifier, sf::Int32 action, bool action_enabled);
	void NotifyPlayerEvent(sf::Int32 bike_identifier, sf::Int32 action);

private:
	struct RemotePeer
	{
		RemotePeer();
		sf::TcpSocket m_socket;
		sf::Time m_last_packet_time;
		std::vector<sf::Int32> m_bike_identifiers;
		bool m_ready;
		bool m_timed_out;
	};

	struct BikeInfo
	{
		sf::Vector2f m_position;
		sf::Int32 m_hitpoints;
		bool m_boost;
		bool m_invincibility;
		bool m_color_id;
		std::map<sf::Int32, bool> m_realtime_actions;
	};

	typedef std::unique_ptr<RemotePeer> PeerPtr;

private:
	void SetListening(bool enable);
	void ExecutionThread();
	void Tick();
	sf::Time Now() const;

	void HandleIncomingPackets();
	void HandleIncomingPacket(sf::Packet& packet, RemotePeer& receiving_peer, bool& detected_timeout);

	void HandleIncomingConnections();
	void HandleDisconnections();

	void InformWorldState(sf::TcpSocket& socket);
	void BroadcastMessage(const std::string& message);
	void SendToAll(sf::Packet& packet);
	void UpdateClientState();

private:
	sf::Thread m_thread;
	sf::Clock m_clock;
	sf::TcpListener m_listener_socket;
	bool m_listening_state;
	sf::Time m_client_timeout;

	std::size_t m_max_connected_players;
	std::size_t m_connected_players;

	float m_world_width;
	sf::FloatRect m_battlefield_rect;
	float m_battlefield_scrollspeed;

	std::size_t m_bike_count;
	std::map<sf::Int32, BikeInfo> m_bike_info;

	std::vector<PeerPtr> m_peers;
	sf::Int32 m_bike_identifier_counter;
	float m_x_bounds;
	bool m_waiting_thread_end;

	sf::Time m_last_spawn_time;
	sf::Time m_last_pickup_spawn_time;
	sf::Time m_time_for_next_spawn;
	sf::Time m_time_for_next_pickup_spawn;

	bool m_in_lobby;
};

