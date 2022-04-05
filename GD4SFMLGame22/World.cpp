#include "World.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <iostream>
#include <limits>

#include "Obstacle.hpp"
#include "ParticleNode.hpp"
#include "ParticleType.hpp"
#include "Pickup.hpp"
#include "PostEffect.hpp"
#include "SoundNode.hpp"
#include "Utility.hpp"

World::World(sf::RenderTarget& output_target, FontHolder& font, SoundPlayer& sounds, bool networked)
	: m_target(output_target)
	, m_camera(output_target.getDefaultView())
	, m_x_bound(m_world_bounds.width / 3.f)
	, m_textures()
	, m_fonts(font)
	, m_sounds(sounds)
	, m_scenegraph()
	, m_scene_layers()
	, m_world_bounds(0.f, 0.f, 12000, m_camera.getSize().x)
	, m_spawn_position(m_camera.getSize().x/2.f, m_world_bounds.height - m_camera.getSize().y /2.f)
	, m_scrollspeed(-200.f)
	, m_scrollspeed_compensation(1.f)
	, m_player_bike()
	, m_enemy_spawn_points()
	, m_obstacle_spawn_points()
	, m_pickup_spawn_points()
	, m_active_enemies()
	, m_networked_world(networked)
	, m_network_node(nullptr)
	, m_host_dead(false)
	, m_finish_sprite(nullptr)
{
	m_scene_texture.create(m_target.getSize().x, m_target.getSize().y);

	LoadTextures();
	BuildScene();
	m_camera.setCenter(m_spawn_position);
}

void World::SetWorldScrollCompensation(float compensation)
{
	m_scrollspeed_compensation = compensation;
}

void World::Update(sf::Time dt)
{
	//Update x Bound
	m_x_bound+=2;

	//Scroll the world
	m_camera.move(-(m_scrollspeed * dt.asSeconds() * m_scrollspeed_compensation), 0);

	for (Bike* a : m_player_bike)
	{
		a->SetVelocity(0.f, 0.f);
	}

	DestroyEntitiesOutsideView();
	//GuideMissiles();

	//Forward commands to the scenegraph until the command queue is empty
	while(!m_command_queue.IsEmpty())
	{
		m_scenegraph.OnCommand(m_command_queue.Pop(), dt);
	}
	AdaptPlayerVelocity();

	HandleCollisions();
	//Remove all destroyed entities
	//RemoveWrecks() only destroys the entities, not the pointers in m_player_bike
	auto first_to_remove = std::remove_if(m_player_bike.begin(), m_player_bike.end(), std::mem_fn(&Bike::IsMarkedForRemoval));
	m_player_bike.erase(first_to_remove, m_player_bike.end());
	m_scenegraph.RemoveWrecks();

	SpawnObstacles();
	SpawnPickups();

	//Apply movement
	m_scenegraph.Update(dt, m_command_queue);
	AdaptPlayerPosition();

	UpdateSounds();
}

void World::Draw()
{
	if(PostEffect::IsSupported())
	{
		m_scene_texture.clear();
		m_scene_texture.setView(m_camera);
		m_scene_texture.draw(m_scenegraph);
		m_scene_texture.display();
		m_bloom_effect.Apply(m_scene_texture, m_target);
	}
	else
	{
		m_target.setView(m_camera);
		m_target.draw(m_scenegraph);
	}
}

Bike* World::GetBike(int identifier) const
{
	for(Bike * a : m_player_bike)
	{
		if (a->GetIdentifier() == identifier)
		{
			return a;
		}
	}
	return nullptr;
}

void World::RemoveBike(int identifier)
{
	Bike* aircraft = GetBike(identifier);
	if (aircraft)
	{
		aircraft->Destroy();
		m_player_bike.erase(std::find(m_player_bike.begin(), m_player_bike.end(), aircraft));
	}
}

Bike* World::AddBike(int identifier)
{
	std::unique_ptr<Bike> player(new Bike(BikeType::kRacer, m_textures, m_fonts));
	sf::Vector2f spawn_area = m_camera.getCenter();
	spawn_area.x = m_x_bound - m_camera.getCenter().x / 2.0f;
	player->setPosition(spawn_area);
	player->SetIdentifier(identifier);

	m_player_bike.emplace_back(player.get());
	m_scene_layers[static_cast<int>(Layers::kUpperAir)]->AttachChild(std::move(player));
	return m_player_bike.back();
}

