#include "CardSystem.h"
namespace
{
	constexpr double TrashPaddingLeft = 24.0;
	constexpr double TrashPaddingRight = 48.0;
	constexpr double TrashPaddingTop = 64.0;
	constexpr double TrashPaddingBottom = 72.0;
	const Vec2 TrashCardSize{ 180, 248 };
	const Vec2 TrashCardSpacing{ 26, 20 };
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
	m_trashOrder.clear();
	m_showTrash = false;
	m_trashJustOpened = false;

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

	if (m_trashButtonScreen.leftClicked())
	{
		m_showTrash = (not m_showTrash);
		m_trashJustOpened = m_showTrash;
	}

	if (m_showTrash)
	{
		const Vec2 cardSize = TrashCardSize;
		const Vec2 spacing = TrashCardSpacing;
		const double contentWidth = m_trashModalSize.x - (TrashPaddingLeft + TrashPaddingRight);
		const double unitWidth = cardSize.x + spacing.x;
		const int32 columns = Max<int32>(1, static_cast<int32>((contentWidth + spacing.x) / unitWidth));
		const int32 totalCards = static_cast<int32>(m_trashOrder.size());
		const int32 rows = (totalCards + columns - 1) / columns;
		const double contentHeight = (rows > 0) ? rows * (cardSize.y + spacing.y) - spacing.y : 0.0;
		const double viewHeight = m_trashModalSize.y - (TrashPaddingTop + TrashPaddingBottom);

		if (contentHeight > viewHeight)
		{
			m_trashScroll -= Mouse::Wheel() * 32.0;
			m_trashScroll = Clamp(m_trashScroll, 0.0, Max(0.0, contentHeight - viewHeight));
		}
		else
		{
			m_trashScroll = 0.0;
		}

		if (not m_trashJustOpened)
		{
			if (m_trashCloseButton.leftClicked())
			{
				m_showTrash = false;
			}
			else if (MouseL.down() && (not m_trashModalRect.contains(Cursor::PosF())) && (not m_trashButtonScreen.contains(Cursor::PosF())))
			{
				m_showTrash = false;
			}

			if (KeyEscape.down())
			{
				m_showTrash = false;
			}
		}
	}
	else
	{
		m_trashScroll = 0.0;
	}

	m_trashJustOpened = false;
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
	m_trashOrder.clear();
	m_showTrash = false;
	m_trashJustOpened = false;
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

	const double extraRightPadding = 200.0;
	const double startX = Max(40.0, m_virtualSize.x - m_config.margin - extraRightPadding - totalWidth);
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
		card.isUsed = false;
		card.inTrash = false;
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

	const double buttonWidth = 160.0;
	const double buttonHeight = 60.0;
	double buttonX = m_offset.x + m_virtualSize.x * m_scale + 60.0;
	double buttonY = m_offset.y + m_virtualSize.y * m_scale - buttonHeight - 40.0;
	if (buttonX + buttonWidth > sceneWidth - 20.0)
	{
		buttonX = sceneWidth - buttonWidth - 20.0;
	}
	if (buttonY + buttonHeight > sceneHeight - 20.0)
	{
		buttonY = sceneHeight - buttonHeight - 20.0;
	}
	m_trashButtonScreen = RectF{ buttonX, buttonY, buttonWidth, buttonHeight };

	const Vec2 modalPos{ (sceneWidth - m_trashModalSize.x) * 0.5, (sceneHeight - m_trashModalSize.y) * 0.5 };
	m_trashModalRect = RectF{ modalPos, m_trashModalSize };
	m_trashCloseButton = RectF{ m_trashModalRect.tr().movedBy(-46, 6), Vec2{ 36, 36 } };
}

