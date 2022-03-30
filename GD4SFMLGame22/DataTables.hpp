#pragma once
#include <functional>
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>

#include "ResourceIdentifiers.hpp"

class Bike;

struct Direction
{
	Direction(float angle, float distance)
		: m_angle(angle), m_distance(distance)
	{
	}
	float m_angle;
	float m_distance;
};

struct BikeData
{
	int m_hitpoints;
	float m_speed;
	Textures m_texture;
	sf::IntRect m_texture_rect;
	std::vector<Direction> m_directions;
	bool m_has_roll_animation;
	//float m_offroad_resistance;
	float m_max_speed;
};

struct PickupData
{
	std::function<void(Bike&)> m_action;
	Textures m_texture;
	sf::IntRect m_texture_rect;
};

struct ObstacleData
{
	float m_slow_down_amount;
	Textures m_texture;
	sf::IntRect m_texture_rect;
};

struct ParticleData
{
	sf::Color						m_color;
	sf::Time						m_lifetime;
};

std::vector<BikeData> InitializeBikeData();
std::vector<PickupData> InitializePickupData();
std::vector<ParticleData> InitializeParticleData();
std::vector<ObstacleData> InitializeObstacleData();
