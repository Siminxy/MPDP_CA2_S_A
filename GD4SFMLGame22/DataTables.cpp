#include "DataTables.hpp"
#include "BikeType.hpp"
#include "Bike.hpp"
#include "ObstacleType.hpp"
#include "ParticleType.hpp"
#include "PickupType.hpp"
#include "ProjectileType.hpp"

std::vector<BikeData> InitializeBikeData()
{
	std::vector<BikeData> data(static_cast<int>(BikeType::kBikeCount));

	data[static_cast<int>(BikeType::kRacer)].m_hitpoints = 100;
	data[static_cast<int>(BikeType::kRacer)].m_speed = 250.f;
	data[static_cast<int>(BikeType::kRacer)].m_max_speed = 450.f;
	data[static_cast<int>(BikeType::kRacer)].m_texture = Textures::kBikeSpriteSheet;
	data[static_cast<int>(BikeType::kRacer)].m_texture_rect = sf::IntRect(58, 0, 57, 29);
	data[static_cast<int>(BikeType::kRacer)].m_has_roll_animation = true;
	//data[static_cast<int>(BikeType::kRacer)].m_offroad_resistance = 0.2f;

	return data;
}

std::vector<ObstacleData> InitializeObstacleData()
{
	std::vector<ObstacleData> data(static_cast<int>(ObstacleType::kObstacleCount));

	data[static_cast<int>(ObstacleType::kTarSpill)].m_texture = Textures::kSpriteSheet;
	data[static_cast<int>(ObstacleType::kTarSpill)].m_texture_rect = sf::IntRect(123, 153, 45, 19);
	data[static_cast<int>(ObstacleType::kTarSpill)].m_slow_down_amount = 0.4f;

	data[static_cast<int>(ObstacleType::kAcidSpill)].m_texture = Textures::kSpriteSheet;
	data[static_cast<int>(ObstacleType::kAcidSpill)].m_texture_rect = sf::IntRect(124, 132, 45, 19);
	data[static_cast<int>(ObstacleType::kAcidSpill)].m_slow_down_amount = 0.2f;

	data[static_cast<int>(ObstacleType::kBarrier)].m_texture = Textures::kSpriteSheet;
	data[static_cast<int>(ObstacleType::kBarrier)].m_texture_rect = sf::IntRect(182, 86, 17, 29);
	data[static_cast<int>(ObstacleType::kBarrier)].m_slow_down_amount = 0.9f;
	return data;
}

std::vector<PickupData> InitializePickupData()
{
	std::vector<PickupData> data(static_cast<int>(PickupType::kPickupCount));

	data[static_cast<int>(PickupType::kInvincible)].m_texture = Textures::kPickupSpriteSheet;
	data[static_cast<int>(PickupType::kInvincible)].m_texture_rect = sf::IntRect(40, 0, 40, 40);
	data[static_cast<int>(PickupType::kInvincible)].m_action = [](Bike& a) {a.CollectInvincibility(); };

	data[static_cast<int>(PickupType::kBoostRefill)].m_texture = Textures::kPickupSpriteSheet;
	data[static_cast<int>(PickupType::kBoostRefill)].m_texture_rect = sf::IntRect(0, 0, 40, 40);
	data[static_cast<int>(PickupType::kBoostRefill)].m_action = [](Bike& a) {a.SetBoost(true); };

	return data;
}

std::vector<ParticleData> InitializeParticleData()
{
	std::vector<ParticleData> data(static_cast<int>(ParticleType::kParticleCount));

	data[static_cast<int>(ParticleType::kPropellant)].m_color = sf::Color(255, 255, 50);
	data[static_cast<int>(ParticleType::kPropellant)].m_lifetime = sf::seconds(0.6f);

	data[static_cast<int>(ParticleType::kSmoke)].m_color = sf::Color(50, 50, 50);
	data[static_cast<int>(ParticleType::kSmoke)].m_lifetime = sf::seconds(4.f);

	return data;
}



