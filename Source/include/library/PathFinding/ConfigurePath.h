#pragma once

#include <iostream>
#include "Heuristic/Heuristic.h"

namespace TGX
{
enum class PathDirections : std::uint8_t
{
	Four,
	Eight
};

enum class CellCollisionMode : uint16_t
{
	Off = 0,
	Soft = 1,
	Weak = 10,
	Medium = 100,
	Hard = 1000,
	Full = 10000,
};

class ConfigurePath
{
public:
	PathDirections pathDirections = PathDirections::Eight;
	int codeMode = 0;

	ConfigurePath() = default;
	virtual ~ConfigurePath() = default;
	explicit ConfigurePath(PathDirections directions) : pathDirections(directions)
	{
	}

	virtual bool IsTraversable(int cellValue) const
	{
		return cellValue != 1;
	}

	virtual HeuristicType GetHeuristicType() const
	{
		return HeuristicType::Euclidean;
	}

	virtual int GetCellWeight(int cellValue) const
	{
		return cellValue;
	}
};

class AStarConfigurePath : public ConfigurePath
{
public:
	HeuristicType heuristicType = HeuristicType::Euclidean;
	CellCollisionMode collisionMode = CellCollisionMode::Off;
	Function<bool(int, int)> traversalLogic;

	AStarConfigurePath() : ConfigurePath(PathDirections::Eight)
	{
		traversalLogic = [](int /*mode*/, int val) {
			return val != 1;
		};
	}

	HeuristicType GetHeuristicType() const override
	{
		return heuristicType;
	}

	bool IsTraversable(int cellValue) const override
	{
		return traversalLogic(static_cast<int>(collisionMode), cellValue);
	}
};

class NavalRoutesConfigurePath : public ConfigurePath
{
public:
	HeuristicType heuristicType = HeuristicType::Euclidean;

	float turnPenalty = 2.0f;

	NavalRoutesConfigurePath() : ConfigurePath(PathDirections::Eight) {}

	HeuristicType GetHeuristicType() const override
	{
		return heuristicType;
	}

	bool IsTraversable(int cellValue) const override
	{
		if (codeMode == static_cast<int>(CellCollisionMode::Soft))
		{
			return cellValue <= static_cast<int>(CellCollisionMode::Soft);
		}

		return cellValue == static_cast<int>(CellCollisionMode::Off);
	}
};

class WaveConfigurePath : public ConfigurePath
{
public:
	WaveConfigurePath() : ConfigurePath(PathDirections::Four) {}

	bool IsTraversable(int cellValue) const override
	{
		return cellValue == 0;
	}
};
} // namespace TGX
