#pragma once

#include <limits>
#include "Flags.h"
#include "ItemInstance.h"
#include "Logs.h"
#include "Point.h"

namespace TGX
{
struct CirclePath
{
	float x = 0.0f;
	float y = 0.0f;
	float speed = 0.0f;
};

class AircraftState : public ItemInstance
{
public:
	AircraftState()
	{
		originX = 2.0f;
		originY = 2.0f;
		priority = 3;
	}

	~AircraftState() override = default;

	static constexpr float pathSpeedScale = 60.0f;
	static constexpr float initialTakeOffSpeed = pathSpeedScale;
	static constexpr float farthestDistance = std::numeric_limits<float>::max();

	bool landed = true;
	bool takingOff = false;
	bool holding = false;
	bool turning = false;

	// Sweeping toward a destination and free to take on anything that comes
	// into range on the way. Armed by Search and dropped the moment a target is
	// taken, so an aircraft only hunts when it was sent hunting.
	bool isAttackMove = false;

	// Rounds left. Counted down as the web version does, where it tracks the
	// magazine without stopping an empty aircraft firing.
	int ammo = 0;

	int deployUid = INT_MIN;
	int currentHangerPosition = 0;
	int landingDirection = 0;
	int takingOffIndex = 0;
	int landingIndex = 0;
	int circleIndex = 0;
	int circleCounter = 0;

	int animationLimit = 1;
	int animationSpeedLimit = 2;

	float takeOffSpeed = initialTakeOffSpeed;
	float wayPointX = 0.0f;
	float wayPointY = 0.0f;
	float approachPositionX = 0.0f;
	float approachPositionY = 0.0f;
	float takeOffPositionX = 0.0f;
	float takeOffPositionY = 0.0f;
	float breakAwayToX = 0.0f;
	float breakAwayToY = 0.0f;
	float minDistance = farthestDistance;
	float previousDistance = farthestDistance;
	float reloadTimeLeft = 0.0f;

	static constexpr int cellCollisionMode = Flags::CELL_COLLISION_MODE_OFF;

	int GetCellCollisionMode() const override
	{
		return cellCollisionMode;
	}

	void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}

	void RemoveFromGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}

	static constexpr Array<float, 8> cosDirectionAngles = {
		1.f, 0.70710678f, 6.123234e-17f, -0.70710678f,
		-1.f, -0.70710678f, -1.83697e-16f, 0.70710678f};

	static constexpr Array<float, 8> sinDirectionAngles = {
		0.f, 0.70710678f, 1.f, 0.70710678f,
		1.2246468e-16f, -0.70710678f, -1.f, -0.70710678f};

	static constexpr Array<CirclePath, 8> circlePaths = {
		CirclePath{0.0f, -3.0f, 480.0f},
		CirclePath{3.0f, -3.0f, 480.0f},
		CirclePath{3.0f, 0.0f, 480.0f},
		CirclePath{3.0f, 3.0f, 480.0f},
		CirclePath{0.0f, 3.0f, 480.0f},
		CirclePath{-3.0f, 3.0f, 480.0f},
		CirclePath{-3.0f, 0.0f, 480.0f},
		CirclePath{-3.0f, -3.0f, 480.0f}};

	float GetRadius() const override = 0;
	int GetFrames() const override = 0;
	virtual int GetTurnSpeed() const = 0;
	virtual float GetTopSpeed() const = 0;
	virtual float GetAttackSpeed() const = 0;
	virtual float GetGroundSpeed() const = 0;
	virtual int GetReloadTime() const = 0;
	virtual bool CanLandOnHelipad() const = 0;

	virtual bool CanTargetAir() const
	{
		return false;
	}

	virtual bool CanTargetLand() const
	{
		return true;
	}

	virtual int GetAmmoCapacity() const
	{
		return 2000;
	}

	virtual bool TurnsOnTheSpot() const
	{
		return false;
	}

	// Extra waypoints past the airport's handover point this aircraft needs
	// before the next one may leave its hanger. Zero hands the taxiway on as
	// soon as it is clear of the merge, which keeps the queue tight; a larger
	// aircraft takes another waypoint to be out of the way.
	virtual int GetTaxiReleaseOffset() const
	{
		return 0;
	}

	virtual int GetTakeOffLandingOffset() const
	{
		return 8;
	}
};

