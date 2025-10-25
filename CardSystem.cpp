#include "CardSystem.h"

namespace
{
	constexpr double UsedTop = 80.0;
	constexpr double UsedLeft = 64.0;
	constexpr double UsedSpacing = 36.0;
}

bool CardSystem::initialize(const FilePathView& libraryPath, const Array<String>& deckIds, const CardHandConfig& config)
{
	m_config = config;
	m_virtualSize = m_config.virtualSize;
	m_scale = 1.0;
	m_offset = Vec2{ 0, 0 };
	m_activationLine = m_virtualSize.y * 0.65;
	m_draggingIndex.reset();
	m_playLog.clear();
	m_textures.clear();
	m_deck = CardDeck{};

	if (not m_library.loadFromJSON(libraryPath))
	{
		return false;
	}

	loadTextures();

	Array<const CardDefinition*> deckDefinitions;
	for (const auto& id : deckIds)
	{
		if (const auto* def = m_library.findDefinition(id))
		{
			deckDefinitions << def;
		}
	}

	if (deckDefinitions.isEmpty())
	{
		return false;
	}

	for (const auto* def : deckDefinitions)
	{
		m_deck.addCard(*def, Vec2::Zero(), def->size);
	}

	setupInitialLayout();
	return true;
}

void CardSystem::update()
{
	updateTransform();
	updateCards();
}

void CardSystem::draw() const
{
	drawScene();
	drawUI();
}

void CardSystem::resetUsage()
{
	m_deck.resetUsage();
	m_playLog.clear();
}

void CardSystem::loadTextures()
{
	m_textures.clear();
	for (const auto& def : m_library.definitions())
	{
		if (def.imagePath.isEmpty())
		{
			continue;
		}

		if (m_textures.contains(def.id))
		{
			continue;
		}

		Texture texture{ def.imagePath };
		if (texture)
		{
			m_textures.emplace(def.id, std::move(texture));
		}
	}
}

void CardSystem::setupInitialLayout()
{
	auto& cards = m_deck.cards();
	if (cards.isEmpty())
	{
		m_activationLine = m_virtualSize.y * 0.65;
		return;
	}

	const size_t count = cards.size();
	const size_t columns = (m_config.columns > 0) ? Min(m_config.columns, count) : count;
	const size_t rows = (count + columns - 1) / columns;

	Array<double> columnWidths(columns, 0.0);
	Array<double> rowHeights(rows, 0.0);

	for (size_t i = 0; i < count; ++i)
	{
		const auto& card = cards[i];
		const size_t col = i % columns;
		const size_t row = i / columns;
		columnWidths[col] = Max(columnWidths[col], card.rect.w);
		rowHeights[row] = Max(rowHeights[row], card.rect.h);
	}

	double totalWidth = 0.0;
	for (size_t c = 0; c < columns; ++c)
	{
		if (c > 0)
		{
			totalWidth += m_config.gap;
		}
		totalWidth += columnWidths[c];
	}

	double totalHeight = 0.0;
	for (size_t r = 0; r < rows; ++r)
	{
		if (r > 0)
		{
			totalHeight += m_config.gap;
		}
		totalHeight += rowHeights[r];
	}

	const double startX = m_virtualSize.x - m_config.margin - totalWidth;
	const double startY = m_virtualSize.y - m_config.margin - totalHeight;

	Array<double> columnOffsets(columns, 0.0);
	{
		double accum = 0.0;
		for (size_t c = 0; c < columns; ++c)
		{
			columnOffsets[c] = accum;
			accum += columnWidths[c] + m_config.gap;
		}
	}

	Array<double> rowOffsets(rows, 0.0);
	{
		double accum = 0.0;
		for (size_t r = 0; r < rows; ++r)
		{
			rowOffsets[r] = accum;
			accum += rowHeights[r] + m_config.gap;
		}
	}

	for (size_t i = 0; i < count; ++i)
	{
		auto& card = cards[i];
		const size_t col = i % columns;
		const size_t row = i / columns;
		const Vec2 pos{
			startX + columnOffsets[col] + (columnWidths[col] - card.rect.w) * 0.5,
			startY + rowOffsets[row] + (rowHeights[row] - card.rect.h) * 0.5
		};
		card.rect.pos = pos;
		card.homePosition = pos;
	}

	m_activationLine = Max(0.0, startY - m_config.activationOffset);
}

void CardSystem::updateTransform()
{
	const double sceneWidth = Scene::Width();
	const double sceneHeight = Scene::Height();

	const double scaleX = sceneWidth / m_virtualSize.x;
	const double scaleY = sceneHeight / m_virtualSize.y;

	m_scale = Math::Min(scaleX, scaleY);
	m_offset = Vec2{ (sceneWidth - m_virtualSize.x * m_scale) * 0.5, (sceneHeight - m_virtualSize.y * m_scale) * 0.5 };
}

