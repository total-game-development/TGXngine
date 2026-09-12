#include "Page.h"
#include "Logs.h"

namespace TGX::UI
{
Map<String, Page> PagesFromJson(const json &source)
{
	Map<String, Page> pages;

	if (!source.is_object())
	{
		return pages;
	}

	for (const auto &[key, entry] : source.items())
	{
		if (!entry.is_object())
		{
			Log::Warning("UI page is not an object: " + key);
			continue;
		}

		Page page;

		page.key = key;
		page.title = entry.value("title", String{});
		page.body = entry.value("body", String{});
		page.window = entry.value("window", page.window);

		if (entry.contains("image") && entry["image"].is_object())
		{
			const json &image = entry["image"];

			page.hasImage = true;
			page.image.name = image.value("name", String{});
			page.image.x = image.value("x", 0.0f);
			page.image.y = image.value("y", 0.0f);
			page.image.placed = image.contains("x") && image.contains("y");
		}

		if (entry.contains("inbox") && entry["inbox"].is_array())
		{
			for (const auto &mail : entry["inbox"])
			{
				PageEntry item;

				item.sender = mail.value("sender", String{});
				item.subject = mail.value("subject", String{});
				item.body = mail.value("body", String{});

				page.inbox.push_back(item);
			}
		}

		pages[key] = page;
	}

	return pages;
}
} // namespace TGX::UI
