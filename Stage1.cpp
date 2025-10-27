#include "Stage1.h"
#include <utility>

Stage1::Stage1(const InitData& init)
	: IScene{ init }
{
	Scene::SetResizeMode(ResizeMode::Keep);

	const Vec2 virtualSize{ 1280, 720 };
	if (not m_mapSystem.loadFromJSON(U"maps/Stage1.json", virtualSize))
	{
		throw Error{ U"Failed Stage1.json" };
	}

	m_mapSystem.setFogOfWarEnabled(true);

	if (not m_player.init(U"player/player.png", m_mapSystem, Point{ 0, 0 }))
	{
		throw Error{ U"Failed texture" };
	}

	m_mapSystem.revealAround(m_player.gridPosition());

	const Array<String> starterIds{ U"zen", U"choku", U"ka" };
	CardHandConfig config;
	config.virtualSize = virtualSize;
	config.margin = 32.0;
	config.gap = 18.0;
	config.columns = 4;
	config.activationOffset = 40.0;

	if (not m_cardSystem.initialize(U"cards/cards.json", starterIds, config))
	{
		throw Error{ U"Failed card system" };
	}

	m_cardEffects.initialize(&m_mapSystem, &m_player);

	m_cardSystem.setCardPlayCallback([this](const String& cardId)
	{
		if (not m_gameOver)
		{
			m_cardEffects.onCardPlayed(cardId);
		}
	});
	m_cardSystem.setEndTurnCallback([this]()
	{
		onEndTurn();
	});

	m_previousPlayerGrid = m_player.gridPosition();
}

void Stage1::update()
{
	m_mapSystem.update();
	m_player.update(m_mapSystem);

	if (m_gameOver)
	{
		return;
	}

	if (m_treasureSelection.active)
	{
		handleTreasureSelectionInput();
		return;
	}

	if (KeyR.down())
	{
		m_cardSystem.resetUsage();
		m_cardEffects.clearAllEffects();
	}

	const Point before = m_player.gridPosition();
	m_previousPlayerGrid = before;

	m_cardSystem.update();

	if (m_gameOver)
	{
		return;
	}

	m_cardEffects.update();

	const Point after = m_player.gridPosition();
	if (after != before)
	{
		handleMovement(before, after);
	}

	m_mapSystem.revealAround(m_player.gridPosition());
	m_previousPlayerGrid = m_player.gridPosition();
}

void Stage1::draw() const
{
	Scene::SetBackground(ColorF{ 0.12, 0.12, 0.16 });
	m_mapSystem.draw();
	if (not m_gameOver)
	{
		m_cardEffects.draw();
	}
	drawActionCounter();
	m_player.draw(m_mapSystem);
	m_cardSystem.draw();
	drawTreasureSelection();
}

void Stage1::drawActionCounter() const
{
	const String text = U"行動回数: {}"_fmt(m_actionsRemaining);
	const ColorF color = (m_actionsRemaining <= 1) ? ColorF{ 0.98, 0.35, 0.35 } : ColorF{ 0.95 };
	const Vec2 pos{ 20, 16 };

	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(text).draw(pos, color);
	}
	else
	{
		static const Font fallbackFont{ 28 };
		fallbackFont(text).draw(pos, color);
	}
}

void Stage1::onEndTurn()
{
	if (m_gameOver)
	{
		return;
	}

	m_cardEffects.clearAllEffects();
	modifyActionPoints(-1);
}

void Stage1::modifyActionPoints(int32 delta)
{
	if ((delta == 0) || m_gameOver)
	{
		return;
	}

	const int64 updated = static_cast<int64>(m_actionsRemaining) + static_cast<int64>(delta);
	m_actionsRemaining = static_cast<int32>(Max<int64>(0, updated));

	if (m_actionsRemaining <= 0)
	{
		m_actionsRemaining = 0;
		handleGameOver();
	}
}

void Stage1::handleGameOver()
{
	if (m_gameOver)
	{
		return;
	}

	m_gameOver = true;
	m_cardEffects.clearAllEffects();
	changeScene(State::Ending);
}

void Stage1::handleMovement(const Point& previous, const Point& current)
{
	handleTileInteractions(previous, current);
}

void Stage1::handleTileInteractions(const Point& previous, const Point& current)
{
	const MapObjectType objectType = ToMapObjectType(m_mapSystem.objectIdAt(current));

	switch (objectType)
	{
	case MapObjectType::None:
		return;
	case MapObjectType::Box:
	case MapObjectType::Rock:
		// Safety fallback: revert to previous tile if an unexpected blockage is encountered.
		m_player.setGridPosition(previous, m_mapSystem);
		return;
	case MapObjectType::Treasure:
		if (not m_treasureSelection.active)
		{
			beginTreasureSelection(current);
		}
		return;
	case MapObjectType::FieldGlass:
		m_mapSystem.revealRadius(current, 2);
		m_mapSystem.removeObjectAt(current);
		return;
	case MapObjectType::Heal:
		modifyActionPoints(1);
		m_mapSystem.removeObjectAt(current);
		return;
	case MapObjectType::Camera:
		m_player.setGridPosition(previous, m_mapSystem);
		m_mapSystem.revealAround(m_player.gridPosition());
		return;
	case MapObjectType::Pillar:
		handleGoalReached();
		return;
	}
}