void CardSystem::updateCards()
{
	auto& cards = m_deck.cards();
	const Vec2 cursorVirtual = toVirtual(Cursor::PosF());

	updateDragging(cursorVirtual);

	int32 usedCount = 0;
	for (auto& card : cards)
	{
		if (card.isUsed)
		{
			const Vec2 usedPos{ UsedLeft + usedCount * (card.rect.w + UsedSpacing), UsedTop };
			card.rect.pos = usedPos;
			++usedCount;
		}
		else if (not card.isDragging)
		{
			card.rect.pos = card.homePosition;
		}
	}
}

void CardSystem::updateDragging(const Vec2& cursorVirtual)
{
	auto& cards = m_deck.cards();

	if (MouseL.down() && isInsideVirtual(cursorVirtual))
	{
		for (int32 i = static_cast<int32>(cards.size()) - 1; i >= 0; --i)
		{
			auto& card = cards[i];
			if (card.isUsed)
			{
				continue;
			}

			if (card.rect.contains(cursorVirtual))
			{
				card.isDragging = true;
				card.dragOffset = cursorVirtual - card.rect.pos;
				m_draggingIndex = i;
				break;
			}
		}
	}

	if (m_draggingIndex)
	{
		auto& card = cards[*m_draggingIndex];

		if (MouseL.pressed())
		{
			card.rect.pos = cursorVirtual - card.dragOffset;
		}
		else
		{
			card.isDragging = false;
			const bool used = (card.rect.y <= m_activationLine);

			if (used)
			{
				card.isUsed = true;
				m_playLog.push_front(Format(card.definition->id, U"#", card.instanceId));
				if (m_playLog.size() > 6)
				{
					m_playLog.pop_back();
				}
			}
			else
			{
				card.rect.pos = card.homePosition;
			}

			m_draggingIndex.reset();
		}
	}
}

void CardSystem::drawScene() const
{
	const Transformer2D transformer{ Mat3x2::Scale(m_scale).translated(m_offset) };

	RectF{ 0, 0, m_virtualSize.x, m_activationLine }.draw(ColorF{ 0.15, 0.18, 0.3, 0.15 });
	Line{ 0, m_activationLine, m_virtualSize.x, m_activationLine }.draw(6, ColorF{ 0.85, 0.3, 0.3, 0.9 });

	const auto& cards = m_deck.cards();
	Array<size_t> order;
	order.reserve(cards.size());

	for (size_t i = 0; i < cards.size(); ++i)
	{
		if (not cards[i].isDragging)
		{
			order << i;
		}
	}

	for (size_t i = 0; i < cards.size(); ++i)
	{
		if (cards[i].isDragging)
		{
			order << i;
		}
	}

	for (const size_t index : order)
	{
		drawCard(cards[index]);
	}
}

void CardSystem::drawCard(const CardInstance& card) const
{
	if (not card.definition)
	{
		return;
	}

	const RectF rect = card.rect;
	const auto textureIt = m_textures.find(card.definition->id);

	if (textureIt != m_textures.end())
	{
		textureIt->second.resized(rect.size).draw(rect.pos);
	}
	else
	{
		ColorF fill = card.definition->color;
		if (card.isUsed)
		{
			fill.a = 0.4;
		}

		rect.draw(fill);
	}

	if (card.isUsed)
	{
		rect.draw(ColorF{ 0, 0, 0, 0.4 });
	}
}

void CardSystem::drawUI() const
{
	RectF background{ 20, 20, 320, 200 };
	background.draw(ColorF{ 0.05, 0.05, 0.08, 0.75 });
	background.drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3, 0.9 });

	m_bodyFont(U"Drag cards upward to play them.").draw(36, 36, ColorF{ 0.95 });
	m_bodyFont(U"Release above the red line to consume.").draw(36, 60, ColorF{ 0.85 });
	m_bodyFont(U"Press R to reset the hand.").draw(36, 84, ColorF{ 0.85 });

	double logY = 120.0;
	for (size_t i = 0; i < m_playLog.size(); ++i)
	{
		m_bodyFont(Format(U"- ", m_playLog[i])).draw(36, logY + i * 24.0, ColorF{ 0.82 });
	}
}

Vec2 CardSystem::toVirtual(const Vec2& screenPos) const
{
	if (m_scale <= 0.0)
	{
		return Vec2::Zero();
	}

	return (screenPos - m_offset) / m_scale;
}

bool CardSystem::isInsideVirtual(const Vec2& pos) const
{
	return (pos.x >= 0.0 && pos.y >= 0.0 && pos.x <= m_virtualSize.x && pos.y <= m_virtualSize.y);
}
