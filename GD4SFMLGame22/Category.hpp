#pragma once
//SceneNode category used to dispatch commands
namespace Category
{
	enum Type
	{
		kNone = 0,
		kScene = 1 << 0,
		kPlayerBike = 1 << 1,
		kAlliedBike = 1 << 2,
		kEnemyBike = 1 << 3,
		kPickup = 1 << 4,
		kAlliedProjectile = 1 << 5,
		kEnemyProjectile = 1 << 6,
		kParticleSystem = 1 << 7,
		kSoundEffect = 1 << 8,
		kNetwork = 1 << 9,
		kObstacle = 1 << 10,

		kBike = kPlayerBike | kAlliedBike | kEnemyBike,
		kProjectile = kAlliedProjectile | kEnemyProjectile,
	};
}