#pragma once
#include <Siv3D.hpp>

struct CardDefinition
{
	String id;
	String name;
	String description;
	Vec2 size = Vec2{ 200, 280 };
	FilePath imagePath;
};

struct CardInstance
{
	int64 instanceId = 0;
	const CardDefinition* definition = nullptr;
	RectF rect;
	Vec2 homePosition = Vec2::Zero();
	bool isDragging = false;
	Vec2 dragOffset = Vec2::Zero();
	bool isUsed = false;
	bool inTrash = false;

	RectF bounds() const { return rect; }
};

class CardLibrary
{
public:
	bool loadFromJSON(const FilePathView& path);
	const Array<CardDefinition>& definitions() const { return m_definitions; }
	const CardDefinition* findDefinition(const StringView& id) const;

private:
	Array<CardDefinition> m_definitions;
};

class CardDeck
{
public:
	CardInstance& addCard(const CardDefinition& def, const Vec2& position, const Vec2& size);
	Array<CardInstance>& cards() { return m_cards; }
	const Array<CardInstance>& cards() const { return m_cards; }
	void resetUsage();

private:
	Array<CardInstance> m_cards;
	int64 m_nextInstanceId = 1;
};
