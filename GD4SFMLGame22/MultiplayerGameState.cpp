#include "MultiplayerGameState.hpp"
#include "MusicPlayer.hpp"
#include "Utility.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <fstream>
#include <iostream>
#include <SFML/Network/Packet.hpp>


#include "PickupType.hpp"

sf::IpAddress GetAddressFromFile()
{
	{
		//Try to open existing file ip.txt
		std::ifstream input_file("ip.txt");
		std::string ip_address;
		if(input_file >> ip_address)
		{
			return ip_address;
		}
	}

	//If open/read failed, create a new file
	std::ofstream output_file("ip.txt");
	std::string local_address = "127.0.0.1";
	output_file << local_address;
	return local_address;
}

MultiplayerGameState::MultiplayerGameState(StateStack& stack, Context context, bool is_host)
: State(stack, context)
, m_world(*context.window, *context.fonts, *context.sounds, true)
, m_window(*context.window)
, m_texture_holder(*context.textures)
, m_connected(false)
, m_game_server(nullptr)
, m_active_state(true)
, m_has_focus(true)
, m_host(is_host)
, m_game_started(false)
, m_client_timeout(sf::seconds(2.f))
, m_time_since_last_packet(sf::seconds(0.f))
, m_in_lobby(true)
{
	m_broadcast_text.setFont(context.fonts->Get(Fonts::Main));
	m_broadcast_text.setPosition(1024.f - 200.f, 600.f);

	m_player_invitation_text.setFont(context.fonts->Get(Fonts::Main));
	m_player_invitation_text.setCharacterSize(20);
	m_player_invitation_text.setFillColor(sf::Color::White);
	m_player_invitation_text.setString("Press Enter to spawn player 2");
	m_player_invitation_text.setPosition(1000 - m_player_invitation_text.getLocalBounds().width, 760 - m_player_invitation_text.getLocalBounds().height);

	//We reuse this text for "Attempt to connect" and "Failed to connect" messages
	m_failed_connection_text.setFont(context.fonts->Get(Fonts::Main));
	m_failed_connection_text.setString("Attempting to connect...");
	m_failed_connection_text.setCharacterSize(35);
	m_failed_connection_text.setFillColor(sf::Color::White);
	Utility::CentreOrigin(m_failed_connection_text);
	m_failed_connection_text.setPosition(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f);

	//Render an "establishing connection" frame for user feedback
	m_window.clear(sf::Color::Black);
	m_window.draw(m_failed_connection_text);
	m_window.display();
	m_failed_connection_text.setString("Could not connect to the remote server");
	Utility::CentreOrigin(m_failed_connection_text);

	//lobby text
	m_in_lobby_text.setFont(context.fonts->Get(Fonts::Main));
	m_in_lobby_text.setString("Waiting in Lobby . . . . . . . . . . . . ");
	m_in_lobby_text.setCharacterSize(50);
	m_in_lobby_text.setFillColor(sf::Color::White);
	Utility::CentreOrigin(m_in_lobby_text);
	m_failed_connection_text.setPosition(m_window.getSize().x / 2.f, m_window.getSize().y / 2.f);


	sf::IpAddress ip;
	if(m_host)
	{
		m_game_server.reset(new GameServer(sf::Vector2f(m_window.getSize())));
		ip = "127.0.0.1";



		auto startButton = std::make_shared<GUI::Button>(context);
		startButton->setPosition(m_window.getSize().x /2.f, m_window.getSize().y/2.f);
		startButton->SetText("Start");
		startButton->SetCallback([this]()
			{
				sf::Packet packet;
				packet << static_cast<sf::Int32>(Server::PacketType::ServerStart);
				m_socket.send(packet);
				m_in_lobby = false;
			});

		m_in_lobby_ui.Pack(startButton);
	}
	else
	{
		ip = GetAddressFromFile();
	}

	if(m_socket.connect(ip, SERVER_PORT, sf::seconds(5.f)) == sf::TcpSocket::Done)
	{
		m_connected = true;
	}
	else
	{
		m_failed_connection_clock.restart();
	}

	m_socket.setBlocking(false);

	//Play game theme
	context.music->Play(MusicThemes::kMissionTheme);
}

void MultiplayerGameState::Draw()
{
	if(m_connected)
	{
		//Show broadcast messages in default view
		m_window.setView(m_window.getDefaultView());
		if (m_in_lobby)
		{
			//m_world.Draw();


			m_window.draw(m_in_lobby_text);
			m_window.draw(m_in_lobby_ui);

		}
		else if (!m_in_lobby)
		{
			m_world.Draw();

			if (!m_broadcasts.empty())
			{
				m_window.draw(m_broadcast_text);
			}

			if (m_local_player_identifiers.size() < 2 && m_player_invitation_time < sf::seconds(0.5f))
			{
				m_window.draw(m_player_invitation_text);
			}
		}

	}
	
	else
	{
		m_window.draw(m_failed_connection_text);
	}
}