void Stage1::beginTreasureSelection(const Point& location)
{
	Array<String> options = m_cardSystem.sampleCardIds(3);
	if (options.isEmpty())
	{
		m_mapSystem.removeObjectAt(location);
		return;
	}

	m_treasureSelection.active = true;
	m_treasureSelection.location = location;
	m_treasureSelection.cardIds = std::move(options);
	m_treasureSelection.cardRects.clear();
	m_treasureHover.reset();

	const size_t count = m_treasureSelection.cardIds.size();
	const double spacing = 42.0;
	Array<Vec2> displaySizes;
	displaySizes.reserve(count);

	for (const auto& cardId : m_treasureSelection.cardIds)
	{
		const CardDefinition* def = m_cardSystem.findCardDefinition(cardId);
		Vec2 size = def ? def->size : Vec2{ 200, 280 };
		const double targetHeight = 320.0;
		const double scale = (size.y > 0.0) ? (targetHeight / size.y) : 1.0;
		displaySizes << size * scale;
	}

	double totalWidth = 0.0;
	for (size_t i = 0; i < count; ++i)
	{
		totalWidth += displaySizes[i].x;
		if (i + 1 != count)
		{
			totalWidth += spacing;
		}
	}

	const double startX = (Scene::Width() - totalWidth) * 0.5;
	const double startY = Scene::Height() * 0.22;
	double cursorX = startX;

	for (size_t i = 0; i < count; ++i)
	{
		const Vec2 size = displaySizes[i];
		m_treasureSelection.cardRects << RectF{ Vec2{ cursorX, startY }, size };
		cursorX += size.x + spacing;
	}

	m_cardEffects.clearAllEffects();
}

void Stage1::handleTreasureSelectionInput()
{
	if (not m_treasureSelection.active)
	{
		return;
	}

	m_treasureHover.reset();
	const Vec2 cursor = Cursor::PosF();

	for (size_t i = 0; i < m_treasureSelection.cardRects.size(); ++i)
	{
		if (m_treasureSelection.cardRects[i].contains(cursor))
		{
			m_treasureHover = i;
			break;
		}
	}

	if (MouseL.down())
	{
		if (m_treasureHover)
		{
			completeTreasureSelection(*m_treasureHover);
		}
	}
}

void Stage1::drawTreasureSelection() const
{
	if (not m_treasureSelection.active)
	{
		return;
	}

	RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.04, 0.05, 0.08, 0.75 });

	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(U"宝箱: 欲しいカードを1枚選んでください").drawAt(Scene::CenterF().x, Scene::Height() * 0.14, ColorF{ 0.95 });
	}

	for (size_t i = 0; i < m_treasureSelection.cardIds.size(); ++i)
	{
		const RectF rect = m_treasureSelection.cardRects[i];
		const bool hovered = (m_treasureHover && (*m_treasureHover == i));
		const ColorF borderColor = hovered ? ColorF{ 0.95, 0.86, 0.4, 0.95 } : ColorF{ 0.6, 0.65, 0.9, 0.85 };
		rect.stretched(6).draw(ColorF{ 0.12, 0.14, 0.2, hovered ? 0.62 : 0.48 });

		if (const Texture* texture = m_cardSystem.textureForCard(m_treasureSelection.cardIds[i]))
		{
			texture->resized(rect.size).draw(rect.pos);
		}
		else
		{
			rect.draw(ColorF{ 0.25, 0.3, 0.38, 0.8 });
		}

		rect.stretched(4).drawFrame(4, 0, borderColor);

		if (FontAsset::IsRegistered(U"MisakiFont"))
		{
			if (const CardDefinition* def = m_cardSystem.findCardDefinition(m_treasureSelection.cardIds[i]))
			{
				FontAsset(U"MisakiFont")(def->name).drawAt(rect.center().movedBy(0, rect.h * 0.58), ColorF{ 0.95 });
			}
		}
	}
}

void Stage1::completeTreasureSelection(size_t optionIndex)
{
	if ((not m_treasureSelection.active) || (optionIndex >= m_treasureSelection.cardIds.size()))
	{
		return;
	}

	const String cardId = m_treasureSelection.cardIds[optionIndex];
	m_cardSystem.addCardToDeck(cardId);
	m_mapSystem.removeObjectAt(m_treasureSelection.location);
	m_treasureSelection = TreasureSelection{};
	m_treasureHover.reset();
	m_mapSystem.revealAround(m_player.gridPosition());
}

void Stage1::handleGoalReached()
{
	if (m_gameOver)
	{
		return;
	}

	m_gameOver = true;
	m_cardEffects.clearAllEffects();
	changeScene(State::Stage2);
}
