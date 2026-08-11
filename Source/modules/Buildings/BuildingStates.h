#pragma once

#include <utility>
#include "Core.h"
#include "ItemInstance.h"

namespace TGX
{
class BuildingState : public ItemInstance
{
public:
	Vector<Vector<int>> passableGrid;
	bool canBePrimary = false;
	int deployIndex = 0;
	int baseWidth = 0;
	int baseHeight = 0;
	int pixelOffsetX = 0;
	int pixelOffsetY = 0;

	static inline int cellCollisionMode = 1000;

	int GetCellCollisionMode() const override
	{
		return cellCollisionMode;
	}

	float GetCenterX() const override
	{
		if (passableGrid.empty() || passableGrid[0].empty()) { return GetX(); }

		// actual center of building
		return GetX() + (static_cast<float>(passableGrid[0].size()) / 2.0f);
	}

	float GetCenterY() const override
	{
		if (passableGrid.empty()) { return GetY(); }

		// actual center of building
		return GetY() + (static_cast<float>(passableGrid.size()) / 2.0f);
	}

	int GetSight() const override
	{
		if (passableGrid.empty() || passableGrid[0].empty()) { return 4; }

		// size of building
		int w = static_cast<int>(passableGrid[0].size());
		int h = static_cast<int>(passableGrid.size());

		return static_cast<int>(std::ceil(std::sqrt(((w / 2.0) * (w / 2.0)) + ((h / 2.0) * (h / 2.0)))));
	}

	float GetRadius() const override = 0;
	int GetFrames() const override = 0;

	virtual const Vector<Tuple<float, float, int>> &GetDeployPositions() const
	{
		static const Vector<Tuple<float, float, int>> empty;
		return empty;
	}

	virtual int GetDeployDirection() const
	{
		return 0;
	}

	void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}

	void RemoveFromGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const override
	{
	}

	virtual int GetPowerUsage() const = 0;
};

// -------------------------------------------------------------------------
// Construction Facility
// -------------------------------------------------------------------------
class ConstructionFacilityState : public BuildingState
{
public:
	static constexpr float radius = 22.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 0;