bool MultiplayerGameState::Update(sf::Time dt)
{
	//Connected to the Server: Handle all the network logic
	if (m_connected)
	{
		if (m_in_lobby)
		{
			CheckPacket();

			if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
			{
				sf::Packet pause_update_packet;
				pause_update_packet << static_cast<sf::Int32>(Client::PacketType::PauseLobbyUpdate);
				pause_update_packet << static_cast<sf::Int32>(m_local_player_identifiers.size());

				for (sf::Int32 identifier : m_local_player_identifiers)
				{
					if (Bike* bike = m_world.GetBike(identifier))
					{
						pause_update_packet << identifier;
						//pause_update_packet << identifier << bike->getPosition().x << bike->getPosition().y << static_cast<sf::Int32>(bike->GetHitPoints()) << bike->GetBoost() << bike->GetInvincibility();
					}
				}
				m_socket.send(pause_update_packet);
				m_tick_clock.restart();
			}
			m_time_since_last_packet += dt;


		}
		else if (!m_in_lobby)
		{
			m_world.Update(dt);

			//Remove players whose aircraft were destroyed
			bool found_local_bike = false;
			for (auto itr = m_players.begin(); itr != m_players.end();)
			{
				//Check if there are no more local planes for remote clients
				if (std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), itr->first) != m_local_player_identifiers.end())
				{
					found_local_bike = true;
				}

				if (!m_world.GetBike(itr->first))
				{
					itr = m_players.erase(itr);

					//No more players left : Mission failed
					if (m_players.empty())

					{
						RequestStackPush(StateID::kGameOver);
					}
				}
				else
				{
					++itr;
				}
			}

			if (!found_local_bike && m_game_started)
			{
				RequestStackPush(StateID::kGameOver);
			}

			//Only handle the realtime input if the window has focus and the game is unpaused
			if (m_active_state && m_has_focus)
			{
				CommandQueue& commands = m_world.GetCommandQueue();
				for (auto& pair : m_players)
				{
					pair.second->HandleRealtimeInput(commands);
				}
			}

			//Always handle the network input
			CommandQueue& commands = m_world.GetCommandQueue();
			for (auto& pair : m_players)
			{
				pair.second->HandleRealtimeNetworkInput(commands);
			}

			CheckPacket();

			UpdateBroadcastMessage(dt);

			//Time counter fro blinking second player text
			m_player_invitation_time += dt;
			if (m_player_invitation_time > sf::seconds(1.f))
			{
				m_player_invitation_time = sf::Time::Zero;
			}

			//Events occurring in the game
			GameActions::Action game_action;
			while (m_world.PollGameAction(game_action))
			{
				sf::Packet packet;
				packet << static_cast<sf::Int32>(Client::PacketType::GameEvent);
				packet << static_cast<sf::Int32>(game_action.type);
				packet << game_action.position.x;
				packet << game_action.position.y;

				m_socket.send(packet);
			}

			//Regular position updates
			if (m_tick_clock.getElapsedTime() > sf::seconds(1.f / 20.f))
			{
				sf::Packet position_update_packet;
				position_update_packet << static_cast<sf::Int32>(Client::PacketType::PositionUpdate);
				position_update_packet << static_cast<sf::Int32>(m_local_player_identifiers.size());

				for (sf::Int32 identifier : m_local_player_identifiers)
				{
					if (Bike* bike = m_world.GetBike(identifier))
					{
						position_update_packet << identifier << bike->getPosition().x << bike->getPosition().y << static_cast<sf::Int32>(bike->GetHitPoints()) << bike->GetBoost() << bike->GetInvincibility();
					}
				}
				m_socket.send(position_update_packet);
				m_tick_clock.restart();
			}
			m_time_since_last_packet += dt;
		}
	}

	//Failed to connect and waited for more than 5 seconds: Back to menu
	else if(m_failed_connection_clock.getElapsedTime() >= sf::seconds(5.f))
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}
	return true;
}

