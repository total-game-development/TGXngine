#include "Projectile.h"

#include <algorithm>
#include "Globals.h"
#include "Lookup.h"
#include "Orders.h"
#include "ProjectileStates.h"
#include "Window.h"
#include "module_interface.h"

namespace TGX
{
constexpr float targetThreshold = 0.1f;

extern "C"
{
	MODULE_API void Init()
	{
		Log::Success("Projectile Init: registering bullet projectile type");
	}

	MODULE_API ProjectileInstance *Awake(const String &name)
	{
		Log::Success("Awake Projectile: " + name);

		WorldState &world = WorldState::GetInstance();

		if (!world.projectiles.contains(name))
		{
			world.projectiles[name] = Vector<Unique<ProjectileInstance>>();
		}

		if (name == "bullet")
		{
			return globalProjectile = new BulletInstance();
		}
		if (name == "grenade")
		{
			return globalProjectile = new GrenadeInstance();
		}
		if (name == "rocket")
		{
			return globalProjectile = new RocketInstance();
		}
		if (name == "missile")
		{
			return globalProjectile = new MissileInstance();
		}
		if (name == "shell")
		{
			return globalProjectile = new ShellInstance();
		}
		if (name == "bomb")
		{
			return globalProjectile = new BombInstance();
		}
		if (name == "laser")
		{
			return globalProjectile = new LaserInstance();
		}

		return nullptr;
	}

	MODULE_API void Create(
		Vector<sf::Sprite *> *spritesRef,
		Vector<sf::Texture *> *texturesRef,
		const String & /*name*/)
	{
		Log::Crash("Creating projectile textures and sprites for: " + globalProjectile->GetName());

		String projectile = "/bullets/" + globalProjectile->GetName();

		for (int i = 0; i < globalProjectile->GetFrames(); i++)
		{
			sf::Image image;
			if (!(image.loadFromFile("Resources/images/" + projectile + "/" + std::to_string(i) + ".png")))
			{
				Log::Error("Cannot load image: Resources/images/" + projectile + "/" + std::to_string(i) + ".png");
				continue;
			}

			auto *texture = new sf::Texture();
			auto *sprite = new sf::Sprite();

			texture->loadFromImage(image);
			sprite->setTexture(*texture);

			texturesRef->emplace_back(texture);
			spritesRef->emplace_back(sprite);
		}

		Log::Success("Projectile asset creation complete for: " + globalProjectile->GetName());
	}

	MODULE_API void Update(ProjectileInstance *projectileType, Vector<sf::Sprite *> * /*spritesRef*/)
	{
		WorldState &world = WorldState::GetInstance();

		if (!projectileType)
		{
			return;
		}

		auto listIt = world.projectiles.find(projectileType->GetName());

		if (listIt != world.projectiles.end())
		{
			auto &projectileList = listIt->second;

			for (int i = static_cast<int>(projectileList.size()) - 1; i >= 0; --i)
			{
				ProjectileInstance *projectile = projectileList[i].get();
				if (!projectile)
				{
					projectileList.erase(projectileList.begin() + i);
					continue;
				}

				int targetIndex = LookUp::Get(projectile->targetUid);

				ItemInstance *target =
					(targetIndex >= 0 && targetIndex < static_cast<int>(world.items.size()))
						? world.items[targetIndex].get()
						: nullptr;

				if (target)
				{
					projectile->targetX = target->GetX();
					projectile->targetY = target->GetY();
				}

				float dx = projectile->targetX - projectile->x;
				float dy = projectile->targetY - projectile->y;
				float distSq = (dx * dx) + (dy * dy);

				float step = static_cast<float>(projectile->GetSpeed()) * (1.0f / 96.0f) * world.GetDeltaTime();

				float hitWindowSquared = std::max(targetThreshold, step * step);

				if (distSq < hitWindowSquared)
				{
					if (target)
					{
						int shooterIndex = LookUp::Get(projectile->uid);

						if (shooterIndex >= 0 && shooterIndex < static_cast<int>(world.items.size()))
						{
							int shooterUid = world.items[shooterIndex]->GetUid();
							auto targetState = target->GetState();

							if (targetState != ItemStates::Firing && targetState != ItemStates::Retreating &&
								target->GetOrders() &&
								(target->GetOrders()->order == Orders::Order::Stand ||
								 target->GetOrders()->order == Orders::Order::Standing))
							{
								target->GetOrders()->order = Orders::Order::Attacked;
								target->GetOrders()->target_uid = shooterUid;
							}
						}

						target->Damage(projectile->GetDamage());
						if (target->GetLife() <= 0)
						{
							target->SetOrders(Orders::Order::Destroyed);
						}
					}

					projectileList.erase(projectileList.begin() + i);
					continue;
				}

				float dist = std::sqrt(distSq);

				if (dist > 0.0f && step > 0.0f)
				{
					projectile->x += (dx / dist) * step;
					projectile->y += (dy / dist) * step;
				}
				else
				{
					projectile->x = projectile->targetX;
					projectile->y = projectile->targetY;
				}
			}
		}
	}

	MODULE_API void Draw(Vector<sf::Sprite *> *inSritesRef, const String &projectileName)
	{
		WorldState &world = WorldState::GetInstance();

		if (!world.projectiles.contains(projectileName))
		{
			return;
		}

		auto &projectileList = world.projectiles.at(projectileName);
		if (projectileList.empty())
		{
			return;
		}

		auto it = world.projectileRegistry.find(projectileName);
		if (it == world.projectileRegistry.end())
		{
			return;
		}

		int spriteIndex = it->second->spriteIndex;

		if (spriteIndex < 0 || spriteIndex >= static_cast<int>(inSritesRef->size()))
		{
			return;
		}

		sf::Sprite *sprite = (*inSritesRef)[spriteIndex];

		Window &window = Window::GetInstance();

		for (auto &projectile : projectileList)
		{
			sprite->setPosition(sf::Vector2f(
				((projectile->x * Globals::grid_size) + static_cast<float>(world.GetMapXOffset())),
				((projectile->y * Globals::grid_size) + static_cast<float>(world.GetMapYOffset()))));

			window.Draw(*sprite);
		}
	}

	MODULE_API void Clear()
	{
		WorldState &world = WorldState::GetInstance();
		world.projectiles.clear();
		Log::Info("Cleared projectile states");
	}
}
} // namespace TGX
