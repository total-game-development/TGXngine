#include "Textures.h"
#include <filesystem>
#include "Logs.h"

namespace TGX::UI
{
sf::Texture *Textures::Load(const String &path)
{
	auto found = textures.find(path);

	if (found != textures.end())
	{
		return found->second.get();
	}

	if (!std::filesystem::exists(path))
	{
		Log::Error("UI texture missing: " + path);
		textures[path] = nullptr;
		return nullptr;
	}

	auto texture = std::make_unique<sf::Texture>();

	if (!texture->loadFromFile(path))
	{
		Log::Error("UI texture failed to load: " + path);
		textures[path] = nullptr;
		return nullptr;
	}

	texture->setSmooth(true);

	sf::Texture *raw = texture.get();
	textures[path] = std::move(texture);

	return raw;
}

void Textures::Clear()
{
	Log::Clean("Clearing the UI texture cache");

	textures.clear();
}
} // namespace TGX::UI