void World::CreatePickup(sf::Vector2f position, PickupType type)
{
	std::unique_ptr<Pickup> pickup(new Pickup(type, m_textures));
	pickup->setPosition(position);
	pickup->SetVelocity(0.f, 1.f);
	m_scene_layers[static_cast<int>(Layers::kUpperAir)]->AttachChild(std::move(pickup));
}

bool World::PollGameAction(GameActions::Action& out)
{
	return m_network_node->PollGameAction(out);
}

void World::SetCurrentBattleFieldPosition(float lineY)
{
	m_camera.setCenter(m_camera.getCenter().x, lineY - m_camera.getSize().y / 2);
	m_spawn_position.y = m_world_bounds.height;
}

void World::SetWorldHeight(float height)
{
	m_world_bounds.height = height;
}

bool World::HasAlivePlayer() const
{
	if (m_player_bike.size() >= 1 && !m_host_dead)
		return true;

	return false;
}

bool World::HasPlayerReachedEnd() const
{
	if(Bike* aircraft = GetBike(1))
	{
		if (!aircraft->IsHostDead() && !m_world_bounds.contains(aircraft->getPosition()))
			return true;
	}
	return false;
}

void World::LoadTextures()
{
	m_textures.Load(Textures::kEntities, "Media/Textures/Entities.png");
	m_textures.Load(Textures::kCity, "Media/Textures/Background.png");
	m_textures.Load(Textures::kExplosion, "Media/Textures/Explosion.png");
	m_textures.Load(Textures::kParticle, "Media/Textures/Particle.png");
	m_textures.Load(Textures::kFinishLine, "Media/Textures/FinishLine.png");
	m_textures.Load(Textures::kSpriteSheet, "Media/Textures/SpriteSheet.png");
	m_textures.Load(Textures::kBikeSpriteSheet, "Media/Textures/Bikes.png");
	m_textures.Load(Textures::kPickupSpriteSheet, "Media/Textures/PickupsV2.png");
}

void World::BuildScene()
{
	//Initialize the different layers
	for (std::size_t i = 0; i < static_cast<int>(Layers::kLayerCount); ++i)
	{
		Category::Type category = (i == static_cast<int>(Layers::kLowerAir)) ? Category::Type::kScene : Category::Type::kNone;
		SceneNode::Ptr layer(new SceneNode(category));
		m_scene_layers[i] = layer.get();
		m_scenegraph.AttachChild(std::move(layer));
	}

	//Prepare the background
	sf::Texture& city_texture = m_textures.Get(Textures::kCity);
	//sf::IntRect textureRect(m_world_bounds);
	//Tile the texture to cover our world
	city_texture.setRepeated(true);

	float view_height = m_camera.getSize().y;
	sf::IntRect texture_rect(m_world_bounds);
	texture_rect.height += static_cast<int>(view_height);

	//Add the background sprite to our scene
	std::unique_ptr<SpriteNode> city_sprite(new SpriteNode(city_texture, texture_rect));
	city_sprite->setPosition(m_world_bounds.left, m_world_bounds.top + 250);
	m_scene_layers[static_cast<int>(Layers::kBackground)]->AttachChild(std::move(city_sprite));

	// Add the finish line to the scene
	sf::Texture& finish_texture = m_textures.Get(Textures::kFinishLine);
	std::unique_ptr<SpriteNode> finish_sprite(new SpriteNode(finish_texture));
	finish_sprite->setPosition(11000.0f, 650);
	m_finish_sprite = finish_sprite.get();
	m_scene_layers[static_cast<int>(Layers::kBackground)]->AttachChild(std::move(finish_sprite));

	// Add particle node to the scene
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleType::kSmoke, m_textures));
	m_scene_layers[static_cast<int>(Layers::kLowerAir)]->AttachChild(std::move(smokeNode));

	// Add propellant particle node to the scene
	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleType::kPropellant, m_textures));
	m_scene_layers[static_cast<int>(Layers::kLowerAir)]->AttachChild(std::move(propellantNode));

	// Add sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(m_sounds));
	m_scenegraph.AttachChild(std::move(soundNode));

	if(m_networked_world)
	{
		std::unique_ptr<NetworkNode> network_node(new NetworkNode());
		m_network_node = network_node.get();
		m_scenegraph.AttachChild(std::move(network_node));
	}

	AddObstacles();
	AddPickups();
}

