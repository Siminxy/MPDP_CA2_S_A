#include "Bike.hpp"

#include <iostream>

#include "DataTables.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

#include "ResourceHolder.hpp"
#include "Utility.hpp"
#include "DataTables.hpp"
#include "Pickup.hpp"
#include "PickupType.hpp"
#include "SoundNode.hpp"
#include "NetworkNode.hpp"


namespace
{
	const std::vector<BikeData> Table = InitializeBikeData();
}

Bike::Bike(BikeType type, const TextureHolder& textures, const FontHolder& fonts)
: Entity(Table[static_cast<int>(type)].m_hitpoints)
, m_type(type)
, m_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_texture_rect)
, m_max_speed(Table[static_cast<int>(type)].m_max_speed)
, m_explosion(textures.Get(Textures::kExplosion))
, m_boost_ready(true)
, m_is_marked_for_removal(false)
, m_show_explosion(true)
, m_set_identifier(false)
, m_invincibility(false)
, m_explosion_began(false)
, m_spawned_pickup(false)
, m_pickups_enabled(true)
, m_health_display(nullptr)
, m_boost_display(nullptr)
, m_player_display(nullptr)
, m_travelled_distance(0.f)
, m_directions_index(0)
, m_identifier(0)
, m_color_id(0)
{
	m_explosion.SetFrameSize(sf::Vector2i(256, 256));
	m_explosion.SetNumFrames(16);
	m_explosion.SetDuration(sf::seconds(1));

	sf::IntRect textureRect = Table[static_cast<int>(m_type)].m_texture_rect;
	textureRect.top += 30;
	m_sprite.setTextureRect(textureRect);

	Utility::CentreOrigin(m_sprite);
	Utility::CentreOrigin(m_explosion);

	std::unique_ptr<TextNode> healthDisplay(new TextNode(fonts, ""));
	healthDisplay->setPosition(0, 25);
	m_health_display = healthDisplay.get();
	AttachChild(std::move(healthDisplay));

	std::unique_ptr<TextNode> boostDisplay(new TextNode(fonts, ""));
	boostDisplay->setPosition(0, -20);
	m_boost_display = boostDisplay.get();
	AttachChild(std::move(boostDisplay));

	std::unique_ptr<TextNode> playerDisplay(new TextNode(fonts, "Player " + std::to_string(m_identifier)));

	playerDisplay->setPosition(0, 25);
	m_player_display = playerDisplay.get();
	AttachChild(std::move(playerDisplay));

	UpdateTexts();

}

void Bike::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	if(IsDestroyed() && m_show_explosion)
	{
		target.draw(m_explosion, states);
	}
	else
	{
		target.draw(m_sprite, states);
	}
}

void Bike::DisablePickups()
{
	m_pickups_enabled = false;
}


unsigned int Bike::GetCategory() const
{
	if (IsAllied())
	{
		return static_cast<int>(Category::kPlayerBike);
	}
	return static_cast<int>(Category::kEnemyBike);
}

bool Bike::GetInvincibility()
{
	return m_invincibility;
}

bool Bike::GetBoost()
{
	return m_boost_ready;
}

void Bike::UpdateTexts()
{
	if(IsDestroyed())
	{
		m_health_display->SetString("");
	}
	else
	{
		m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
		m_health_display->setPosition(0.f, 50.f);
		m_health_display->setRotation(-getRotation());

		m_player_display->SetString("Player " + std::to_string(m_identifier));

		if (m_boost_ready && m_boost_display)
		{
			m_boost_display->SetString("Boost Ready!");
		}
		else
		{
			m_boost_display->SetString("");
		}
	}

}

void Bike::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	UpdateTexts();
	UpdateRollAnimation();
	UpdateSpeed();

	//Set or remove invincibility
	if (m_invincible_counter > 0 || m_invincibility)
	{
		m_invincible_counter++;
	}
	if (m_invincible_counter > 100)
	{
		m_invincibility = false;
		m_invincible_counter = 0;
	}

	//Entity has been destroyed, possibly drop pickup, mark for removal
	if(IsDestroyed())
	{
		//CheckPickupDrop(commands);
		m_explosion.Update(dt);

		// Play explosion sound only once
		if (!m_explosion_began)
		{
			SoundEffect soundEffect = (Utility::RandomInt(2) == 0) ? SoundEffect::kExplosion1 : SoundEffect::kExplosion2;
			PlayLocalSound(commands, soundEffect);

			//Emit network game action for enemy explodes
			if(!IsAllied())
			{
				sf::Vector2f position = GetWorldPosition();

				Command command;
				command.category = Category::kNetwork;
				command.action = DerivedAction<NetworkNode>([position](NetworkNode& node, sf::Time)
				{
					node.NotifyGameAction(GameActions::EnemyExplode, position);
				});

				commands.Push(command);
			}

			m_explosion_began = true;
		}
		return;
	}

	// Update enemy movement pattern; apply velocity
	UpdateMovementPattern(dt);
	Entity::UpdateCurrent(dt, commands);
}

