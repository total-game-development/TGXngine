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

// The battleship's round. Frames and damage are the web version's missile
// entry; the speed is the one the battleship's own weapon block sets rather
// than the 24 on the missile template, because the web version lets a weapon
// override the template it fires and the battleship does. Speeds here are the
// web value times sixty, the same convention the rest of these carry -- the web
// advanced a round once per 60Hz tick where this engine advances it by the
// frame's elapsed seconds.
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
} // namespace TGX