CommandQueue& World::GetCommandQueue()
{
	return m_command_queue;
}

void World::AdaptPlayerPosition()
{
	//Keep all players on the screen, at least border_distance from the border
	sf::FloatRect view_bounds = GetViewBounds();
	const float border_distance = 40.f;
	const float barrier_distance = 650.0f;
	const float offscreen_amount = 400.0f;
	for (Bike* aircraft : m_player_bike)
	{
		sf::Vector2f position = aircraft->getPosition();
		position.x = std::max(position.x, view_bounds.left - offscreen_amount);
		position.x = std::min(position.x, view_bounds.left + view_bounds.width - border_distance);
		position.y = std::max(position.y, barrier_distance);
		position.y = std::min(position.y, view_bounds.top + view_bounds.height - border_distance);
		aircraft->setPosition(position);
	}
}

void World::AdaptPlayerVelocity()
{
	for (Bike* aircraft : m_player_bike)
	{
		sf::Vector2f velocity = aircraft->GetVelocity();
		//if moving diagonally then reduce velocity
		if (velocity.x != 0.f && velocity.y != 0.f)
		{
			aircraft->SetVelocity(velocity / std::sqrt(2.f));
		}
	}
}

sf::FloatRect World::GetViewBounds() const
{
	return sf::FloatRect(m_camera.getCenter() - m_camera.getSize() / 2.f, m_camera.getSize());
}

sf::FloatRect World::GetBattlefieldBounds() const
{
	//Return camera bounds + width updated to constantly change
	sf::FloatRect bounds = GetViewBounds();
	bounds.left -= 250.0f;
	bounds.width = m_x_bound + 200;

	return bounds;
}

void World::SpawnObstacles()
{

	//Spawn an obstacle when they are relevant - they are relevant when they enter the battlefield bounds
	while (!m_obstacle_spawn_points.empty() && m_obstacle_spawn_points.back().m_x < GetBattlefieldBounds().width)
	{
		//std::cout << m_x_bound << " " << m_obstacle_spawn_points.back().m_x << std::endl;
		ObstacleSpawnPoint spawn = m_obstacle_spawn_points.back();
		std::cout << static_cast<int>(spawn.m_type) << std::endl;
		std::unique_ptr<Obstacle> obs(new Obstacle(spawn.m_type, m_textures));
		obs->setPosition(spawn.m_x, spawn.m_y);
		m_scene_layers[static_cast<int>(Layers::kUpperAir)]->AttachChild(std::move(obs));
		m_obstacle_spawn_points.pop_back();
	}
}

void World::AddObstacle(ObstacleType type, float relX, float relY)
{
	ObstacleSpawnPoint spawn(type, relX, relY);
	m_obstacle_spawn_points.emplace_back(spawn);
}