class ApacheState : public AircraftState
{
public:
	static constexpr float radius = 15.0f;
	static constexpr int frames = 32;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 120;
	static constexpr float topSpeed = 720.0f;
	static constexpr float attackSpeed = 1620.0f;
	static constexpr float groundSpeed = 120.0f;
	static constexpr int sight = 10;
	static constexpr int reloadTime = 6;

	ApacheState()
	{
		SetDirections(directions);
		SetAircraft(true);

		animationLimit = 2;

		AddGroup("army");
		AddGroup("aircrafts");
		AddGroup("air");

		combat.enabled = true;
		combat.weapon = "bullet";
		combat.attackable = true;

		hitPoints = 100.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetTopSpeed() const override
	{
		return topSpeed;
	}
	float GetAttackSpeed() const override
	{
		return attackSpeed;
	}
	float GetGroundSpeed() const override
	{
		return groundSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	bool CanLandOnHelipad() const override
	{
		return true;
	}
	bool TurnsOnTheSpot() const override
	{
		return true;
	}
	int GetAmmoCapacity() const override
	{
		return 20000;
	}
};

class JetState : public AircraftState
{
public:
	static constexpr float radius = 15.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 24;
	static constexpr float topSpeed = 1440.0f;
	static constexpr float attackSpeed = 2160.0f;
	static constexpr float groundSpeed = 360.0f;
	static constexpr int sight = 12;
	static constexpr int reloadTime = 240;

	JetState()
	{
		SetDirections(directions);
		SetAircraft(true);

		AddGroup("army");
		AddGroup("aircrafts");
		AddGroup("air");

		combat.enabled = true;
		combat.weapon = "rocket";
		combat.attackable = true;

		hitPoints = 100.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetTopSpeed() const override
	{
		return topSpeed;
	}
	float GetAttackSpeed() const override
	{
		return attackSpeed;
	}
	float GetGroundSpeed() const override
	{
		return groundSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	bool CanLandOnHelipad() const override
	{
		return false;
	}
};

class BomberState : public AircraftState
{
public:
	static constexpr float radius = 15.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 12;
	static constexpr float topSpeed = 720.0f;
	static constexpr float attackSpeed = 1080.0f;
	static constexpr float groundSpeed = 360.0f;
	static constexpr int sight = 12;
	static constexpr int reloadTime = 360;

	BomberState()
	{
		SetDirections(directions);
		SetAircraft(true);

		AddGroup("army");
		AddGroup("aircrafts");
		AddGroup("air");

		combat.enabled = true;
		combat.weapon = "bomb";
		combat.attackable = true;

		hitPoints = 100.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetTopSpeed() const override
	{
		return topSpeed;
	}
	float GetAttackSpeed() const override
	{
		return attackSpeed;
	}
	float GetGroundSpeed() const override
	{
		return groundSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	bool CanLandOnHelipad() const override
	{
		return false;
	}
	int GetTaxiReleaseOffset() const override
	{
		return 1;
	}
};

class TransportState : public AircraftState
{
public:
	static constexpr float radius = 15.0f;
	static constexpr int frames = 16;
	static constexpr int directions = 8;
	static constexpr int turnSpeed = 12;
	static constexpr float topSpeed = 720.0f;
	static constexpr float attackSpeed = 720.0f;
	static constexpr float groundSpeed = 360.0f;
	static constexpr int sight = 12;
	static constexpr int reloadTime = 300;

	TransportState()
	{
		SetDirections(directions);
		SetAircraft(true);

		AddGroup("army");
		AddGroup("aircrafts");
		AddGroup("air");

		combat.attackable = true;
		logistics.canLoad = true;

		hitPoints = 100.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetTurnSpeed() const override
	{
		return turnSpeed;
	}
	float GetTopSpeed() const override
	{
		return topSpeed;
	}
	float GetAttackSpeed() const override
	{
		return attackSpeed;
	}
	float GetGroundSpeed() const override
	{
		return groundSpeed;
	}
	int GetSight() const override
	{
		return sight;
	}
	int GetReloadTime() const override
	{
		return reloadTime;
	}
	bool CanLandOnHelipad() const override
	{
		return false;
	}
};
} // namespace TGX
