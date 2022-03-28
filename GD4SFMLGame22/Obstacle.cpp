#include "Obstacle.hpp"
#pragma once
#include <SFML/Graphics/RenderTarget.hpp>

#include "DataTables.hpp"
#include "Utility.hpp"
#include "ResourceHolder.hpp"

namespace
{
	const std::vector<ObstacleData> Table = InitializeObstacleData();
}

Obstacle::Obstacle(ObstacleType type, const TextureHolder& textures)
	: Entity(100)
	, m_type(type)
	, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
	, m_slow_down_amount(Table[static_cast<int>(type)].m_slow_down_amount)
	, m_is_marked_for_removal(false)
{
	Utility::CentreOrigin(m_sprite);
}

unsigned Obstacle::GetCategory() const
{
	return static_cast<int>(Category::kObstacle);
}

sf::FloatRect Obstacle::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

float Obstacle::GetSlowdown() const
{
	return m_slow_down_amount;
}

bool Obstacle::IsMarkedForRemoval() const
{
	return IsDestroyed();
}

void Obstacle::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_sprite, states);
}

void Obstacle::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	Entity::UpdateCurrent(dt, commands);
}