void World::AddObstacles()
{
	if (m_networked_world)
	{
		return;
	}

	//Add obstacles
	AddObstacle(ObstacleType::kBarrier, 200.f, 650.f);

	AddObstacle(ObstacleType::kTarSpill, 500.f, 750.f);

	AddObstacle(ObstacleType::kTarSpill, 800.f, 950.f);
	AddObstacle(ObstacleType::kAcidSpill, 850.f, 750.f);
	AddObstacle(ObstacleType::kBarrier, 900.f, 650.f);

	AddObstacle(ObstacleType::kBarrier, 1450.f, 950.f);
	AddObstacle(ObstacleType::kBarrier, 1950.f, 850.f);
	AddObstacle(ObstacleType::kBarrier, 2000.f, 650.f);

	AddObstacle(ObstacleType::kAcidSpill, 2250.f, 850.f);
	AddObstacle(ObstacleType::kTarSpill, 2250.f, 750.f);

	AddObstacle(ObstacleType::kTarSpill, 2850.f, 750.f);
	AddObstacle(ObstacleType::kAcidSpill, 3050.f, 750.f);
	AddObstacle(ObstacleType::kTarSpill, 3550.f, 950.f);
	AddObstacle(ObstacleType::kBarrier, 3750.f, 850.f);

	AddObstacle(ObstacleType::kAcidSpill, 3750.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 4000.f, 750.f);

	AddObstacle(ObstacleType::kBarrier, 4000.f, 800.f);
	AddObstacle(ObstacleType::kAcidSpill, 4250.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 4250.f, 750.f);


	AddObstacle(ObstacleType::kBarrier, 4500.f, 650.f);

	AddObstacle(ObstacleType::kTarSpill, 4500.f, 750.f);

	AddObstacle(ObstacleType::kTarSpill, 4700.f, 950.f);
	AddObstacle(ObstacleType::kAcidSpill, 4800.f, 750.f);
	AddObstacle(ObstacleType::kBarrier, 4800.f, 650.f);

	AddObstacle(ObstacleType::kBarrier, 5300.f, 800.f);
	AddObstacle(ObstacleType::kAcidSpill, 5450.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 5650.f, 750.f);

	AddObstacle(ObstacleType::kAcidSpill, 5750.f, 750.f);
	AddObstacle(ObstacleType::kTarSpill, 6250.f, 950.f);
	AddObstacle(ObstacleType::kBarrier, 6550.f, 850.f);

	AddObstacle(ObstacleType::kAcidSpill, 6750.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 7200.f, 750.f);

	AddObstacle(ObstacleType::kBarrier, 7350.f, 800.f);
	AddObstacle(ObstacleType::kAcidSpill, 7500.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 8000.f, 750.f);

	AddObstacle(ObstacleType::kBarrier, 8200.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 8400.f, 750.f);

	AddObstacle(ObstacleType::kAcidSpill, 8950.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 9300.f, 750.f);

	AddObstacle(ObstacleType::kBarrier, 9500.f, 800.f);
	AddObstacle(ObstacleType::kAcidSpill, 9750.f, 650.f);
	AddObstacle(ObstacleType::kTarSpill, 1050.f, 750.f);

	SortObstacles();
}

void World::SortObstacles()
{
	//Sort all enemies according to their x-value, such that lower enemies are checked first for spawning
	std::sort(m_obstacle_spawn_points.begin(), m_obstacle_spawn_points.end(), [](ObstacleSpawnPoint lhs, ObstacleSpawnPoint rhs)
		{
			return lhs.m_x > rhs.m_x;
		});
}

void World::SpawnPickups()
{
	//Spawn an Pickups when they are relevant - they are relevant when they enter the battlefield bounds
	while (!m_pickup_spawn_points.empty() && m_pickup_spawn_points.back().m_x < GetBattlefieldBounds().width)
	{
		//std::cout << m_x_bound << " " << m_obstacle_spawn_points.back().m_x << std::endl;
		PickupSpawnPoint spawn = m_pickup_spawn_points.back();
		std::cout << static_cast<int>(spawn.m_type) << std::endl;
		std::unique_ptr<Pickup> pickup(new Pickup(spawn.m_type, m_textures));
		pickup->setPosition(spawn.m_x, spawn.m_y);
		m_scene_layers[static_cast<int>(Layers::kUpperAir)]->AttachChild(std::move(pickup));
		m_pickup_spawn_points.pop_back();
	}
}

void World::AddPickup(PickupType type, float relX, float relY)
{
	PickupSpawnPoint spawn(type, relX, relY);
	m_pickup_spawn_points.emplace_back(spawn);
}

void World::AddPickups()
{
	if (m_networked_world)
	{
		return;
	}

	//Add pickups
	AddPickup(PickupType::kBoostRefill, 500.f, 850.f);
	AddPickup(PickupType::kBoostRefill, 1500.f, 850.f);
	AddPickup(PickupType::kBoostRefill, 3000.f, 850.f);
	AddPickup(PickupType::kInvincible, 3500.f, 850.f);

	AddPickup(PickupType::kBoostRefill, 6500.f, 850.f);
	AddPickup(PickupType::kBoostRefill, 4500.f, 850.f);
	AddPickup(PickupType::kBoostRefill, 5500.f, 850.f);
	AddPickup(PickupType::kInvincible, 7500.f, 850.f);

	AddPickup(PickupType::kBoostRefill, 8000.f, 850.f);
	AddPickup(PickupType::kBoostRefill, 8200.f, 850.f);
	AddPickup(PickupType::kInvincible, 9200.f, 850.f);

	SortPickups();
}