	ConstructionFacilityState()
	{
		passableGrid = Vector<Vector<int>>(23, Vector<int>(23, 1));
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// Powerplant
// -------------------------------------------------------------------------
class PowerplantState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = -400;

	PowerplantState()
	{
		passableGrid = Vector<Vector<int>>(12, Vector<int>(16, 1));
		baseWidth = 69;
		baseHeight = 69;
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// Science Post
// -------------------------------------------------------------------------
class SciencePostState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 0;

	SciencePostState()
	{
		passableGrid = Vector<Vector<int>>(12, Vector<int>(17, 1));
		baseWidth = 69;
		baseHeight = 69;
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

struct AirWayPoint
{
	float x = 0.0f;
	float y = 0.0f;
	int direction = 0;
	float speed = 0.0f;
};

struct HangerPosition
{
	float x = 0.0f;
	float y = 0.0f;
	int direction = 0;
};

class AirportState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int maximumNumberOfPlanes = 4;
	static constexpr int powerUsage = 250;
	static constexpr int hangersPerRunway = 4;

	int numberOfPlanes = 0;
	int helipadUid = INT_MIN;

	const Vector<HangerPosition> hangerPositions =
		{
			{-9.37f, 13.5f, 0},
			{-7.5f, 13.5f, 0},
			{-5.5f, 13.5f, 0},
			{-3.25f, 13.5f, 0},
			{4.0f, 10.0f, 4},
			{6.0f, 10.0f, 4},
			{8.0f, 10.0f, 4},
			{10.0f, 10.0f, 4}};

	Vector<Tuple<float, float, int>> deployPositions =
		{
			std::make_tuple(-9.37f, 13.5f, INT_MIN),
			std::make_tuple(-7.5f, 13.5f, INT_MIN),
			std::make_tuple(-5.5f, 13.5f, INT_MIN),
			std::make_tuple(-3.25f, 13.5f, INT_MIN),
			std::make_tuple(4.0f, 10.0f, INT_MIN),
			std::make_tuple(6.0f, 10.0f, INT_MIN),
			std::make_tuple(8.0f, 10.0f, INT_MIN),
			std::make_tuple(10.0f, 10.0f, INT_MIN)};

	const Vector<Vector<AirWayPoint>> takeOffPaths =
		{
			{{-9.37f, 7.5f, 6, 2.0f},
			 {-10.5f, 7.5f, 5, 2.0f},
			 {-12.5f, 9.7f, 4, 2.0f},
			 {-12.5f, 11.75f, 2, 2.0f},
			 {-12.0f, 11.75f, 2, 8.0f},
			 {-6.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 24.0f},
			 {6.0f, 11.75f, 2, 32.0f},
			 {12.0f, 11.75f, 2, 48.0f}},
			{{-7.5f, 7.5f, 6, 2.0f},
			 {-10.5f, 7.5f, 5, 2.0f},
			 {-12.5f, 9.7f, 4, 2.0f},
			 {-12.5f, 11.75f, 2, 2.0f},
			 {-12.0f, 11.75f, 2, 8.0f},
			 {-6.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 24.0f},
			 {6.0f, 11.75f, 2, 32.0f},
			 {12.0f, 11.75f, 2, 48.0f}},
			{{-5.5f, 7.5f, 6, 2.0f},
			 {-10.5f, 7.5f, 5, 2.0f},
			 {-12.5f, 9.7f, 4, 2.0f},
			 {-12.5f, 11.75f, 2, 2.0f},
			 {-12.0f, 11.75f, 2, 8.0f},
			 {-6.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 24.0f},
			 {6.0f, 11.75f, 2, 32.0f},
			 {12.0f, 11.75f, 2, 48.0f}},
			{{-3.25f, 7.5f, 6, 2.0f},
			 {-10.5f, 7.5f, 5, 2.0f},
			 {-12.5f, 9.7f, 4, 2.0f},
			 {-12.5f, 11.75f, 2, 2.0f},
			 {-12.0f, 11.75f, 2, 8.0f},
			 {-6.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 24.0f},
			 {6.0f, 11.75f, 2, 32.0f},
			 {12.0f, 11.75f, 2, 48.0f}},
			{{4.0f, 8.0f, 2, 2.0f},
			 {11.0f, 8.0f, 1, 2.0f},
			 {12.0f, 6.5f, 0, 2.0f},
			 {12.0f, 3.75f, 6, 2.0f},
			 {11.5f, 3.75f, 6, 8.0f},
			 {5.5f, 3.75f, 6, 18.0f},
			 {0.5f, 3.75f, 6, 24.0f},
			 {-6.5f, 3.75f, 6, 32.0f},
			 {-12.5f, 3.75f, 6, 48.0f}},
			{{6.0f, 8.0f, 2, 2.0f},
			 {11.0f, 8.0f, 1, 2.0f},
			 {12.0f, 6.5f, 0, 2.0f},
			 {12.0f, 3.75f, 6, 2.0f},
			 {11.5f, 3.75f, 6, 8.0f},
			 {5.5f, 3.75f, 6, 18.0f},
			 {0.5f, 3.75f, 6, 24.0f},
			 {-6.5f, 3.75f, 6, 32.0f},
			 {-12.5f, 3.75f, 6, 48.0f}},
			{{8.0f, 8.0f, 2, 2.0f},
			 {11.0f, 8.0f, 1, 2.0f},
			 {12.0f, 6.5f, 0, 2.0f},
			 {12.0f, 3.75f, 6, 2.0f},
			 {11.5f, 3.75f, 6, 8.0f},
			 {5.5f, 3.75f, 6, 18.0f},
			 {0.5f, 3.75f, 6, 24.0f},
			 {-6.5f, 3.75f, 6, 32.0f},
			 {-12.5f, 3.75f, 6, 48.0f}},
			{{10.0f, 8.0f, 2, 2.0f},
			 {11.0f, 8.0f, 1, 2.0f},
			 {12.0f, 6.5f, 0, 2.0f},
			 {12.0f, 3.75f, 6, 2.0f},
			 {11.5f, 3.75f, 6, 8.0f},
			 {5.5f, 3.75f, 6, 18.0f},
			 {0.5f, 3.75f, 6, 24.0f},
			 {-6.5f, 3.75f, 6, 32.0f},
			 {-12.5f, 3.75f, 6, 48.0f}}};

	const Vector<Vector<AirWayPoint>> landingPaths =
		{
			{{-5.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 16.0f},
			 {3.0f, 11.75f, 2, 12.0f},
			 {7.0f, 11.75f, 2, 8.0f},
			 {10.0f, 11.75f, 2, 4.0f},
			 {11.0f, 11.75f, 2, 3.0f},
			 {12.25f, 11.75f, 2, 2.0f},
			 {12.25f, 9.6f, 0, 2.0f},
			 {-0.5f, 9.6f, 6, 2.0f},
			 {-2.0f, 7.5f, 7, 2.0f},
			 {-9.37f, 7.5f, 6, 2.0f},
			 {-9.37f, 9.5f, 4, 2.0f}},
			{{-5.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 16.0f},
			 {3.0f, 11.75f, 2, 12.0f},
			 {7.0f, 11.75f, 2, 8.0f},
			 {10.0f, 11.75f, 2, 4.0f},
			 {11.0f, 11.75f, 2, 3.0f},
			 {12.25f, 11.75f, 2, 2.0f},
			 {12.25f, 9.6f, 0, 2.0f},
			 {-0.5f, 9.6f, 6, 2.0f},
			 {-2.0f, 7.5f, 7, 2.0f},
			 {-7.5f, 7.5f, 6, 2.0f},
			 {-7.5f, 9.5f, 4, 2.0f}},
			{{-5.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 16.0f},
			 {3.0f, 11.75f, 2, 12.0f},
			 {7.0f, 11.75f, 2, 8.0f},
			 {10.0f, 11.75f, 2, 4.0f},
			 {11.0f, 11.75f, 2, 3.0f},
			 {12.25f, 11.75f, 2, 2.0f},
			 {12.25f, 9.6f, 0, 2.0f},
			 {-0.5f, 9.6f, 6, 2.0f},
			 {-2.0f, 7.5f, 7, 2.0f},
			 {-5.5f, 7.5f, 6, 2.0f},
			 {-5.5f, 9.5f, 4, 2.0f}},
			{{-5.0f, 11.75f, 2, 18.0f},
			 {0.0f, 11.75f, 2, 16.0f},
			 {3.0f, 11.75f, 2, 12.0f},
			 {7.0f, 11.75f, 2, 8.0f},
			 {10.0f, 11.75f, 2, 4.0f},
			 {11.0f, 11.75f, 2, 3.0f},
			 {12.25f, 11.75f, 2, 2.0f},
			 {12.25f, 9.6f, 0, 2.0f},
			 {-0.5f, 9.6f, 6, 2.0f},
			 {-2.0f, 7.5f, 7, 2.0f},
			 {-3.25f, 7.5f, 6, 2.0f},
			 {-3.25f, 9.5f, 4, 2.0f}},
			{{5.0f, 3.75f, 6, 18.0f},
			 {0.0f, 3.75f, 6, 16.0f},
			 {-3.0f, 3.75f, 6, 12.0f},
			 {-7.0f, 3.75f, 6, 8.0f},
			 {-10.0f, 3.75f, 6, 4.0f},
			 {-11.0f, 3.75f, 6, 3.0f},
			 {-12.25f, 3.75f, 6, 2.0f},
			 {-12.25f, 5.75f, 4, 2.0f},
			 {1.5f, 5.75f, 2, 2.0f},
			 {2.5f, 8.5f, 3, 2.0f},
			 {4.0f, 8.5f, 2, 2.0f},
			 {4.0f, 5.75f, 0, 2.0f}},
			{{5.0f, 3.75f, 6, 18.0f},
			 {0.0f, 3.75f, 6, 16.0f},
			 {-3.0f, 3.75f, 6, 12.0f},
			 {-7.0f, 3.75f, 6, 8.0f},
			 {-10.0f, 3.75f, 6, 4.0f},
			 {-11.0f, 3.75f, 6, 3.0f},
			 {-12.25f, 3.75f, 6, 2.0f},
			 {-12.25f, 5.75f, 4, 2.0f},
			 {1.5f, 5.75f, 2, 2.0f},
			 {2.5f, 8.5f, 3, 2.0f},
			 {6.0f, 8.5f, 2, 2.0f},
			 {6.0f, 5.75f, 0, 2.0f}},
			{{5.0f, 3.75f, 6, 18.0f},
			 {0.0f, 3.75f, 6, 16.0f},
			 {-3.0f, 3.75f, 6, 12.0f},
			 {-7.0f, 3.75f, 6, 8.0f},
			 {-10.0f, 3.75f, 6, 4.0f},
			 {-11.0f, 3.75f, 6, 3.0f},
			 {-12.25f, 3.75f, 6, 2.0f},
			 {-12.25f, 5.75f, 4, 2.0f},
			 {1.5f, 5.75f, 2, 2.0f},
			 {2.5f, 8.5f, 3, 2.0f},
			 {8.0f, 8.5f, 2, 2.0f},
			 {8.0f, 5.75f, 0, 2.0f}},
			{{5.0f, 3.75f, 6, 18.0f},
			 {0.0f, 3.75f, 6, 16.0f},
			 {-3.0f, 3.75f, 6, 12.0f},
			 {-7.0f, 3.75f, 6, 8.0f},
			 {-10.0f, 3.75f, 6, 4.0f},
			 {-11.0f, 3.75f, 6, 3.0f},
			 {-12.25f, 3.75f, 6, 2.0f},
			 {-12.25f, 5.75f, 4, 2.0f},
			 {1.5f, 5.75f, 2, 2.0f},
			 {2.5f, 8.5f, 3, 2.0f},
			 {10.0f, 8.5f, 2, 2.0f},
			 {10.0f, 5.75f, 0, 2.0f}}};

	const Vector<AirWayPoint> approachPositions =
		{
			{-18.5f, 11.75f, 2, 18.0f},
			{18.5f, 3.75f, 6, 18.0f}};

	const Vector<AirWayPoint> helipadLandingPaths =
		{
			{9.85f, 0.5f, 0, 4.0f}};

	const AirWayPoint helipadApproachPosition = {9.85f, 0.5f, 6, 4.0f};
	const AirWayPoint helipadDeployPosition = {9.85f, 4.6f, 0, 0.0f};

	AirportState()
	{
		passableGrid = Vector<Vector<int>>(8, Vector<int>(16, 1));
		baseWidth = 300;
		baseHeight = 160;
		canBePrimary = true;

		hitPoints = 500.0f;
	}

	const Vector<Tuple<float, float, int>> &GetDeployPositions() const override
	{
		return deployPositions;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

class BarracksState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 30;

	Vector<Tuple<float, float, int>> deployPositions =
		{
			{5.0f, 11.0f, INT_MIN},
			{4.0f, 11.0f, INT_MIN},
			{3.0f, 11.0f, INT_MIN},
			{2.0f, 11.0f, INT_MIN},
			{1.0f, 11.0f, INT_MIN},
			{0.0f, 11.0f, INT_MIN},
			{5.0f, 12.0f, INT_MIN},
			{4.0f, 12.0f, INT_MIN},
			{3.0f, 12.0f, INT_MIN},
			{2.0f, 12.0f, INT_MIN},
			{1.0f, 12.0f, INT_MIN},
			{0.0f, 12.0f, INT_MIN},
			{5.0f, 13.0f, INT_MIN},
			{4.0f, 13.0f, INT_MIN},
			{3.0f, 13.0f, INT_MIN},
			{2.0f, 13.0f, INT_MIN},
			{1.0f, 13.0f, INT_MIN},
			{0.0f, 13.0f, INT_MIN},
		};

	BarracksState()
	{
		passableGrid = Vector<Vector<int>>(10, Vector<int>(14, 1));
		baseWidth = 95;
		baseHeight = 78;
		canBePrimary = true;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}

	int GetFrames() const override
	{
		return frames;
	}

	const Vector<Tuple<float, float, int>> &GetDeployPositions() const override
	{
		return deployPositions;
	}

	int GetDeployDirection() const override
	{
		return 0;
	}

	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// VehicleAssemblyTunnel
// -------------------------------------------------------------------------
class VehicleAssemblyTunnelState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 250;

	Vector<Tuple<float, float, int>> deployPositions =
		{
			{1.0f, 11.0f, INT_MIN},
			{3.5f, 11.0f, INT_MIN},
			{6.0f, 11.0f, INT_MIN},
			{8.5f, 11.0f, INT_MIN},
			{11.0f, 11.0f, INT_MIN},
			{13.5f, 11.0f, INT_MIN},
			{1.0f, 12.0f, INT_MIN},
			{3.5f, 12.0f, INT_MIN},
			{6.0f, 12.0f, INT_MIN},
			{8.5f, 12.0f, INT_MIN},
			{11.0f, 12.0f, INT_MIN},
			{13.5f, 12.0f, INT_MIN},
			{1.0f, 13.0f, INT_MIN},
			{3.5f, 13.0f, INT_MIN},
			{6.0f, 13.0f, INT_MIN},
			{8.5f, 13.0f, INT_MIN},
			{11.0f, 13.0f, INT_MIN},
			{13.5f, 13.0f, INT_MIN},
		};

	VehicleAssemblyTunnelState()
	{
		passableGrid = Vector<Vector<int>>(10, Vector<int>(18, 1));
		baseWidth = 144;
		baseHeight = 97;
		canBePrimary = true;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}

	int GetFrames() const override
	{
		return frames;
	}

	const Vector<Tuple<float, float, int>> &GetDeployPositions() const override
	{
		return deployPositions;
	}

	int GetDeployDirection() const override
	{
		return 11;
	}

	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
class ShipyardState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 250;

	Vector<Tuple<float, float, int>> deployPositions =
		{
			{10.0f, 25.0f, INT_MIN},
		};

	ShipyardState()
	{
		passableGrid = Vector<Vector<int>>(24, Vector<int>(25, 1));
		baseWidth = 200;
		baseHeight = 190;
		canBePrimary = true;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}

	int GetFrames() const override
	{
		return frames;
	}

	const Vector<Tuple<float, float, int>> &GetDeployPositions() const override
	{
		return deployPositions;
	}

	int GetDeployDirection() const override
	{
		return 6;
	}

	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// Radar
// -------------------------------------------------------------------------
class RadarState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 250;

	RadarState()
	{
		passableGrid = Vector<Vector<int>>(5, Vector<int>(5, 1));
		baseWidth = 85;
		baseHeight = 100;
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// Oil Extractor
// -------------------------------------------------------------------------
class OilExtractor : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 0;

	OilExtractor()
	{
		passableGrid = Vector<Vector<int>>(2, Vector<int>(4, 1));
		baseWidth = 85;
		baseHeight = 100;
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};

// -------------------------------------------------------------------------
// Water Extractor
// -------------------------------------------------------------------------
class WaterExtractor : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int powerUsage = 0;

	WaterExtractor()
	{
		passableGrid = Vector<Vector<int>>(2, Vector<int>(4, 1));
		baseWidth = 85;
		baseHeight = 100;
		canBePrimary = false;

		hitPoints = 30.0f;
	}

	float GetRadius() const override
	{
		return radius;
	}
	int GetFrames() const override
	{
		return frames;
	}
	int GetPowerUsage() const override
	{
		return powerUsage;
	}
};
} // namespace TGX