void MultiplayerGameState::CheckPacket()
{
	//Handle messages from the server that may have arrived
	sf::Packet packet;
	if (m_socket.receive(packet) == sf::Socket::Done)
	{
		m_time_since_last_packet = sf::seconds(0.f);
		sf::Int32 packet_type;
		packet >> packet_type;
		HandlePacket(packet_type, packet);
	}
	else
	{
		//Check for timeout with the server
		if (m_time_since_last_packet > m_client_timeout)
		{
			m_connected = false;
			m_failed_connection_text.setString("Lost connection to the server");
			Utility::CentreOrigin(m_failed_connection_text);

			m_failed_connection_clock.restart();
		}
	}
}


bool MultiplayerGameState::HandleEvent(const sf::Event& event)
{
	if(m_in_lobby && m_host)
	{
		m_in_lobby_ui.HandleEvent(event);
	}
	else{

	//Game input handling
	CommandQueue& commands = m_world.GetCommandQueue();

	//Forward events to all players
	for(auto& pair : m_players)
	{
		pair.second->HandleEvent(event, commands);
	}

	if(event.type == sf::Event::KeyPressed)
	{
		//If enter pressed, add second player co-op only if there is only 1 player
		if(event.key.code == sf::Keyboard::Return && m_local_player_identifiers.size()==1)
		{
			sf::Packet packet;
			packet << static_cast<sf::Int32>(Client::PacketType::RequestCoopPartner);
			m_socket.send(packet);
		}
		//If escape is pressed, show the pause screen
		else if(event.key.code == sf::Keyboard::Escape)
		{
			DisableAllRealtimeActions();
			RequestStackPush(StateID::kNetworkPause);
		}
	}
	else if(event.type == sf::Event::GainedFocus)
	{
		m_has_focus = true;
	}
	else if(event.type == sf::Event::LostFocus)
	{
		m_has_focus = false;
	}
	}
	return true;
}

void MultiplayerGameState::OnActivate()
{
	m_active_state = true;
}

void MultiplayerGameState::OnDestroy()
{
	if(!m_host && m_connected)
	{
		//Inform server this client is dying
		sf::Packet packet;
		packet << static_cast<sf::Int32>(Client::PacketType::Quit);
		m_socket.send(packet);
	}
}

void MultiplayerGameState::DisableAllRealtimeActions()
{
	m_active_state = false;
	for(sf::Int32 identifier : m_local_player_identifiers)
	{
		m_players[identifier]->DisableAllRealtimeActions();
	}
}

