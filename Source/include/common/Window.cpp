#include "Window.h"
#include "Globals.h"
#include "SFML/Graphics/Text.hpp"
#include "WorldState.h"

namespace TGX
{

Window &Window::GetInstance()
{
	static Window window;
	return window;
}

Window::Window() : _shouldClose(false)
{
	WorldState &world = WorldState::GetInstance();

	if (world.IsProduction())
	{
		_window.create(sf::VideoMode::getDesktopMode(), "TGXngine", sf::Style::Fullscreen);
	}
	else
	{
		const int width = Globals::canvasWidth + world.GetCanvasOffsetWidth();
		const int height = Globals::canvasHeight + world.GetCanvasOffsetHeight();

		_window.create(sf::VideoMode(width, height), "TGXngine", sf::Style::Default);
	}
}

void Window::Start()
{
	WorldState &world = WorldState::GetInstance();

	_window.setVerticalSyncEnabled(Globals::verticalSync);

	if (Globals::verticalSync)
	{
		_window.setFramerateLimit(0);
	}
	else
	{
		_window.setFramerateLimit(Globals::targetFPS);
	}

	_window.setMouseCursorVisible(false);

	if (world.IsProduction())
	{
		_fixed = sf::View(sf::FloatRect(0.0f, 0.0f, BASE_WIDTH, BASE_HEIGHT));
		_fixed.setCenter(BASE_WIDTH * 0.5f, BASE_HEIGHT * 0.5f);

		UpdateViewport(_window.getSize().x, _window.getSize().y);
	}
	else
	{
		sf::Vector2u winSize = _window.getSize();

		_fixed = sf::View(sf::FloatRect(
			0.0f,
			0.0f,
			static_cast<float>(winSize.x),
			static_cast<float>(winSize.y)));

		_fixed.setCenter(winSize.x * 0.5f, winSize.y * 0.5f);
	}

	_window.setView(_fixed);

	if (!_mainFont.loadFromFile("Resources/courier.ttf"))
	{
		// handle error
	}
}

bool Window::ShouldClose()
{
	return !_window.isOpen() || _shouldClose;
}

void Window::SetEventCallbacks(std::function<void(sf::Event)> &key, std::function<void(sf::Event)> &mouse)
{
	_key = std::move(key);
	_mouse = std::move(mouse);
}

sf::Vector2f Window::PixelToCoords(sf::Vector2i pixel)
{
	return _window.mapPixelToCoords(pixel, _fixed);
}

sf::Vector2f Window::GetMousePosition()
{
	return _window.mapPixelToCoords(sf::Mouse::getPosition(_window), _fixed);
}

sf::Vector2f Window::GetViewSize() const
{
	return _fixed.getSize();
}

void Window::UpdateViewport(unsigned int width, unsigned int height)
{
	_fixed.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
}

void Window::Update()
{
	WorldState &world = WorldState::GetInstance();

	sf::Event event;
	while (_window.pollEvent(event))
	{
		switch (event.type)
		{
			case sf::Event::Closed:
				{
					_shouldClose = true;
					return;
				}

			case sf::Event::Resized:
				{
					if (world.IsProduction())
					{
						UpdateViewport(event.size.width, event.size.height);
					}
					else
					{
						_fixed.setSize(
							static_cast<float>(event.size.width),
							static_cast<float>(event.size.height));

						_fixed.setCenter(_fixed.getSize() * 0.5f);
					}

					_window.setView(_fixed);
					break;
				}

			case sf::Event::MouseMoved:
			case sf::Event::MouseButtonPressed:
			case sf::Event::MouseButtonReleased:
				{
					if (_mouse)
					{
						_mouse(event);
					}
					break;
				}

			case sf::Event::KeyPressed:
				{
					if (_key)
					{
						_key(event);
					}
					break;
				}

			default:
				break;
		}

		if (world.IsClosed())
		{
			break;
		}
	}
}

void Window::Clear()
{
	_window.clear();
	_window.setView(_fixed);
}

void Window::Display()
{
	_window.display();
}

void Window::Close()
{
	_window.close();
}

void Window::Draw(const sf::Drawable &drawable)
{
	_window.draw(drawable);
}

void Window::DrawText(const std::string &content, sf::Vector2f position, unsigned int size, sf::Color color)
{
	sf::Text text;
	text.setFont(_mainFont);
	text.setString(content);
	text.setCharacterSize(size);
	text.setFillColor(color);
	text.setPosition(position);

	_window.draw(text);
}

void Window::MoveView(float x, float y)
{
	_fixed.move(x, y);
}

} // namespace TGX
