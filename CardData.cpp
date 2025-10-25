#include "CardData.h"

namespace
{
	double readNumber(const JSON& node, const double fallback)
	{
		if (const auto opt = node.getOpt<double>())
		{
			return *opt;
		}
		return fallback;
	}

	ColorF parseColor(const JSON& node)
	{
		ColorF result = Palette::Lightgray;

		if (not node.isArray())
		{
			return result;
		}

		const size_t count = node.size();

		if (count >= 1)
		{
			result.r = readNumber(node[0], result.r);
		}
		if (count >= 2)
		{
			result.g = readNumber(node[1], result.g);
		}
		if (count >= 3)
		{
			result.b = readNumber(node[2], result.b);
		}
		if (count >= 4)
		{
			result.a = readNumber(node[3], result.a);
		}

		return result;
	}

	Vec2 parseSize(const JSON& node)
	{
		Vec2 size{ 200, 280 };

		if (not node.isArray())
		{
			return size;
		}

		const size_t count = node.size();

		if (count >= 1)
		{
			size.x = readNumber(node[0], size.x);
		}
		if (count >= 2)
		{
			size.y = readNumber(node[1], size.y);
		}

		return size;
	}
}

bool CardLibrary::loadFromJSON(const FilePathView& path)
{
	const JSON json = JSON::Load(path);
	if (not json)
	{
		return false;
	}

	const JSON cardsNode = json[U"cards"];
	if (not cardsNode.isArray())
	{
		return false;
	}

	m_definitions.clear();
	for (const auto& cardValue : cardsNode.arrayView())
	{
		if (not cardValue.isObject())
		{
			continue;
		}

		CardDefinition def;

		if (const auto idOpt = cardValue[U"id"].getOpt<String>())
		{
			def.id = *idOpt;
		}
		else
		{
			continue;
		}

		def.name = cardValue[U"name"].getOr<String>(def.id);
		def.cost = cardValue[U"cost"].getOr<int32>(0);
		def.color = parseColor(cardValue[U"color"]);
		def.size = parseSize(cardValue[U"size"]);
		def.imagePath = cardValue[U"image"].getOr<String>(U"");

		m_definitions << def;
	}

	return (not m_definitions.isEmpty());
}

const CardDefinition* CardLibrary::findDefinition(const StringView& id) const
{
	for (const auto& def : m_definitions)
	{
		if (def.id == id)
		{
			return &def;
		}
	}
	return nullptr;
}

CardInstance& CardDeck::addCard(const CardDefinition& def, const Vec2& position, const Vec2& size)
{
	auto& instance = m_cards.emplace_back();
	instance.instanceId = m_nextInstanceId++;
	instance.definition = &def;
	instance.rect = RectF{ position, size };
	instance.homePosition = position;

	return instance;
}

void CardDeck::resetUsage()
{
	for (auto& card : m_cards)
	{
		card.isUsed = false;
		card.rect.pos = card.homePosition;
		card.isDragging = false;
		card.dragOffset = Vec2::Zero();
	}
}