void MultiplayerGameState::UpdateBroadcastMessage(sf::Time elapsed_time)
{
	if(m_broadcasts.empty())
	{
		return;
	}

	//Update broadcast timer
	m_broadcast_elapsed_time += elapsed_time;
	if(m_broadcast_elapsed_time > sf::seconds(2.f))
	{
		//If message has expired, remove it
		m_broadcasts.erase(m_broadcasts.begin());

		//Continue to display the next broadcast message
		if(!m_broadcasts.empty())
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
}

void MultiplayerGameState::HandlePacket(sf::Int32 packet_type, sf::Packet& packet)
{
	switch (static_cast<Server::PacketType>(packet_type))
	{
		//Send message to all Clients
	case Server::PacketType::BroadcastMessage:
	{
		std::string message;
		packet >> message;
		m_broadcasts.push_back(message);

		//Just added the first message, display immediately
		if (m_broadcasts.size() == 1)
		{
			m_broadcast_text.setString(m_broadcasts.front());
			Utility::CentreOrigin(m_broadcast_text);
			m_broadcast_elapsed_time = sf::Time::Zero;
		}
	}
	break;

	//Sent by the server to spawn player 1 bike on connect
	case Server::PacketType::SpawnSelf:
	{
		sf::Int32 bike_identifier;
		sf::Vector2f bike_position;
		packet >> bike_identifier >> bike_position.x >> bike_position.y;
		Bike* bike = m_world.AddBike(bike_identifier);
		bike->setPosition(bike_position);
		m_players[bike_identifier].reset(new Player(&m_socket, bike_identifier, GetContext().keys1));
		m_local_player_identifiers.push_back(bike_identifier);
		m_game_started = true;
	}
	break;

	case Server::PacketType::PlayerConnect:
	{
		sf::Int32 bike_identifier;
		sf::Vector2f bike_position;
		packet >> bike_identifier >> bike_position.x >> bike_position.y;

		Bike* bike = m_world.AddBike(bike_identifier);
		bike->setPosition(bike_position);
		m_players[bike_identifier].reset(new Player(&m_socket, bike_identifier, nullptr));
	}
	break;

	case Server::PacketType::PlayerDisconnect:
	{
		sf::Int32 bike_identifier;
		packet >> bike_identifier;
		m_world.RemoveBike(bike_identifier);
		m_players.erase(bike_identifier);
	}
	break;

	case Server::PacketType::InitialState:
	{
		sf::Int32 bike_count;
		float world_height, current_scroll;
		packet >> world_height >> current_scroll;

		m_world.SetWorldHeight(world_height);
		m_world.SetCurrentBattleFieldPosition(current_scroll);

		packet >> bike_count;
		for (sf::Int32 i = 0; i < bike_count; ++i)
		{
			sf::Int32 bike_identifier;
			sf::Int32 hitpoints;
			bool boost;
			bool invincibility;
			sf::Vector2f bike_position;
			packet >> bike_identifier >> bike_position.x >> bike_position.y >> hitpoints >> boost >> invincibility;

			Bike* bike = m_world.AddBike(bike_identifier);
			bike->setPosition(bike_position);
			bike->SetHitpoints(hitpoints);
			bike->SetBoost(boost);
			//bike->SetInvincibility(invincibility);

			m_players[bike_identifier].reset(new Player(&m_socket, bike_identifier, nullptr));
		}
	}
	break;

	case Server::PacketType::AcceptCoopPartner:
	{
		sf::Int32 bike_identifier;
		packet >> bike_identifier;

		m_world.AddBike(bike_identifier);
		m_players[bike_identifier].reset(new Player(&m_socket, bike_identifier, GetContext().keys2));
		m_local_player_identifiers.emplace_back(bike_identifier);
	}
	break;

	//Player event, like player becomes invincible
	case Server::PacketType::PlayerEvent:
	{
		sf::Int32 bike_identifier;
		sf::Int32 action;
		packet >> bike_identifier >> action;

		auto itr = m_players.find(bike_identifier);
		if (itr != m_players.end())
		{
			itr->second->HandleNetworkEvent(static_cast<PlayerAction>(action), m_world.GetCommandQueue());
		}
	}
	break;

	//Player's movement or boost keyboard state changes
	case Server::PacketType::PlayerRealtimeChange:
	{
		sf::Int32 bike_identifier;
		sf::Int32 action;
		bool action_enabled;
		packet >> bike_identifier >> action >> action_enabled;

		auto itr = m_players.find(bike_identifier);
		if (itr != m_players.end())
		{
			itr->second->HandleNetworkRealtimeChange(static_cast<PlayerAction>(action), action_enabled);
		}
	}
	break;

	//New Obstacle to be created
	case Server::PacketType::SpawnObstacle:
	{
		float height;
		sf::Int32 type;
		float relative_x;
		packet >> type >> height >> relative_x;

		m_world.AddObstacle(static_cast<ObstacleType>(type), relative_x, height);
		m_world.SortObstacles();
	}
	break;

	//Mission Successfully completed
	case Server::PacketType::MissionSuccess:
	{
		RequestStackPush(StateID::kMissionSuccess);
	}
	break;

	//Pickup created
	case Server::PacketType::SpawnPickup:
	{
		sf::Int32 type;
		sf::Vector2f position;
		packet >> type >> position.x >> position.y;
		std::cout << "Spawning pickup type " << type << std::endl;
		m_world.CreatePickup(position, static_cast<PickupType>(type));
	}
	break;

	case Server::PacketType::UpdateClientState:
	{
		float current_world_position;
		sf::Int32 bike_count;
		packet >> current_world_position >> bike_count;

		float current_view_position = m_world.GetViewBounds().top + m_world.GetViewBounds().height;

		//Set the world's scroll compensation according to whether the view is behind or ahead
		m_world.SetWorldScrollCompensation(current_view_position / current_world_position);

		for (sf::Int32 i = 0; i < bike_count; ++i)
		{
			sf::Vector2f bike_position;
			sf::Int32 bike_identifier;
			sf::Int32 hitpoints;
			bool boost;
			bool invincibility;

			packet >> bike_identifier >> bike_position.x >> bike_position.y >> hitpoints >> boost >> invincibility;

			Bike* bike = m_world.GetBike(bike_identifier);
			bool is_local_bike = std::find(m_local_player_identifiers.begin(), m_local_player_identifiers.end(), bike_identifier) != m_local_player_identifiers.end();
			if(bike && !is_local_bike)
			{
				sf::Vector2f interpolated_position = bike->getPosition() + (bike_position - bike->getPosition()) * 0.1f;
				bike->setPosition(interpolated_position);
				bike->SetHitpoints(hitpoints);
				bike->SetBoost(boost);
				//bike->SetInvincibility(invincibility);
			}
		}
	}
	break;
	case Server::PacketType::ServerStart:
	{
		m_in_lobby = false;
	}
	break;
	}
}
