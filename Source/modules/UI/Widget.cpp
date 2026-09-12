#include "Widget.h"
#include <utility>
#include "Layout.h"
#include "Logs.h"
#include "Textures.h"
#include "Window.h"

namespace TGX::UI
{
namespace
{
constexpr float LINE_SPACING = 1.35f;

Vector<String> Split(const String &text, char delimiter)
{
	Vector<String> parts;
	String current;

	for (const char character : text)
	{
		if (character == delimiter)
		{
			parts.push_back(current);
			current.clear();
			continue;
		}

		current.push_back(character);
	}

	parts.push_back(current);

	return parts;
}
} // namespace

Vector<String> Wrap(const String &text, unsigned int fontSize, float width)
{
	Window &window = Window::GetInstance();

	Vector<String> lines;

	for (const String &paragraph : Split(text, '\n'))
	{
		if (paragraph.empty())
		{
			lines.emplace_back();
			continue;
		}

		String current;

		for (const String &word : Split(paragraph, ' '))
		{
			const String candidate = current.empty() ? word : current + " " + word;

			if (window.MeasureText(candidate, fontSize) > width && !current.empty())
			{
				lines.push_back(current);
				current = word;
				continue;
			}

			current = candidate;
		}

		lines.push_back(current);
	}

	return lines;
}

Widget::Widget(Element inElement)
	: element(std::move(inElement))
{
}

void Widget::Perform(const Action &action) const
{
	if (action.Empty() || !handler)
	{
		return;
	}

	handler(action);
}

bool Widget::Press(sf::Vector2f point)
{
	return Contains(point);
}

String Widget::Identity() const
{
	return element.title.empty() ? element.name : element.title;
}

bool Widget::Contains(sf::Vector2f point) const
{
	return point.x >= position.x && point.x <= position.x + size.x &&
		   point.y >= position.y && point.y <= position.y + size.y;
}

void Widget::SetHandler(Handler inHandler)
{
	handler = std::move(inHandler);
}

void Widget::SetFocused(bool inFocused)
{
	focused = inFocused;
}

bool Widget::IsFocused() const
{
	return focused;
}

bool Widget::WantsFocus() const
{
	return element.type == ElementType::TextInput;
}

const Element &Widget::Descriptor() const
{
	return element;
}

Label::Label(Element inElement)
	: Widget(std::move(inElement))
{
}

void Label::Arrange(sf::Vector2f view)
{
	Window &window = Window::GetInstance();

	size = {
		window.MeasureText(element.text, element.fontSize),
		static_cast<float>(element.fontSize) * LINE_SPACING};

	position = Layout::Anchored(
		Layout::Resolve(element.x, element.y, view),
		size,
		element.anchorX,
		element.anchorY);
}

void Label::Draw()
{
	Window::GetInstance().DrawText(element.text, position, element.fontSize, element.textFill);
}

Button::Button(Element inElement)
	: Widget(std::move(inElement))
{
}

void Button::Arrange(sf::Vector2f view)
{
	size = {element.width, element.height};

	position = Layout::Anchored(
		Layout::Resolve(element.x, element.y, view),
		size,
		element.anchorX,
		element.anchorY);
}

void Button::Draw()
{
	Window &window = Window::GetInstance();

	sf::RectangleShape box(size);
	box.setPosition(position);
	box.setFillColor(element.fill);

	if (element.strokeWidth > 0.0f)
	{
		box.setOutlineThickness(element.strokeWidth);
		box.setOutlineColor(element.stroke);
	}

	window.Draw(box);

	const float textWidth = window.MeasureText(element.text, element.fontSize);
	const float textHeight = static_cast<float>(element.fontSize) * LINE_SPACING;

	window.DrawText(
		element.text,
		{position.x + ((size.x - textWidth) * 0.5f), position.y + ((size.y - textHeight) * 0.5f)},
		element.fontSize,
		element.textFill);
}

bool Button::Press(sf::Vector2f point)
{
	if (!Contains(point))
	{
		return false;
	}

	Perform(element.action);

	return true;
}

IconButton::IconButton(Element inElement)
	: Widget(std::move(inElement))
{
	texture = Textures::Load(element.path + element.name);

	if (texture != nullptr)
	{
		sprite.setTexture(*texture, true);
	}
}

void IconButton::Arrange(sf::Vector2f view)
{
	if (texture == nullptr)
	{
		return;
	}

	sprite.setScale(element.scale, element.scale);

	size = {
		static_cast<float>(texture->getSize().x) * element.scale,
		static_cast<float>(texture->getSize().y) * element.scale};

	position = Layout::Anchored(
		Layout::Resolve(element.x, element.y, view),
		size,
		element.anchorX,
		element.anchorY);

	sprite.setPosition(position);
}

void IconButton::Draw()
{
	if (texture == nullptr)
	{
		return;
	}

	Window::GetInstance().Draw(sprite);
}

bool IconButton::Press(sf::Vector2f point)
{
	if (texture == nullptr || !Contains(point))
	{
		return false;
	}

	Perform(element.action);

	return true;
}

TextField::TextField(Element inElement)
	: Widget(std::move(inElement))
{
}

void TextField::Arrange(sf::Vector2f view)
{
	size = {
		element.width > 0.0f ? element.width : 500.0f,
		static_cast<float>(element.fontSize) * 2.4f};

	position = Layout::Anchored(
		Layout::Resolve(element.x, element.y, view),
		size,
		element.anchorX,
		element.anchorY);
}

void TextField::Draw()
{
	Window &window = Window::GetInstance();

	sf::RectangleShape box(size);
	box.setPosition(position);
	box.setFillColor(element.fill);
	box.setOutlineThickness(element.strokeWidth > 0.0f ? element.strokeWidth : 3.0f);
	box.setOutlineColor(focused ? sf::Color::White : element.stroke);

	window.Draw(box);

	const float padding = static_cast<float>(element.fontSize) * 0.6f;
	const sf::Vector2f caret(position.x + padding, position.y + (padding * 0.6f));

	if (value.empty() && !focused)
	{
		window.DrawText(element.placeholder, caret, element.fontSize, sf::Color(0xA9, 0xA9, 0xA9));
		return;
	}

	const String shown = element.password ? String(value.size(), '*') : value;

	window.DrawText(focused ? shown + "_" : shown, caret, element.fontSize, element.textFill);
}

bool TextField::Press(sf::Vector2f point)
{
	return Contains(point);
}

bool TextField::Character(unsigned int codepoint)
{
	if (!focused)
	{
		return false;
	}

	if (codepoint == '\b')
	{
		if (!value.empty())
		{
			value.pop_back();
		}

		return true;
	}

	if (codepoint == '\r' || codepoint == '\n')
	{
		Perform(element.action);
		return true;
	}

	if (codepoint < 32 || codepoint > 126)
	{
		return false;
	}

	value.push_back(static_cast<char>(codepoint));

	return true;
}

bool TextField::Key(int /*code*/)
{
	return false;
}

const String &TextField::Value() const
{
	return value;
}
} // namespace TGX::UI
