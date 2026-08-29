#pragma once

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include "Core.h"
#include "Scene.h"
#include "SkirmishSetup.h"

using namespace nlohmann;

namespace TGX
{
class Skirmish : public Scene
{
private:
	struct Row
	{
		String team;
		String role;
	};

	struct Hotspot
	{
		enum class Kind : std::uint8_t
		{
			PreviousMap,
			NextMap,
			CycleTeam,
			CycleRole,
			Start,
			Cancel
		};

		Kind kind = Kind::Start;
		std::size_t row = 0;
		float x = 0.0f;
		float y = 0.0f;
		float w = 0.0f;
		float h = 0.0f;

		bool Contains(float px, float py) const
		{
			return px >= x && px <= (x + w) && py >= y && py <= (y + h);
		}
	};

	json maps;
	Vector<int> levels;
	std::size_t cursor = 0;

	Vector<Row> rows;
	Vector<String> teamChoices;
	Vector<Hotspot> hotspots;

	sf::Font font;
	sf::Texture previewTexture;
	sf::Sprite preview;
	bool hasPreview = false;

	float originX = 0.0f;
	float originY = 0.0f;

	void CollectLevels();
	void SelectLevel(std::size_t index);
	void LoadPreview();
	void Layout();
	void CycleTeam(std::size_t row);
	void CycleRole(std::size_t row);
	void Commit();

	void DrawPanel(float x, float y, float w, float h, sf::Color fill, sf::Color edge);
	void DrawLabel(const String &text, float x, float y, unsigned int size, sf::Color colour);
	float MeasureLabel(const String &text, unsigned int size);

	const json &CurrentLevel() const;
	String CurrentBriefing() const;

public:
	Skirmish();
	~Skirmish() override;

	void Init() override;
	void Update() override;
	void Draw() override;
	void Click() override;
	void RightClick() override;
	void Release() override;
	void Close() override;
	void Free() override;
};
} // namespace TGX
