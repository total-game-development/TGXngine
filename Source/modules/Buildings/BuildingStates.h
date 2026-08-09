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

class AirportState : public BuildingState
{
public:
	static constexpr float radius = 9.0f;
	static constexpr int frames = 1;
	static constexpr int maximumNumberOfPlanes = 4;
	static constexpr int powerUsage = 250;

	const Pair<float, float> approachPosition = std::make_pair(22.5f, 3.6f);
	int numberOfPlanes = 0;

	Vector<Tuple<float, float, int>> deployPositions =
		{
			std::make_tuple(-2.35f, -6.65f, INT_MIN),
			std::make_tuple(-1.5f, -6.65f, INT_MIN),
			std::make_tuple(0.5f, -6.65f, INT_MIN),
			std::make_tuple(1.25f, -6.65f, INT_MIN)};

	Vector<Vector<Tuple<int, int, int>>> takeOffPaths =
		{
			{std::make_tuple(65, 2, 1),
			 std::make_tuple(520, 3, 1),
			 std::make_tuple(740, 4, 1),
			 std::make_tuple(1050, 6, 4),
			 std::make_tuple(1075, 6, 9),
			 std::make_tuple(1150, 6, 11),
			 std::make_tuple(1200, 6, 12)},
			{std::make_tuple(65, 2, 1),
			 std::make_tuple(470, 3, 1),
			 std::make_tuple(685, 4, 1),
			 std::make_tuple(995, 6, 4),
			 std::make_tuple(1040, 6, 9),
			 std::make_tuple(1095, 6, 11),
			 std::make_tuple(1145, 6, 12)},
			{std::make_tuple(65, 2, 1),
			 std::make_tuple(340, 3, 1),
			 std::make_tuple(555, 4, 1),
			 std::make_tuple(865, 6, 4),
			 std::make_tuple(900, 6, 9),
			 std::make_tuple(965, 6, 11),
			 std::make_tuple(1015, 6, 12)},
			{std::make_tuple(65, 2, 1),
			 std::make_tuple(290, 3, 1),
			 std::make_tuple(505, 4, 1),
			 std::make_tuple(815, 6, 4),
			 std::make_tuple(850, 6, 9),
			 std::make_tuple(915, 6, 11),
			 std::make_tuple(965, 6, 12)}};

	Vector<Vector<Tuple<int, int, int>>> landingPaths =
		{
			{std::make_tuple(0, 6, 12),
			 std::make_tuple(40, 6, 10),
			 std::make_tuple(60, 6, 8),
			 std::make_tuple(80, 6, 6),
			 std::make_tuple(180, 6, 4),
			 std::make_tuple(210, 6, 3),
			 std::make_tuple(225, 6, 2),
			 std::make_tuple(375, 0, 1),
			 std::make_tuple(570, 1, 1),
			 std::make_tuple(935, 2, 1),
			 std::make_tuple(985, 4, 1),
			 std::make_tuple(1045, 4, 1)},
			{std::make_tuple(0, 6, 12),
			 std::make_tuple(40, 6, 10),
			 std::make_tuple(60, 6, 8),
			 std::make_tuple(80, 6, 6),
			 std::make_tuple(180, 6, 4),
			 std::make_tuple(210, 6, 3),
			 std::make_tuple(225, 6, 2),
			 std::make_tuple(375, 0, 1),
			 std::make_tuple(570, 1, 1),
			 std::make_tuple(935, 2, 1),
			 std::make_tuple(1050, 4, 1),
			 std::make_tuple(1110, 4, 1)},
			{std::make_tuple(0, 6, 12),
			 std::make_tuple(40, 6, 10),
			 std::make_tuple(60, 6, 8),
			 std::make_tuple(80, 6, 6),
			 std::make_tuple(180, 6, 4),
			 std::make_tuple(210, 6, 3),
			 std::make_tuple(225, 6, 2),
			 std::make_tuple(375, 0, 1),
			 std::make_tuple(570, 1, 1),
			 std::make_tuple(935, 2, 1),
			 std::make_tuple(1175, 4, 1),
			 std::make_tuple(1235, 4, 1)},
			{std::make_tuple(0, 6, 12),
			 std::make_tuple(40, 6, 10),
			 std::make_tuple(60, 6, 8),
			 std::make_tuple(80, 6, 6),
			 std::make_tuple(180, 6, 4),
			 std::make_tuple(210, 6, 3),
			 std::make_tuple(225, 6, 2),
			 std::make_tuple(375, 0, 1),
			 std::make_tuple(570, 1, 1),
			 std::make_tuple(935, 2, 1),
			 std::make_tuple(1240, 4, 1),
			 std::make_tuple(1300, 4, 1)}};

	AirportState()
	{
		passableGrid = Vector<Vector<int>>(8, Vector<int>(16, 1));
		baseWidth = 300;
		baseHeight = 160;
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
