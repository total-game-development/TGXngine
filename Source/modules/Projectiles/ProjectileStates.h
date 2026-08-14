#pragma once

#include "ProjectileInstance.h"

namespace TGX
{
class BulletInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 11;
	static constexpr int speed = 420;
	static constexpr float damage = 5;

	BulletInstance() : ProjectileInstance("bullet") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<BulletInstance>(*this);
	}
};

class GrenadeInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 8;
	static constexpr int speed = 360;
	static constexpr float damage = 12;

	GrenadeInstance() : ProjectileInstance("grenade") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<GrenadeInstance>(*this);
	}
};

class RocketInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 11;
	static constexpr int speed = 600;
	static constexpr int damage = 10;

	RocketInstance() : ProjectileInstance("rocket") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<RocketInstance>(*this);
	}
};

class MissileInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 11;
	static constexpr int speed = 720; // web weapon speed 12
	static constexpr float damage = 40;

	MissileInstance() : ProjectileInstance("missile") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<MissileInstance>(*this);
	}
};

class BombInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 11;
	static constexpr int speed = 360;
	static constexpr float damage = 150;

	BombInstance() : ProjectileInstance("bomb") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<BombInstance>(*this);
	}
};

class ShellInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 8;
	static constexpr int speed = 600;
	static constexpr float damage = 20;

	ShellInstance() : ProjectileInstance("shell") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<ShellInstance>(*this);
	}
};

// The laser tower's beam. It is a projectile like everything else rather than
// an instant hit, so that it damages and reads on screen through the same path
// as every other round -- but the speed is high enough that the flight is over
// within a frame or two at any range the tower can reach.
class LaserInstance : public ProjectileInstance
{
public:
	static constexpr int frames = 1;
	static constexpr int speed = 1200;
	static constexpr float damage = 20;

	LaserInstance() : ProjectileInstance("laser") {}
	int GetFrames() const override
	{
		return frames;
	}

	int GetSpeed() const override
	{
		return speed;
	}

	float GetDamage() const override
	{
		return damage;
	}

	Unique<ProjectileInstance> Clone() const override
	{
		return std::make_unique<LaserInstance>(*this);
	}
};
} // namespace TGX
