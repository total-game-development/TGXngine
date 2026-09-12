#pragma once

#include <SFML/Graphics.hpp>
#include "Element.h"

namespace TGX::UI
{
using Handler = Function<void(const Action &)>;

Vector<String> Wrap(const String &text, unsigned int fontSize, float width);

class Widget
{
protected:
	Element element;
	Handler handler;

	sf::Vector2f position;
	sf::Vector2f size;

	bool focused = false;

	void Perform(const Action &action) const;

public:
	explicit Widget(Element inElement);
	virtual ~Widget() = default;

	virtual void Arrange(sf::Vector2f view) = 0;
	virtual void Draw() = 0;

	virtual void Update()
	{
	}

	virtual bool Press(sf::Vector2f point);

	virtual void Release()
	{
	}

	virtual void Move(sf::Vector2f /*point*/)
	{
	}

	virtual bool Character(unsigned int /*codepoint*/)
	{
		return false;
	}

	virtual bool Key(int /*code*/)
	{
		return false;
	}

	virtual bool Dismissed() const
	{
		return false;
	}

	virtual String Identity() const;

	bool Contains(sf::Vector2f point) const;
	void SetHandler(Handler inHandler);
	void SetFocused(bool inFocused);
	bool IsFocused() const;
	bool WantsFocus() const;
	const Element &Descriptor() const;
};

class Label : public Widget
{
public:
	explicit Label(Element inElement);

	void Arrange(sf::Vector2f view) override;
	void Draw() override;
};

class Button : public Widget
{
public:
	explicit Button(Element inElement);

	void Arrange(sf::Vector2f view) override;
	void Draw() override;
	bool Press(sf::Vector2f point) override;
};

class IconButton : public Widget
{
private:
	sf::Sprite sprite;
	sf::Texture *texture = nullptr;

public:
	explicit IconButton(Element inElement);

	void Arrange(sf::Vector2f view) override;
	void Draw() override;
	bool Press(sf::Vector2f point) override;
};

class TextField : public Widget
{
private:
	String value;

public:
	explicit TextField(Element inElement);

	void Arrange(sf::Vector2f view) override;
	void Draw() override;
	bool Press(sf::Vector2f point) override;
	bool Character(unsigned int codepoint) override;
	bool Key(int code) override;

	const String &Value() const;
};
} // namespace TGX::UI
