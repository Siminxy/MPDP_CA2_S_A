#pragma once
#include "Entity.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include "CommandQueue.hpp"
#include "ObstacleType.hpp"
#include "TextNode.hpp"

class Obstacle : public Entity
{
public:
	Obstacle(ObstacleType type, const TextureHolder& textures);
	unsigned int GetCategory() const override;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	float GetSlowdown() const;

private:
	void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
	void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

private:
	ObstacleType m_type;
	sf::Sprite m_sprite;
	float m_slow_down_amount;

	bool m_is_marked_for_removal;
};