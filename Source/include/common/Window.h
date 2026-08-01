#pragma once

#include <functional>
#include <string>
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Event.hpp"
#include "common_export.h"

namespace TGX
{

class Window
{
public:
	COMMON_API static Window &GetInstance();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	COMMON_API void Start();
	COMMON_API void Update();
	COMMON_API void Clear();
	COMMON_API void Display();
	COMMON_API void Close();

	COMMON_API sf::Vector2f GetMousePosition();
	COMMON_API sf::Vector2f PixelToCoords(sf::Vector2i pixel);

	COMMON_API void Draw(const sf::Drawable &drawable);
	COMMON_API void DrawText(const std::string &content, sf::Vector2f position, unsigned int size = 24, sf::Color color = sf::Color::White);

	COMMON_API bool ShouldClose();
	COMMON_API void SetEventCallbacks(std::function<void(sf::Event)> &key, std::function<void(sf::Event)> &mouse);

	COMMON_API sf::Vector2f GetViewSize() const;

	COMMON_API void MoveView(float x, float y);

private:
	Window();
	~Window() = default;

	void UpdateViewport(unsigned int width, unsigned int height);

	static constexpr float BASE_WIDTH = 1920.0f;
	static constexpr float BASE_HEIGHT = 1080.0f;

	sf::RenderWindow _window;

	bool _shouldClose;
	sf::View _fixed;

	sf::Font _mainFont;

	std::function<void(sf::Event)> _key;
	std::function<void(sf::Event)> _mouse;
};

} // namespace TGX