void World::SortPickups()
{
	//Sort all enemies according to their x-value, such that lower pickups are checked first for spawning
	std::sort(m_pickup_spawn_points.begin(), m_pickup_spawn_points.end(), [](PickupSpawnPoint lhs, PickupSpawnPoint rhs)
		{
			return lhs.m_x > rhs.m_x;
		});
}

bool MatchesCategories(SceneNode::Pair& colliders, Category::Type type1, Category::Type type2)
{
	unsigned int category1 = colliders.first->GetCategory();
	unsigned int category2 = colliders.second->GetCategory();
	if(type1 & category1 && type2 & category2)
	{
		return true;
	}
	else if(type1 & category2 && type2 & category1)
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}

void World::HandleCollisions()
{
	std::set<SceneNode::Pair> collision_pairs;
	m_scenegraph.CheckSceneCollision(m_scenegraph, collision_pairs);
	for(SceneNode::Pair pair : collision_pairs)
	{
		if(MatchesCategories(pair, Category::Type::kPlayerBike, Category::Type::kEnemyBike))
		{
			auto& player = static_cast<Bike&>(*pair.first);
			auto& enemy = static_cast<Bike&>(*pair.second);
			//Collision
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollision);
			enemy.Destroy();
			if (!player.GetInvincibility())
			{
				player.Damage(enemy.GetHitPoints());
			}
		}

		else if (MatchesCategories(pair, Category::Type::kPlayerBike, Category::Type::kPickup))
		{
			auto& player = static_cast<Bike&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Apply the pickup effect
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, SoundEffect::kCollectPickup);
		}
		
		if(MatchesCategories(pair, Category::Type::kPlayerBike, Category::kPlayerBike))
		{
			auto& player1 = static_cast<Bike&>(*pair.first);
			auto& player2 = static_cast<Bike&>(*pair.second);
			//player1.DecreaseSpeed(10.f);
			//player2.DecreaseSpeed(10.f);
			//player1.PlayLocalSound(m_command_queue, SoundEffect::kCollision);

			if (player1.GetInvincibility())
				player2.Destroy();
			else if (player2.GetInvincibility())
				player1.Destroy();
		}

		if (MatchesCategories(pair, Category::Type::kPlayerBike, Category::kPickup))
		{
			auto& player = static_cast<Bike&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);
			//Apply the pickup effect
			pickup.Apply(player);
			pickup.Destroy();
			player.PlayLocalSound(m_command_queue, SoundEffect::kBoostGet);
		}

		else if (MatchesCategories(pair, Category::Type::kPlayerBike, Category::Type::kObstacle))
		{
			auto& bike = static_cast<Bike&>(*pair.first);
			auto& obstacle = static_cast<Obstacle&>(*pair.second);

			//Apply the slowdown to the plane
			obstacle.Destroy();
			bike.PlayLocalSound(m_command_queue, SoundEffect::kCollision);

			if(!bike.GetInvincibility())
			{
				bike.DecreaseSpeed(obstacle.GetSlowdown());
				bike.Damage(10);
			}
		}
	}
}

void World::DestroyEntitiesOutsideView()
{
	Command command;
	command.category = Category::Type::kPlayerBike | Category::Type::kObstacle;
	command.action = DerivedAction<Entity>([this](Entity& e, sf::Time)
	{
		//Does the object intersect with the battlefield
		if (!GetBattlefieldBounds().intersects(e.GetBoundingRect()))
		{
			if (!e.IsHost())
				e.Remove();
			else
			{
				if(m_network_node)
					m_host_dead = true;

				e.SetHostDead(true);
			}
		}
	});
	m_command_queue.Push(command);
}

void World::UpdateSounds()
{
	sf::Vector2f listener_position;

	// 0 players (multiplayer mode, until server is connected) -> view center
	if (m_player_bike.empty())
	{
		listener_position = m_camera.getCenter();
	}

	// 1 or more players -> mean position between all aircrafts
	else
	{
		for (Bike* aircraft : m_player_bike)
		{
			listener_position += aircraft->GetWorldPosition();
		}

		listener_position /= static_cast<float>(m_player_bike.size());
	}

	// Set listener's position
	m_sounds.SetListenerPosition(listener_position);

	// Remove unused sounds
	m_sounds.RemoveStoppedSounds();
}