int	Bike::GetIdentifier()
{
	return m_identifier;
}

void Bike::SetIdentifier(int identifier)
{
	m_identifier = identifier;
}

void Bike::UpdateMovementPattern(sf::Time dt)
{
	//Enemy AI
	const std::vector<Direction>& directions = Table[static_cast<int>(m_type)].m_directions;
	if(!directions.empty())
	{
		//Move along the current direction, change direction
		if(m_travelled_distance > directions[m_directions_index].m_distance)
		{
			m_directions_index = (m_directions_index + 1) % directions.size();
			m_travelled_distance = 0.f;
		}

		//Compute velocity from direction
		double radians = Utility::ToRadians(directions[m_directions_index].m_angle + 90.f);
		float vx = GetMaxSpeed() * std::cos(radians);
		float vy = GetMaxSpeed() * std::sin(radians);

		SetVelocity(vx, vy);
		m_travelled_distance += GetMaxSpeed() * dt.asSeconds();
	}
}

float Bike::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_max_speed;
}

bool Bike::IsAllied() const
{
	return m_type == BikeType::kRacer;
}

sf::FloatRect Bike::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_sprite.getGlobalBounds());
}

bool Bike::IsMarkedForRemoval() const
{
	return IsDestroyed() && (m_explosion.IsFinished() || !m_show_explosion);
}

void Bike::Remove()
{
	Entity::Remove();
	m_show_explosion = false;
}

//void Bike::CreatePickup(SceneNode& node, const TextureHolder& textures) const
//{
//	auto type = static_cast<PickupType>(Utility::RandomInt(static_cast<int>(PickupType::kPickupCount)));
//	std::unique_ptr<Pickup> pickup(new Pickup(type, textures));
//	pickup->setPosition(GetWorldPosition());
//	pickup->SetVelocity(0.f, 0.f);
//	node.AttachChild(std::move(pickup));
//}
//

void Bike::UpdateRollAnimation()
{
	int const invincibility = 270;
	int const bike_count = 9;
	if (Table[static_cast<int>(m_type)].m_has_roll_animation)
	{
		//Note, spritesheet is set up where this value is the neutral, aka. the middle texture
		sf::IntRect textureRect = Table[static_cast<int>(m_type)].m_texture_rect;

		if(!m_set_identifier)
		{
			m_color_id = m_identifier;
			m_set_identifier = true;
		}
		//Changes the top distance to change bike based off identifier
		if (m_identifier > 1)
			textureRect.top = (30 * m_color_id) % (bike_count * 30);

		//Sets the bike to the 'invincibility' bike
		if (m_invincibility)
			textureRect.top = invincibility;

		// Roll left: Texture rect offset to the left
		if (GetVelocity().x < 0.f)
			textureRect.left -= textureRect.width;

		// Roll right: Texture rect offset to the right
		else if (GetVelocity().x > 0.f)
			textureRect.left += textureRect.width +1;

		m_sprite.setTextureRect(textureRect);
	}
}

void Bike::PlayLocalSound(CommandQueue& commands, SoundEffect effect)
{
	sf::Vector2f world_position = GetWorldPosition();

	Command command;
	command.category = Category::kSoundEffect;
	command.action = DerivedAction<SoundNode>(
		[effect, world_position](SoundNode& node, sf::Time)
	{
		node.PlaySound(effect, world_position);
	});

	commands.Push(command);
}


void Bike::UpdateSpeed()
{
	float maxSpeedBoost = (m_max_speed / 100.f);

	if (m_use_boost)
	{
		m_speed += maxSpeedBoost;
		++m_counter;
		if (m_counter > 250)
		{
			m_use_boost = false;
			m_counter = 0;
		}
	}

	if (m_speed < 100.f)
	{
		m_speed += maxSpeedBoost;
	}
	else if (m_speed < m_max_speed)
	{
		m_speed += (maxSpeedBoost / 10.f);
	}
	else if (!m_use_boost && m_speed > m_max_speed)
	{
		m_speed -= maxSpeedBoost;
	}
}

float Bike::GetSpeed() const
{
	return m_speed;
}

void Bike::CollectInvincibility()
{
	std::cout << "INVINCIBLE" << std::endl;
	m_invincibility = true;
	m_invincible_counter = 1;
}

void Bike::UseBoost()
{
	if (m_boost_ready && !m_use_boost)
	{
		m_boost_ready = false;
		m_use_boost = true;
	}
}

void Bike::IncreaseSpeed(float speedUp)
{
	m_speed = m_speed + (m_speed * speedUp);
	if (m_speed > m_max_speed)
		m_speed = m_max_speed;
}

void Bike::DecreaseSpeed(float speedDown)
{
	m_speed = m_speed - (m_speed * speedDown);
	//m_speed = m_speed - (m_speed * m_offroad_resistance);
	if (m_speed < 0)
		m_speed = 0;
}

void Bike::SetBoost(bool hasBoost)
{
	m_boost_ready = hasBoost;
}