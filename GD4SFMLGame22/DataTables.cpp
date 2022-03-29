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
	data[static_cast<int>(BikeType::kRacer)].m_speed = 200.f;
	data[static_cast<int>(BikeType::kRacer)].m_max_speed = 250.f;
	data[static_cast<int>(BikeType::kRacer)].m_fire_interval = sf::seconds(1);
	data[static_cast<int>(BikeType::kRacer)].m_texture = Textures::kBikeSpriteSheet;
	data[static_cast<int>(BikeType::kRacer)].m_texture_rect = sf::IntRect(58, 0, 57, 29);
	data[static_cast<int>(BikeType::kRacer)].m_has_roll_animation = true;
	data[static_cast<int>(BikeType::kRacer)].m_offroad_resistance = 0.2f;

	data[static_cast<int>(BikeType::kNitro)].m_hitpoints = 20;
	data[static_cast<int>(BikeType::kNitro)].m_speed = 80.f;
	data[static_cast<int>(BikeType::kNitro)].m_fire_interval = sf::Time::Zero;
	data[static_cast<int>(BikeType::kNitro)].m_texture = Textures::kEntities;
	data[static_cast<int>(BikeType::kNitro)].m_texture_rect = sf::IntRect(144, 0, 84, 64);
	//AI
	data[static_cast<int>(BikeType::kNitro)].m_directions.emplace_back(Direction(+45.f, 80.f));
	data[static_cast<int>(BikeType::kNitro)].m_directions.emplace_back(Direction(-45.f, 160.f));
	data[static_cast<int>(BikeType::kNitro)].m_directions.emplace_back(Direction(+45.f, 80.f));
	data[static_cast<int>(BikeType::kNitro)].m_has_roll_animation = false;

	data[static_cast<int>(BikeType::kOffroader)].m_hitpoints = 40;
	data[static_cast<int>(BikeType::kOffroader)].m_speed = 50.f;
	data[static_cast<int>(BikeType::kOffroader)].m_fire_interval = sf::seconds(2);
	data[static_cast<int>(BikeType::kOffroader)].m_texture = Textures::kEntities;
	data[static_cast<int>(BikeType::kOffroader)].m_texture_rect = sf::IntRect(228, 0, 60, 59);
	//AI
	data[static_cast<int>(BikeType::kOffroader)].m_directions.emplace_back(Direction(+45.f, 50.f));
	data[static_cast<int>(BikeType::kOffroader)].m_directions.emplace_back(Direction(0.f, 50.f));
	data[static_cast<int>(BikeType::kOffroader)].m_directions.emplace_back(Direction(-45.f, 100.f));
	data[static_cast<int>(BikeType::kOffroader)].m_directions.emplace_back(Direction(0.f, 50.f));
	data[static_cast<int>(BikeType::kOffroader)].m_directions.emplace_back(Direction(+45.f, 50.f));
	data[static_cast<int>(BikeType::kOffroader)].m_has_roll_animation = false;
	return data;
}

//All bike sprites are lined up the same, only add +30 to top distance. 4 in all.
//data[static_cast<int>(BikeType::kPlayer1)].m_texture_rect = sf::IntRect(58, 0, 57, 29);
//data[static_cast<int>(BikeType::kPlayer2)].m_texture_rect = sf::IntRect(58, 30, 57, 29);
//data[static_cast<int>(BikeType::kOffroader)].m_texture_rect = sf::IntRect(58, 60, 57, 29);
//data[static_cast<int>(BikeType::kRacer)].m_texture_rect = sf::IntRect(58, 90, 57, 29);


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

std::vector<ProjectileData> InitializeProjectileData()
{
	std::vector<ProjectileData> data(static_cast<int>(ProjectileType::kProjectileCount));

	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture = Textures::kEntities;
	data[static_cast<int>(ProjectileType::kAlliedBullet)].m_texture_rect = sf::IntRect(175, 64, 3, 14);

	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_damage = 10;
	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_speed = 300;
	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture = Textures::kEntities;
	data[static_cast<int>(ProjectileType::kEnemyBullet)].m_texture_rect = sf::IntRect(178, 64, 3, 14);

	data[static_cast<int>(ProjectileType::kMissile)].m_damage = 200;
	data[static_cast<int>(ProjectileType::kMissile)].m_speed = 150.f;
	data[static_cast<int>(ProjectileType::kMissile)].m_texture = Textures::kEntities;
	data[static_cast<int>(ProjectileType::kMissile)].m_texture_rect = sf::IntRect(160, 64, 15, 32);
	return data;
}


std::vector<PickupData> InitializePickupData()
{
	std::vector<PickupData> data(static_cast<int>(PickupType::kPickupCount));

	data[static_cast<int>(PickupType::kInvincible)].m_texture = Textures::kSpriteSheet;
	data[static_cast<int>(PickupType::kInvincible)].m_texture_rect = sf::IntRect(150, 171, 29, 28);
	data[static_cast<int>(PickupType::kInvincible)].m_action = [](Bike& a) {a.Repair(25); };

	data[static_cast<int>(PickupType::kBoostRefill)].m_texture = Textures::kSpriteSheet;
	data[static_cast<int>(PickupType::kBoostRefill)].m_texture_rect = sf::IntRect(171, 122, 29, 28);
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



