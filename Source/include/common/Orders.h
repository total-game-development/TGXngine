#pragma once

#include "Logs.h"

namespace TGX
{
class Orders
{
public:
	enum class Order : std::uint8_t
	{
		None,
		Action,
		Idle,
		Stand,
		Standing,
		Turning,
		Move,
		Moving,
		MoveTo,
		Approach,
		Search,
		SearchAndDestroy,
		Attack,
		Attacking,
		Attacked,
		TurnToFire,
		Fire,
		Firing,
		Destroyed,
		Float,
		TakingOff,
		Landing,
		Circle,
		DetourTo,
		Load,
		Loading,
		Unload,
		Unloading,
		Extract,
		TakeOff,
		Fly,
		Approaching,
		Land,
		BreakAway,
		BreakingAway
	};

	int id = 0;
	int uid = -1;
	int target_uid = -1;
	Order order;
	float toX = -1;
	float toY = -1;

	Orders() : order(Order::Stand)
	{
	}

	Orders(Order order) : order(order)
	{
	}

	Orders(Order order, float x, float y, int target_uid) : target_uid(target_uid), order(order), toX(x), toY(y)
	{
	}

	static std::string ToString(Order o)
	{
		switch (o)
		{
			case Order::None:
				return "none";
			case Order::Idle:
				return "idle";
			case Order::Stand:
				return "stand";
			case Order::Standing:
				return "standing";
			case Order::Turning:
				return "turning";
			case Order::Move:
				return "move";
			case Order::Moving:
				return "moving";
			case Order::MoveTo:
				return "moveTo";
			case Order::Approach:
				return "approach";
			case Order::Search:
				return "search";
			case Order::SearchAndDestroy:
				return "search-and-destroy";
			case Order::Attack:
				return "attack";
			case Order::Attacking:
				return "attacking";
			case Order::Attacked:
				return "attacked";
			case Order::TurnToFire:
				return "turnToFire";
			case Order::Fire:
				return "fire";
			case Order::Firing:
				return "firing";
			case Order::Destroyed:
				return "destroyed";
			case Order::Float:
				return "float";
			case Order::TakingOff:
				return "takingOff";
			case Order::Landing:
				return "landing";
			case Order::Circle:
				return "circle";
			case Order::DetourTo:
				return "detour-to";
			case Order::Load:
				return "load";
			case Order::Loading:
				return "loading";
			case Order::Unloading:
				return "unloading";
			case Order::Action:
				return "action";
			case Order::TakeOff:
				return "take-off";
			case Order::Fly:
				return "fly";
			case Order::Approaching:
				return "approaching";
			case Order::Land:
				return "land";
			case Order::BreakAway:
				return "break-away";
			case Order::BreakingAway:
				return "breaking-away";
			default:
				return "Unknown";
		}
	}

	static Order FromString(const std::string &str)
	{
		static const std::unordered_map<std::string, Order> map = {
			{"none", Order::None},
			{"idle", Order::Idle},
			{"stand", Order::Stand},
			{"standing", Order::Standing},
			{"turning", Order::Turning},
			{"move", Order::Move},
			{"moving", Order::Moving},
			{"moveTo", Order::MoveTo},
			{"approach", Order::Approach},
			{"search", Order::Search},
			{"search-and-destroy", Order::SearchAndDestroy},
			{"attack", Order::Attack},
			{"attacking", Order::Attacking},
			{"attacked", Order::Attacked},
			{"turnToFire", Order::TurnToFire},
			{"fire", Order::Fire},
			{"firing", Order::Firing},
			{"destroyed", Order::Destroyed},
			{"float", Order::Float},
			{"takingOff", Order::TakingOff},
			{"landing", Order::Landing},
			{"circle", Order::Circle},
			{"detour-to", Order::DetourTo},
			{"load", Order::Load},
			{"loading", Order::Loading},
			{"unloading", Order::Unloading},
			{"action", Order::Action},
			{"take-off", Order::TakeOff},
			{"fly", Order::Fly},
			{"approaching", Order::Approaching},
			{"land", Order::Land},
			{"break-away", Order::BreakAway},
			{"breaking-away", Order::BreakingAway},
		};

		auto it = map.find(str);
		if (it != map.end())
		{
			return it->second;
		}

		Log::Error("Invalid String for Order");
		return Order::None;
	}
};
} // namespace TGX

namespace std
{
template <>
struct hash<TGX::Orders::Order>
{
	std::size_t operator()(TGX::Orders::Order o) const noexcept
	{
		return std::hash<int>()(static_cast<int>(o));
	}
};
}; // namespace std
