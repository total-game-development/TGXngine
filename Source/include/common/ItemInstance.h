#pragma once

#include "GridTracker.h"
#include "Instance.h"
#include "ItemStates.h"
#include "Orders.h"

namespace TGX
{
class ItemInstance : public Instance
{
public:
	enum class ItemProperty : std::uint8_t
	{
		LoadThreshold,
		LoadCapacity,
		LoadedUIDs
	};

	using PropertyValue = std::variant<int, float, bool, std::string, Vector<int>>;

	struct CombatCapabilities
	{
		bool enabled = false;
		bool attackable = false;
		String weapon = "none";
	};

	struct LogisticsCapabilities
	{
		bool loadable = false;
		bool canLoad = false;
		bool canExtract = false;
	};

	struct ClassificationCapabilities
	{
		bool army = false;
		bool navy = false;
		bool aircraft = false;
		bool armoured = false;
		bool buildable = false;
	};

private:
	Set<String> groups;
	Unique<Orders> orders;
	int targetUid = 0;
	int remoteUid = 0;
	float lastMovementX = 0;
	float lastMovementY = 0;
	float speed = 0;
	bool selected = false;
	bool training = false;
	bool isHidden = false;
	bool visible = true;
	ItemStates state = ItemStates::Stand;
	Map<ItemProperty, PropertyValue> extras;

protected:
	float life = 0;
	float hitPoints = 0;
	int priority = 0;
	int cellCollisionMode = 0;
	float direction = 0;
	int directions = 0;

	CombatCapabilities combat;
	LogisticsCapabilities logistics;
	ClassificationCapabilities classification;

public:
	Array<float, 8> polygon{};

	ItemInstance() : orders(std::make_unique<Orders>())
	{
	}

	ItemInstance(const ItemInstance &) = delete;
	virtual ~ItemInstance() = default;

	ItemInstance &operator=(const ItemInstance &) = delete;

	const Set<String> &GetGroups() const
	{
		return groups;
	}

	void AddGroup(const String &group)
	{
		this->groups.insert(group);
	}

	Orders *GetOrders() const
	{
		return orders.get();
	}

	void SetOrders(Orders::Order order)
	{
		this->orders->order = order;
	}

	int GetRemoteUid() const
	{
		return remoteUid;
	}

	void SetRemoteUid(int inUid)
	{
		remoteUid = inUid;
	}

	int GetTargetUid() const
	{
		return targetUid;
	}

	void SetTargetUid(int inUid)
	{
		targetUid = inUid;
	}

	float GetLastMovementX() const
	{
		return lastMovementX;
	}

	void SetLastMovementX(float inLastMovementX)
	{
		lastMovementX = inLastMovementX;
	}

	float GetLastMovementY() const
	{
		return lastMovementY;
	}

	void SetLastMovementY(float inLastMovementY)
	{
		lastMovementY = inLastMovementY;
	}

	float GetDirection() const
	{
		return direction;
	}

	void SetDirection(float inDirection)
	{
		direction = inDirection;
	}

	int GetDirections() const
	{
		return directions;
	}

	void SetDirections(int inDirections)
	{
		directions = inDirections;
	}

	float GetSpeed() const
	{
		return speed;
	}

	void SetSpeed(float inSpeed)
	{
		speed = inSpeed;
	}

	float GetLife() const
	{
		return life;
	}

	void SetLife(float inLife)
	{
		life = inLife;
	}

	void Damage(float inDamage)
	{
		life -= inDamage;
	}

	float GetHitPoints() const
	{
		return hitPoints;
	}

	void SetHitPoints(float inHitPoints)
	{
		hitPoints = inHitPoints;
	}

	const String &GetWeapon() const
	{
		return combat.weapon;
	}

	int IsSelected() const
	{
		return selected;
	}

	void SetSelected(int inSelected)
	{
		selected = inSelected;
	}

	bool IsTraining() const
	{
		return training;
	}

	void SetTraining(bool inTraining)
	{
		training = inTraining;
	}

	int GetPriority() const
	{
		return priority;
	}

	ItemStates GetState() const
	{
		return state;
	}

	void SetState(ItemStates inState)
	{
		state = inState;
	}

	bool IsArmy() const
	{
		return classification.army;
	}

	void SetArmy(bool army)
	{
		classification.army = army;
	}

	bool IsNavy() const
	{
		return classification.navy;
	}

	void SetNavy(bool navy)
	{
		classification.navy = navy;
	}

	bool IsAircraft() const
	{
		return classification.aircraft;
	}

	void SetAircraft(bool aircraft)
	{
		classification.aircraft = aircraft;
	}

	bool IsArmoured() const
	{
		return classification.armoured;
	}

	void SetArmoured(bool armoured)
	{
		classification.armoured = armoured;
	}

	bool IsBuildable() const
	{
		return classification.buildable;
	}

	void SetBuildable(bool buildable)
	{
		classification.buildable = buildable;
	}

	bool CanAttack() const
	{
		return combat.enabled;
	}

	bool CanLoad() const
	{
		return logistics.canLoad;
	}

	bool CanExtract() const
	{
		return logistics.canExtract;
	}

	bool IsAttackable() const
	{
		return combat.attackable;
	}

	bool IsLoadable() const
	{
		return logistics.loadable;
	}

	// hidden for everyone, will not update
	bool GetHidden() const
	{
		return isHidden;
	}

	void SetHidden(bool hidden)
	{
		isHidden = hidden;
	}

	// visible by other team, will update
	bool isVisible() const
	{
		return visible;
	}

	void setVisible(bool v)
	{
		visible = v;
	}

	CombatCapabilities &GetCombatCapabilities()
	{
		return combat;
	}

	const CombatCapabilities &GetCombatCapabilities() const
	{
		return combat;
	}

	LogisticsCapabilities &GetLogisticsCapabilities()
	{
		return logistics;
	}

	const LogisticsCapabilities &GetLogisticsCapabilities() const
	{
		return logistics;
	}

	ClassificationCapabilities &GetClassificationCapabilities()
	{
		return classification;
	}

	const ClassificationCapabilities &GetClassificationCapabilities() const
	{
		return classification;
	}

	template <typename T>
	void SetExtra(ItemProperty key, T &&value)
	{
		extras.insert_or_assign(key, std::forward<T>(value));
	}

	template <typename T>
	T *GetExtra(ItemProperty key)
	{
		if (auto it = extras.find(key); it != extras.end())
		{
			return std::get_if<T>(&it->second);
		}
		return nullptr;
	}

	template <typename T>
	const T *GetExtra(ItemProperty key) const
	{
		if (auto it = extras.find(key); it != extras.end())
		{
			return std::get_if<T>(&it->second);
		}
		return nullptr;
	}

	virtual void AddToGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const = 0;
	virtual void RemoveFromGrid(Vector<Vector<int>> &grid, GridTracker &gridTracker) const = 0;

	virtual int GetCellCollisionMode() const = 0;
};
} // namespace TGX
