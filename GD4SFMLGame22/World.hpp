#pragma once
#include "ResourceHolder.hpp"
#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"
#include "SpriteNode.hpp"
#include "Bike.hpp"
#include "Layers.hpp"
#include "BikeType.hpp"
#include "NetworkNode.hpp"

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <array>
#include <SFML/Graphics/RenderWindow.hpp>

#include "BloomEffect.hpp"
#include "CommandQueue.hpp"
#include "SoundPlayer.hpp"

#include "NetworkProtocol.hpp"
#include "ObstacleType.hpp"
#include "PickupType.hpp"
#include "PlayerAction.hpp"

namespace sf
{
	class RenderTarget;
}



class World : private sf::NonCopyable
{
public:
	explicit World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked=false);
	void Update(sf::Time dt);
	void Draw();

	sf::FloatRect GetViewBounds() const;
	CommandQueue& GetCommandQueue();

	Bike* AddBike(int identifier);
	void RemoveBike(int identifier);
	void SetCurrentBattleFieldPosition(float line_y);
	void SetWorldHeight(float height);

	void AddObstacle(ObstacleType type, float relX, float relY);
	void SortObstacles();
	void SortPickups();
	void AddPickup(PickupType type, float relX, float relY);

	bool HasAlivePlayer() const;
	bool HasPlayerReachedEnd() const;

	void SetWorldScrollCompensation(float compensation);
	Bike* GetBike(int identifier) const;
	sf::FloatRect GetBattlefieldBounds() const;
	void CreatePickup(sf::Vector2f position, PickupType type);
	bool PollGameAction(GameActions::Action& out);


private:
	void LoadTextures();
	void BuildScene();
	void AdaptPlayerPosition();
	void AdaptPlayerVelocity();

	void HandleCollisions();
	void DestroyEntitiesOutsideView();
	void UpdateSounds();

	void SpawnObstacles();
	void SpawnPickups();
	void AddObstacles();
	void AddPickups();

private:
	struct SpawnPoint
	{
		SpawnPoint(BikeType type, float x, float y) : m_type(type), m_x(x), m_y(y)
		{
			
		}
		BikeType m_type;
		float m_x;
		float m_y;
	};

	struct ObstacleSpawnPoint
	{
		ObstacleSpawnPoint(ObstacleType type, float x, float y) : m_type(type), m_x(x), m_y(y)
		{

		}
		ObstacleType m_type;
		float m_x;
		float m_y;
	};

	struct PickupSpawnPoint
	{
		PickupSpawnPoint(PickupType type, float x, float y) : m_type(type), m_x(x), m_y(y)
		{

		}
		PickupType m_type;
		float m_x;
		float m_y;
	};
	

private:
	sf::RenderTarget& m_target;
	sf::RenderTexture m_scene_texture;
	sf::View m_camera;
	TextureHolder m_textures;
	FontHolder& m_fonts;
	SoundPlayer& m_sounds;
	SceneNode m_scenegraph;
	std::array<SceneNode*, static_cast<int>(Layers::kLayerCount)> m_scene_layers;
	CommandQueue m_command_queue;

	sf::FloatRect m_world_bounds;
	sf::Vector2f m_spawn_position;
	float m_scrollspeed;
	float m_x_bound;
	float m_scrollspeed_compensation;
	std::vector<Bike*> m_player_bike;
	std::vector<SpawnPoint> m_enemy_spawn_points;
	std::vector<ObstacleSpawnPoint> m_obstacle_spawn_points;
	std::vector<PickupSpawnPoint> m_pickup_spawn_points;
	std::vector<Bike*>	m_active_enemies;

	BloomEffect m_bloom_effect;
	bool m_networked_world;
	NetworkNode* m_network_node;
	SpriteNode* m_finish_sprite;
};