void CardSystem::updateCards()
{
	auto& cards = m_deck.cards();
	const Vec2 cursorVirtual = toVirtual(Cursor::PosF());

	updateDragging(cursorVirtual);

	for (auto& card : cards)
	{
		if (card.inTrash)
		{
			continue;
		}

		if (not card.isDragging)
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
				if (not card.inTrash)
				{
					card.isUsed = true;
					card.inTrash = true;
					m_trashOrder << *m_draggingIndex;
				}

				if (card.definition)
				{
					m_playLog.push_front(Format(card.definition->name, U" (", card.definition->id, U"#", card.instanceId, U")"));
					if (m_playLog.size() > 6)
					{
						m_playLog.pop_back();
					}
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
		if (cards[index].inTrash)
		{
			continue;
		}
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

	if (card.isUsed)
	{
		rect.draw(ColorF{ 0, 0, 0, 0.4 }); rect.drawFrame(4, ColorF{ 0.1, 0.1, 0.15, 0.8 });
	}
}

void CardSystem::drawUI() const
{
	const double margin = 20.0;
	const Vec2 panelSize{ 320, 200 };
	const RectF background{ margin, Scene::Height() - panelSize.y - margin, panelSize };
	background.draw(ColorF{ 0.05, 0.05, 0.08, 0.75 });
	background.drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3, 0.9 });

	Vec2 logPos = background.pos.movedBy(16, 96);
	for (size_t i = 0; i < m_playLog.size(); ++i)
	{
		m_bodyFont(Format(U"- ", m_playLog[i])).draw(logPos + Vec2{ 0, i * 24.0 }, ColorF{ 0.82 });
	}

	const ColorF buttonColor = m_showTrash ? ColorF{ 0.35, 0.25, 0.45, 0.9 } : ColorF{ 0.25, 0.22, 0.32, 0.85 };
	const ColorF frameColor = ColorF{ 0.6, 0.55, 0.8, 0.9 };
	const RectF buttonRect = m_trashButtonScreen;
	buttonRect.rounded(12).draw(buttonColor);
	buttonRect.rounded(12).drawFrame(2, 0, frameColor);
	m_bodyFont(Format(U"Trash (", m_trashOrder.size(), U")")).drawAt(buttonRect.center(), ColorF{ 0.95 });

	if (m_showTrash)
	{
		RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0, 0, 0, 0.55 });

		const RectF modal = m_trashModalRect;
		modal.rounded(18).draw(ColorF{ 0.08, 0.09, 0.12, 0.96 });
		modal.rounded(18).drawFrame(3, 0, ColorF{ 0.4, 0.45, 0.6, 0.9 });
		m_bodyFont(U"Trash").draw(modal.pos.movedBy(28, 22), ColorF{ 0.95 });

		const RectF closeButton = m_trashCloseButton;
		closeButton.rounded(10).draw(ColorF{ 0.3, 0.25, 0.35, 0.9 });
		closeButton.rounded(10).drawFrame(2, 0, frameColor);
		m_bodyFont(U"X").drawAt(closeButton.center(), ColorF{ 0.95 });

		const auto& cards = m_deck.cards();
		const Vec2 cardSize = TrashCardSize;
		const Vec2 spacing = TrashCardSpacing;
		const Vec2 contentOrigin = modal.pos.movedBy(TrashPaddingLeft, TrashPaddingTop);
		const Vec2 contentAreaSize{
			m_trashModalSize.x - (TrashPaddingLeft + TrashPaddingRight),
			m_trashModalSize.y - (TrashPaddingTop + TrashPaddingBottom)
		};
		const RectF clipRect{ contentOrigin, contentAreaSize };
		clipRect.drawFrame(2, 0, ColorF{ 0.25, 0.28, 0.36, 0.9 });

		const double availableWidth = Max(0.0, contentAreaSize.x);
		const double unitWidth = cardSize.x + spacing.x;
		const int32 maxColumns = (unitWidth > 0.0) ? static_cast<int32>(Math::Floor((availableWidth + spacing.x) / unitWidth)) : 1;
		const int32 columns = Max<int32>(1, maxColumns);

		const Rect scissor = clipRect.asRect();
		ScopedViewport2D viewport{ scissor };
		const Transformer2D transform{ Mat3x2::Translate(contentOrigin.x, contentOrigin.y - m_trashScroll) };

		int32 index = 0;
		for (const size_t deckIndex : m_trashOrder)
		{
			if (deckIndex >= cards.size())
			{
				continue;
			}

			const auto& card = cards[deckIndex];
			if (not card.definition)
			{
				continue;
			}

			const int32 row = index / columns;
			const int32 col = index % columns;
			const Vec2 pos{ col * unitWidth, row * (cardSize.y + spacing.y) };

			RectF cardRect{ pos, cardSize };
			if (const auto textureIt = m_textures.find(card.definition->id); textureIt != m_textures.end())
			{
				textureIt->second.resized(cardRect.size).draw(cardRect.pos);
			}
			cardRect.drawFrame(3, 0, ColorF{ 0.18, 0.18, 0.25, 0.85 });
			++index;
		}

		if (m_trashOrder.empty())
		{
			m_bodyFont(U"Trash is empty!!!!").drawAt(modal.center(), ColorF{ 0.8 });
		}
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















