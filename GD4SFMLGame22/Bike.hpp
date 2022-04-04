#pragma once
#include "Entity.hpp"
#include "BikeType.hpp"
#include "ResourceIdentifiers.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include "Animation.hpp"
#include "CommandQueue.hpp"
#include "ProjectileType.hpp"
#include "TextNode.hpp"


class Bike : public Entity
{
public:
	Bike(BikeType type, const TextureHolder& textures, const FontHolder& fonts);
	unsigned int GetCategory() const override;
	bool GetInvincibility();

	void DisablePickups();
	int GetIdentifier();
	void SetIdentifier(int identifier);
	void SetBoost(bool hasBoost);
	void SetInvincibility(bool isInvincible);

	void UseBoost();
	void IncreaseSpeed(float speed);
	void DecreaseSpeed(float speed);
	float GetSpeed() const;
	//float GetOffroadResistance() const;

	bool GetBoost();
	void CollectInvincibility();
	void UpdateTexts();
	void UpdateMovementPattern(sf::Time dt);
	float GetMaxSpeed() const;

	sf::FloatRect GetBoundingRect() const override;
	bool IsMarkedForRemoval() const override;
	void Remove() override;
	void PlayLocalSound(CommandQueue& commands, SoundEffect effect);


private:
	void DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const override;
	void UpdateCurrent(sf::Time dt, CommandQueue& commands) override;

	bool IsAllied() const;
	void CreatePickup(SceneNode& node, const TextureHolder& textures) const;
	void CheckPickupDrop(CommandQueue& commands);
	void UpdateRollAnimation();
	void UpdateSpeed();

private:
	BikeType m_type;
	sf::Sprite m_sprite;
	Animation m_explosion;

	Command m_boost_command;

	bool m_is_marked_for_removal;
	bool m_show_explosion;
	bool m_explosion_began;
	bool m_spawned_pickup;
	bool m_pickups_enabled;

	TextNode* m_health_display;
	float m_travelled_distance;
	int m_directions_index;

	int m_identifier;
	int m_color_id;
	bool m_set_identifier;

	bool m_boost_ready;
	bool m_invincibility;
	bool m_use_boost;
	float m_speed;
	//float m_offroad_resistance;
	unsigned int m_counter;
	unsigned int m_invincible_counter;

	bool m_played_explosion_sound;
	bool m_is_player1;

	float m_max_speed;
	TextNode* m_boost_display;
	TextNode* m_player_display;
};

