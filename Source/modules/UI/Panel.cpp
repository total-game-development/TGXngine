#include "Panel.h"
#include <algorithm>
#include <cctype>
#include <utility>
#include "Layout.h"
#include "Logs.h"
#include "Textures.h"
#include "Window.h"

namespace TGX::UI
{
namespace
{
constexpr unsigned int TITLE_SIZE = 18;
constexpr unsigned int BODY_SIZE = 16;
constexpr unsigned int SENDER_SIZE = 14;
constexpr float LINE_HEIGHT = 22.0f;

constexpr float FRAME_INSET = 20.0f;
constexpr float FRAME_HEADER = 28.0f;

constexpr float TITLE_Y = 34.0f;
constexpr float TITLE_GAP = 30.0f;

constexpr float BODY_X = 45.0f;
constexpr float BODY_Y = 45.0f;
constexpr float BODY_MARGIN = 80.0f;
constexpr float IMAGE_GAP = 18.0f;
constexpr float CASCADE_STEP = 34.0f;

constexpr float CLOSE_INSET = 50.0f;
constexpr float CLOSE_TARGET = 26.0f;

constexpr float HEADER_X = 21.0f;
constexpr float HEADER_Y = 32.0f;
constexpr float HEADER_STEP = 84.0f;
constexpr float SUBJECT_X = 54.0f;
constexpr float SUBJECT_Y = 63.0f;
constexpr float SENDER_X = 821.0f;

constexpr float MAIL_X = 74.0f;
constexpr float MAIL_SENDER_Y = 93.0f;
constexpr float MAIL_BODY_Y = 180.0f;

const sf::Color TITLE_COLOUR = sf::Color(0x5B, 0xC8, 0xE8);
const sf::Color BODY_COLOUR = sf::Color(0xE8, 0xE9, 0xF3);
const sf::Color BACK_COLOUR = sf::Color(0xFF, 0xD4, 0x00);
} // namespace

Panel::Panel(Element inElement, Page inPage, String inWindowPath, String inImagePath)
	: Widget(std::move(inElement))
	, page(std::move(inPage))
	, windowPath(std::move(inWindowPath))
	, imagePath(std::move(inImagePath))
{
	backgroundTexture = Textures::Load(windowPath + page.window + ".png");

	if (backgroundTexture != nullptr)
	{
		background.setTexture(*backgroundTexture, true);

		size = {
			static_cast<float>(backgroundTexture->getSize().x),
			static_cast<float>(backgroundTexture->getSize().y)};
	}
	else
	{
		size = {640.0f, 480.0f};
	}

	if (element.close)
	{
		closeTexture = Textures::Load(windowPath + "close.png");

		if (closeTexture != nullptr)
		{
			closeButton.setTexture(*closeTexture, true);
		}
	}

	if (page.hasImage)
	{
		imageTexture = Textures::Load(imagePath + page.image.name);

		if (imageTexture != nullptr)
		{
			image.setTexture(*imageTexture, true);
		}
	}

	if (!page.inbox.empty())
	{
		headerTexture = Textures::Load(windowPath + "email_header.png");

		if (headerTexture != nullptr)
		{
			header.setTexture(*headerTexture, true);
		}
	}
}

void Panel::Arrange(sf::Vector2f view)
{
	if (!arranged)
	{
		position = element.positioned
			? Layout::Anchored(Layout::Resolve(element.x, element.y, view), size, element.anchorX, element.anchorY)
			: sf::Vector2f((view.x - size.x) * 0.5f, (view.y - size.y) * 0.5f);

		// Windows opened one after another step down and across instead of
		// landing on each other, and never step off the view doing it.
		const float step = CASCADE_STEP * static_cast<float>(cascade);

		position.x = std::min(position.x + step, std::max(0.0f, view.x - size.x));
		position.y = std::min(position.y + step, std::max(0.0f, view.y - size.y));

		arranged = true;
	}

	Compose();
}

sf::FloatRect Panel::Content() const
{
	return {
		position.x + FRAME_INSET,
		position.y + FRAME_HEADER,
		size.x - (FRAME_INSET * 2.0f),
		size.y - FRAME_HEADER - FRAME_INSET};
}

void Panel::SetCascade(int step)
{
	cascade = step;
}

void Panel::Compose()
{
	background.setPosition(position);

	const sf::FloatRect close = Close();

	closeButton.setPosition(close.left, close.top);

	if (body.empty() && !page.body.empty())
	{
		body = Wrap(page.body, BODY_SIZE, size.x - BODY_MARGIN);
	}

	if (imageTexture == nullptr)
	{
		return;
	}

	if (page.image.placed)
	{
		image.setPosition(position.x + page.image.x, position.y + page.image.y);
		return;
	}

	// An image with no declared position follows the text it illustrates, and
	// gives up whatever size it must to stay inside the frame.
	const float top = BodyTop() + (static_cast<float>(body.size()) * LINE_HEIGHT) + IMAGE_GAP;
	const float room = size.y - FRAME_INSET - top;

	const auto natural = sf::Vector2f(
		static_cast<float>(imageTexture->getSize().x),
		static_cast<float>(imageTexture->getSize().y));

	float scale = 1.0f;

	if (room > 0.0f && natural.y > room)
	{
		scale = room / natural.y;
	}

	const float widest = size.x - (FRAME_INSET * 2.0f);

	if (natural.x * scale > widest)
	{
		scale = widest / natural.x;
	}

	image.setScale(scale, scale);
	image.setPosition(position.x + ((size.x - (natural.x * scale)) * 0.5f), position.y + top);
}

float Panel::BodyTop() const
{
	return BODY_Y + (page.title.empty() ? 0.0f : TITLE_GAP);
}

sf::FloatRect Panel::Close() const
{
	const float width = closeTexture != nullptr ? static_cast<float>(closeTexture->getSize().x) : CLOSE_TARGET;
	const float height = closeTexture != nullptr ? static_cast<float>(closeTexture->getSize().y) : CLOSE_TARGET;

	return {
		position.x + size.x - CLOSE_INSET,
		position.y,
		std::max(width, CLOSE_TARGET),
		std::max(height, CLOSE_TARGET)};
}

sf::FloatRect Panel::Back() const
{
	const float width = Window::GetInstance().MeasureText("[ back ]", BODY_SIZE);

	return {position.x + BODY_X, position.y + BODY_Y, width, LINE_HEIGHT};
}

sf::FloatRect Panel::Entry(std::size_t index) const
{
	const float width = headerTexture != nullptr ? static_cast<float>(headerTexture->getSize().x) : size.x - (HEADER_X * 2.0f);
	const float height = headerTexture != nullptr ? static_cast<float>(headerTexture->getSize().y) : HEADER_STEP - 4.0f;

	return {
		position.x + HEADER_X,
		position.y + HEADER_Y + (HEADER_STEP * static_cast<float>(index)),
		width,
		height};
}

void Panel::Draw()
{
	Window &window = Window::GetInstance();

	if (backgroundTexture != nullptr)
	{
		window.Draw(background);
	}
	else
	{
		sf::RectangleShape fallback(size);
		fallback.setPosition(position);
		fallback.setFillColor(sf::Color(0x10, 0x14, 0x1C, 0xF0));
		fallback.setOutlineThickness(2.0f);
		fallback.setOutlineColor(sf::Color(0x00, 0xA7, 0xFF));

		window.Draw(fallback);
	}

	if (!page.title.empty())
	{
		String heading = page.title;

		std::transform(heading.begin(), heading.end(), heading.begin(), [](unsigned char letter) {
			return static_cast<char>(std::toupper(letter));
		});

		const float centred = position.x + ((size.x - window.MeasureText(heading, TITLE_SIZE)) * 0.5f);

		window.DrawText(heading, {centred, position.y + TITLE_Y}, TITLE_SIZE, TITLE_COLOUR);
	}

	if (imageTexture != nullptr)
	{
		window.Draw(image);
	}

	if (!page.inbox.empty())
	{
		if (opened < 0)
		{
			DrawInbox();
		}
		else
		{
			DrawMail();
		}
	}
	else
	{
		DrawBody();
	}

	if (!element.close)
	{
		return;
	}

	if (closeTexture != nullptr)
	{
		window.Draw(closeButton);
		return;
	}

	const sf::FloatRect close = Close();

	window.DrawText("X", {close.left, close.top}, TITLE_SIZE, TITLE_COLOUR);
}

void Panel::DrawBody()
{
	Window &window = Window::GetInstance();

	float y = position.y + BodyTop();

	for (const String &line : body)
	{
		window.DrawText(line, {position.x + BODY_X, y}, BODY_SIZE, BODY_COLOUR);

		y += LINE_HEIGHT;
	}
}

void Panel::DrawInbox()
{
	Window &window = Window::GetInstance();

	for (std::size_t index = 0; index < page.inbox.size(); index++)
	{
		const sf::FloatRect bounds = Entry(index);

		if (headerTexture != nullptr)
		{
			header.setPosition(bounds.left, bounds.top);

			window.Draw(header);
		}
		else
		{
			sf::RectangleShape row({bounds.width, bounds.height});
			row.setPosition(bounds.left, bounds.top);
			row.setFillColor(sf::Color(0x1C, 0x22, 0x2E, 0xC0));

			window.Draw(row);
		}

		const float y = position.y + SUBJECT_Y + (HEADER_STEP * static_cast<float>(index));

		window.DrawText(page.inbox[index].subject, {position.x + SUBJECT_X, y}, BODY_SIZE, TITLE_COLOUR);
		window.DrawText("From: " + page.inbox[index].sender, {position.x + SENDER_X, y}, SENDER_SIZE, TITLE_COLOUR);
	}
}

void Panel::DrawMail()
{
	Window &window = Window::GetInstance();

	const PageEntry &mail = page.inbox[static_cast<std::size_t>(opened)];

	const sf::FloatRect back = Back();

	window.DrawText("[ back ]", {back.left, back.top}, BODY_SIZE, BACK_COLOUR);

	window.DrawText(mail.subject, {position.x + SUBJECT_X, position.y + SUBJECT_Y}, TITLE_SIZE, TITLE_COLOUR);
	window.DrawText("From: " + mail.sender, {position.x + MAIL_X, position.y + MAIL_SENDER_Y}, BODY_SIZE, BODY_COLOUR);

	float y = position.y + MAIL_BODY_Y;

	for (const String &line : Wrap(mail.body, BODY_SIZE, size.x - BODY_MARGIN))
	{
		window.DrawText(line, {position.x + MAIL_X, y}, BODY_SIZE, BODY_COLOUR);

		y += LINE_HEIGHT;
	}
}

bool Panel::Press(sf::Vector2f point)
{
	if (!Contains(point))
	{
		return false;
	}

	if (element.close && Close().contains(point))
	{
		Dismiss();

		return true;
	}

	if (!page.inbox.empty())
	{
		if (opened >= 0)
		{
			if (Back().contains(point))
			{
				opened = -1;

				return true;
			}
		}
		else
		{
			for (std::size_t index = 0; index < page.inbox.size(); index++)
			{
				if (Entry(index).contains(point))
				{
					opened = static_cast<int>(index);

					return true;
				}
			}
		}
	}

	if (element.mobile)
	{
		dragging = true;
		grab = {point.x - position.x, point.y - position.y};
	}

	return true;
}

void Panel::Release()
{
	dragging = false;
}

void Panel::Move(sf::Vector2f point)
{
	if (!dragging)
	{
		return;
	}

	position = {point.x - grab.x, point.y - grab.y};

	Compose();
}

bool Panel::Key(int code)
{
	if (code != static_cast<int>(sf::Keyboard::Escape))
	{
		return false;
	}

	if (opened >= 0)
	{
		opened = -1;

		return true;
	}

	Dismiss();

	return true;
}

bool Panel::Dismissed() const
{
	return dismissed;
}

String Panel::Identity() const
{
	return page.key;
}

void Panel::Dismiss()
{
	dragging = false;
	dismissed = true;

	Log::Info("UI window closed: " + page.key);
}
} // namespace TGX::UI
