#pragma once

#include <nlohmann/json.hpp>
#include "Core.h"

using namespace nlohmann;

namespace TGX::UI
{
struct PageImage
{
	String name;
	float x = 0.0f;
	float y = 0.0f;
	bool placed = false;
};

struct PageEntry
{
	String sender;
	String subject;
	String body;
};

struct Page
{
	String key;
	String title;
	String body;
	String window = "h640";

	bool hasImage = false;
	PageImage image;

	Vector<PageEntry> inbox;
};

Map<String, Page> PagesFromJson(const json &source);
} // namespace TGX::UI
