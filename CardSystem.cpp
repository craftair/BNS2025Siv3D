#include "CardSystem.h"
#include <algorithm>
namespace
{
	constexpr double TrashPaddingLeft = 24.0;
	constexpr double TrashPaddingRight = 48.0;
	constexpr double TrashPaddingTop = 64.0;
	constexpr double TrashPaddingBottom = 72.0;
	const Vec2 TrashCardSize{ 180, 248 };
	const Vec2 TrashCardSpacing{ 26, 20 };
	constexpr size_t DeckCopiesPerCard = 2;
	constexpr size_t InitialHandSize = 4;
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
	m_cardPlayCallback = nullptr;
	m_endTurnCallback = nullptr;
	m_cardPlayValidator = nullptr;

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
		for (size_t copy = 0; copy < DeckCopiesPerCard; ++copy)
		{
			auto& card = m_deck.addCard(*def, Vec2::Zero(), def->size);
			card.inHand = false;
		}
	}

	resetDeckState();
	return true;
}

void CardSystem::update()
{
	updateTransform();

	if (m_inputSuppressed)
	{
		m_draggingIndex.reset();
		return;
	}

	updateCards();

	if (m_endTurnButtonScreen.leftClicked())
	{
		endTurn();
	}

	if (m_deckButtonScreen.leftClicked())
	{
		m_showDeck = (not m_showDeck);
		m_deckJustOpened = m_showDeck;
		if (m_showDeck)
		{
			m_showTrash = false;
			m_trashScroll = 0.0;
		}
	}

	if (m_trashButtonScreen.leftClicked())
	{
		m_showTrash = (not m_showTrash);
		m_trashJustOpened = m_showTrash;
		if (m_showTrash)
		{
			m_showDeck = false;
			m_deckScroll = 0.0;
		}
	}

	if (m_showDeck)
	{
		const Vec2 cardSize = TrashCardSize;
		const Vec2 spacing = TrashCardSpacing;
		const double contentWidth = m_deckModalSize.x - (TrashPaddingLeft + TrashPaddingRight);
		const double unitWidth = cardSize.x + spacing.x;
		const int32 columns = Max<int32>(1, static_cast<int32>((contentWidth + spacing.x) / unitWidth));
		const int32 totalCards = static_cast<int32>(m_drawPile.size());
		const int32 rows = (totalCards + columns - 1) / columns;
		const double contentHeight = (rows > 0) ? rows * (cardSize.y + spacing.y) - spacing.y : 0.0;
		const double viewHeight = m_deckModalSize.y - (TrashPaddingTop + TrashPaddingBottom);

		if (contentHeight > viewHeight)
		{
			m_deckScroll -= Mouse::Wheel() * 32.0;
			m_deckScroll = Clamp(m_deckScroll, 0.0, Max(0.0, contentHeight - viewHeight));
		}
		else
		{
			m_deckScroll = 0.0;
		}

		if (not m_deckJustOpened)
		{
			if (m_deckCloseButton.leftClicked())
			{
				m_showDeck = false;
			}
			else if (MouseL.down() && (not m_deckModalRect.contains(Cursor::PosF())) && (not m_deckButtonScreen.contains(Cursor::PosF())))
			{
				m_showDeck = false;
			}

			if (KeyEscape.down())
			{
				m_showDeck = false;
			}
		}
	}
	else
	{
		m_deckScroll = 0.0;
	}

	m_deckJustOpened = false;

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
	resetDeckState();
}

void CardSystem::setInputSuppressed(bool suppressed)
{
	if (m_inputSuppressed == suppressed)
	{
		return;
	}

	m_inputSuppressed = suppressed;

	if (m_inputSuppressed)
	{
		m_draggingIndex.reset();
		m_showDeck = false;
		m_deckJustOpened = false;
		m_showTrash = false;
		m_trashJustOpened = false;
		m_deckScroll = 0.0;
		m_trashScroll = 0.0;
	}
}

size_t CardSystem::drawCards(size_t count)
{
	size_t drawn = 0;

	while ((drawn < count) && count > 0)
	{
		const size_t need = count - drawn;
		drawn += drawFromDeck(need);

		if (drawn >= count)
		{
			break;
		}

		reloadDeckFromTrash();
		const size_t additional = drawFromDeck(count - drawn);
		if (additional == 0)
		{
			break;
		}
		drawn += additional;
	}

	if (drawn > 0)
	{
		layoutHand();
	}

	return drawn;
}

bool CardSystem::discardCardInHand(size_t deckIndex)
{
	auto& cards = m_deck.cards();
	if (deckIndex >= cards.size())
	{
		return false;
	}

	auto& card = cards[deckIndex];
	if (card.inTrash || (not card.inHand))
	{
		return false;
	}

	card.isUsed = true;
	card.inTrash = true;
	card.inHand = false;
	card.isDragging = false;
	card.dragOffset = Vec2::Zero();

	if (std::find(m_trashOrder.begin(), m_trashOrder.end(), deckIndex) == m_trashOrder.end())
	{
		m_trashOrder << deckIndex;
	}

	removeFromHand(deckIndex);
	return true;
}

size_t CardSystem::discardHand()
{
	const Array<size_t> handCopy = m_handIndices;
	size_t discarded = 0;

	for (const size_t index : handCopy)
	{
		if (discardCardInHand(index))
		{
			++discarded;
		}
	}

	return discarded;
}

void CardSystem::shuffleAllAndDraw(size_t drawCount)
{
	auto& cards = m_deck.cards();
	m_drawPile.clear();
	m_handIndices.clear();
	m_trashOrder.clear();
	m_draggingIndex.reset();
	m_showDeck = false;
	m_deckJustOpened = false;
	m_showTrash = false;
	m_trashJustOpened = false;
	m_deckScroll = 0.0;
	m_trashScroll = 0.0;

	for (size_t index = 0; index < cards.size(); ++index)
	{
		auto& card = cards[index];
		card.isUsed = false;
		card.inTrash = false;
		card.inHand = false;
		card.isDragging = false;
		card.dragOffset = Vec2::Zero();
		card.homePosition = Vec2::Zero();
		card.rect.pos = Vec2::Zero();
		m_drawPile << index;
	}

	if (not m_drawPile.isEmpty())
	{
		m_drawPile.shuffle();
	}

	if (drawCount > 0)
	{
		drawCards(drawCount);
	}
	else
	{
		layoutHand();
	}
}

bool CardSystem::addDeckCardToHand(size_t deckIndex)
{
	auto& cards = m_deck.cards();
	if (deckIndex >= cards.size())
	{
		return false;
	}

	const auto drawIt = std::find(m_drawPile.begin(), m_drawPile.end(), deckIndex);
	if (drawIt == m_drawPile.end())
	{
		return false;
	}

	m_drawPile.erase(drawIt);

	auto& card = cards[deckIndex];
	card.isUsed = false;
	card.inTrash = false;
	card.inHand = true;
	card.isDragging = false;
	card.dragOffset = Vec2::Zero();
	card.homePosition = Vec2::Zero();
	card.rect.pos = Vec2::Zero();

	if (std::find(m_handIndices.begin(), m_handIndices.end(), deckIndex) == m_handIndices.end())
	{
		m_handIndices << deckIndex;
	}

	layoutHand();
	return true;
}

Optional<size_t> CardSystem::handCardAtScreenPos(const Vec2& screenPos) const
{
	if (m_scale <= 0.0)
	{
		return none;
	}

	const Vec2 cursorVirtual = toVirtual(screenPos);
	const auto& cards = m_deck.cards();

	Array<size_t> order;
	order.reserve(m_handIndices.size());

	for (const size_t index : m_handIndices)
	{
		if (cards[index].inTrash || (not cards[index].inHand))
		{
			continue;
		}

		if (not cards[index].isDragging)
		{
			order << index;
		}
	}

	for (const size_t index : m_handIndices)
	{
		if (cards[index].inTrash || (not cards[index].inHand))
		{
			continue;
		}

		if (cards[index].isDragging)
		{
			order << index;
		}
	}

	for (auto it = order.rbegin(); it != order.rend(); ++it)
	{
		const auto& card = cards[*it];
		if (card.rect.contains(cursorVirtual))
		{
			return *it;
		}
	}

	return none;
}

const CardInstance* CardSystem::instanceAt(size_t deckIndex) const
{
	const auto& cards = m_deck.cards();
	if (deckIndex >= cards.size())
	{
		return nullptr;
	}

	return &cards[deckIndex];
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

void CardSystem::resetDeckState()
{
	auto& cards = m_deck.cards();
	m_playLog.clear();
	m_trashOrder.clear();
	m_showTrash = false;
	m_trashJustOpened = false;
	m_trashScroll = 0.0;
	m_showDeck = false;
	m_deckJustOpened = false;
	m_deckScroll = 0.0;
	m_draggingIndex.reset();
	m_drawPile.clear();
	m_handIndices.clear();

	for (size_t i = 0; i < cards.size(); ++i)
	{
		auto& card = cards[i];
		card.isUsed = false;
		card.inTrash = false;
		card.isDragging = false;
		card.dragOffset = Vec2::Zero();
		card.inHand = false;
		card.homePosition = Vec2::Zero();
		card.rect.pos = Vec2::Zero();
		m_drawPile << i;
	}

	if (m_drawPile.isEmpty())
	{
		m_activationLine = m_virtualSize.y * 0.65;
		return;
	}

	m_drawPile.shuffle();

	drawHand(InitialHandSize);
}

size_t CardSystem::drawFromDeck(size_t count)
{
	size_t drawn = 0;
	auto& cards = m_deck.cards();

	for (size_t i = 0; i < count && not m_drawPile.isEmpty(); ++i)
	{
		const size_t cardIndex = m_drawPile.back();
		m_drawPile.pop_back();

		auto& card = cards[cardIndex];
		card.isUsed = false;
		card.inTrash = false;
		card.inHand = true;
		card.isDragging = false;
		card.dragOffset = Vec2::Zero();
		card.homePosition = Vec2::Zero();
		card.rect.pos = Vec2::Zero();

		if (std::find(m_handIndices.begin(), m_handIndices.end(), cardIndex) == m_handIndices.end())
		{
			m_handIndices << cardIndex;
		}

		++drawn;
	}

	return drawn;
}

void CardSystem::removeFromHand(size_t cardIndex)
{
	const auto it = std::remove(m_handIndices.begin(), m_handIndices.end(), cardIndex);
	m_handIndices.erase(it, m_handIndices.end());
	layoutHand();
}

void CardSystem::layoutHand()
{
	auto& cards = m_deck.cards();
	if (m_handIndices.isEmpty())
	{
		m_activationLine = m_virtualSize.y * 0.65;
		return;
	}

	const size_t count = m_handIndices.size();
	const size_t columns = (m_config.columns > 0) ? Min(m_config.columns, count) : count;
	const size_t rows = (count + columns - 1) / columns;

	Array<double> columnWidths(columns, 0.0);
	Array<double> rowHeights(rows, 0.0);

	for (size_t i = 0; i < count; ++i)
	{
		const auto& card = cards[m_handIndices[i]];
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
		auto& card = cards[m_handIndices[i]];
		const size_t col = i % columns;
		const size_t row = i / columns;
		const Vec2 pos{
			startX + columnOffsets[col] + (columnWidths[col] - card.rect.w) * 0.5,
			startY + rowOffsets[row] + (rowHeights[row] - card.rect.h) * 0.5
		};
		card.rect.pos = pos;
		card.homePosition = pos;
		card.inHand = true;
	}

	m_activationLine = Max(0.0, startY - m_config.activationOffset);
}

void CardSystem::drawHand(size_t desiredCount)
{
	while (m_handIndices.size() < desiredCount)
	{
		if (drawFromDeck(1) == 0)
		{
			reloadDeckFromTrash();
			if (drawFromDeck(1) == 0)
			{
				break;
			}
		}
	}

	layoutHand();
}

void CardSystem::endTurn()
{
	auto& cards = m_deck.cards();

	for (const size_t index : m_handIndices)
	{
		if (index >= cards.size())
		{
			continue;
		}

		auto& card = cards[index];
		if (card.inHand && not card.inTrash)
		{
			card.isUsed = true;
			card.inTrash = true;
			card.inHand = false;
			card.isDragging = false;
			card.dragOffset = Vec2::Zero();
			m_trashOrder << index;
		}
	}

	m_handIndices.clear();
	m_draggingIndex.reset();
	m_showDeck = false;
	m_deckJustOpened = false;
	m_showTrash = false;
	m_trashJustOpened = false;
	m_deckScroll = 0.0;
	m_trashScroll = 0.0;

	drawHand(InitialHandSize);

	if (m_endTurnCallback)
	{
		m_endTurnCallback();
	}
}

void CardSystem::reloadDeckFromTrash()
{
	if (m_trashOrder.isEmpty())
	{
		return;
	}

	auto& cards = m_deck.cards();

	for (const size_t index : m_trashOrder)
	{
		if (index >= cards.size())
		{
			continue;
		}

		auto& card = cards[index];
		card.isUsed = false;
		card.inTrash = false;
		card.inHand = false;
		card.isDragging = false;
		card.dragOffset = Vec2::Zero();
		card.homePosition = Vec2::Zero();
		card.rect.pos = Vec2::Zero();
		if (std::find(m_drawPile.begin(), m_drawPile.end(), index) == m_drawPile.end())
		{
			m_drawPile << index;
		}
	}

	m_trashOrder.clear();
	m_drawPile.shuffle();
	m_trashScroll = 0.0;
	m_deckScroll = 0.0;
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
	const double buttonSpacing = 16.0;
	double trashButtonX = m_offset.x + m_virtualSize.x * m_scale + 60.0;
	double baseButtonY = m_offset.y + m_virtualSize.y * m_scale - buttonHeight - 40.0;
	if (trashButtonX + buttonWidth > sceneWidth - 20.0)
	{
		trashButtonX = sceneWidth - buttonWidth - 20.0;
	}
	if (baseButtonY + buttonHeight > sceneHeight - 20.0)
	{
		baseButtonY = sceneHeight - buttonHeight - 20.0;
	}
	double deckButtonX = m_offset.x - buttonWidth - 60.0;
	if (deckButtonX < 20.0)
	{
		deckButtonX = 20.0;
	}
	if (baseButtonY < 20.0)
	{
		baseButtonY = 20.0;
	}
	double trashButtonY = baseButtonY;
	double endTurnButtonY = trashButtonY - (buttonHeight + buttonSpacing);
	if (endTurnButtonY < 20.0)
	{
		endTurnButtonY = 20.0;
		trashButtonY = endTurnButtonY + buttonHeight + buttonSpacing;
		if (trashButtonY + buttonHeight > sceneHeight - 20.0)
		{
			trashButtonY = sceneHeight - buttonHeight - 20.0;
		}
	}

	m_trashButtonScreen = RectF{ trashButtonX, trashButtonY, buttonWidth, buttonHeight };
	m_endTurnButtonScreen = RectF{ trashButtonX, endTurnButtonY, buttonWidth, buttonHeight };
	m_deckButtonScreen = RectF{ deckButtonX, trashButtonY, buttonWidth, buttonHeight };

	const Vec2 modalPos{ (sceneWidth - m_trashModalSize.x) * 0.5, (sceneHeight - m_trashModalSize.y) * 0.5 };
	m_trashModalRect = RectF{ modalPos, m_trashModalSize };
	m_trashCloseButton = RectF{ m_trashModalRect.tr().movedBy(-46, 6), Vec2{ 36, 36 } };
	m_deckModalRect = RectF{ modalPos, m_deckModalSize };
	m_deckCloseButton = RectF{ m_deckModalRect.tr().movedBy(-46, 6), Vec2{ 36, 36 } };
}

void CardSystem::updateCards()
{
	auto& cards = m_deck.cards();
	const Vec2 cursorVirtual = toVirtual(Cursor::PosF());

	updateDragging(cursorVirtual);

	for (auto& card : cards)
	{
		if (card.inTrash || (not card.inHand))
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
			if (card.isUsed || card.inTrash || (not card.inHand))
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
				bool allowPlay = true;
				if (card.definition && m_cardPlayValidator)
				{
					allowPlay = m_cardPlayValidator(card.definition->id);
				}

				if (allowPlay)
				{
					if (not card.inTrash)
					{
						card.isUsed = true;
						card.inTrash = true;
						card.inHand = false;
						removeFromHand(*m_draggingIndex);
						m_trashOrder << *m_draggingIndex;
					}

					if (card.definition)
					{
						m_playLog.push_front(Format(card.definition->name, U" (", card.definition->id, U"#", card.instanceId, U")"));
						if (m_playLog.size() > 6)
						{
							m_playLog.pop_back();
						}

						if (m_cardPlayCallback)
						{
							m_cardPlayCallback(card.definition->id);
						}
					}
				}
				else
				{
					card.rect.pos = card.homePosition;
					card.isUsed = false;
					card.inTrash = false;
					card.inHand = true;
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
	//Line{ 0, m_activationLine, m_virtualSize.x, m_activationLine }.draw(6, ColorF{ 0.85, 0.3, 0.3, 0.9 });

	const auto& cards = m_deck.cards();
	Array<size_t> order;
	order.reserve(m_handIndices.size());

	for (const size_t index : m_handIndices)
	{
		if (cards[index].inTrash || (not cards[index].inHand))
		{
			continue;
		}
		if (not cards[index].isDragging)
		{
			order << index;
		}
	}

	for (const size_t index : m_handIndices)
	{
		if (cards[index].inTrash || (not cards[index].inHand))
		{
			continue;
		}
		if (cards[index].isDragging)
		{
			order << index;
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

	const ColorF frameColor = ColorF{ 0.6, 0.55, 0.8, 0.9 };
	const RectF endTurnButtonRect = m_endTurnButtonScreen;
	const bool endTurnHovered = endTurnButtonRect.mouseOver();
	const ColorF endTurnColor = endTurnHovered ? ColorF{ 0.68, 0.38, 0.28, 0.95 } : ColorF{ 0.58, 0.32, 0.24, 0.9 };
	endTurnButtonRect.rounded(12).draw(endTurnColor);
	endTurnButtonRect.rounded(12).drawFrame(2, 0, frameColor);
	m_bodyFont(U"ターン終了").drawAt(endTurnButtonRect.center(), ColorF{ 0.95 });

	const ColorF deckButtonColor = m_showDeck ? ColorF{ 0.28, 0.35, 0.32, 0.9 } : ColorF{ 0.22, 0.28, 0.26, 0.85 };
	const RectF deckButtonRect = m_deckButtonScreen;
	deckButtonRect.rounded(12).draw(deckButtonColor);
	deckButtonRect.rounded(12).drawFrame(2, 0, frameColor);
	m_bodyFont(Format(U"山札 (", m_drawPile.size(), U")")).drawAt(deckButtonRect.center(), ColorF{ 0.95 });

	const ColorF trashButtonColor = m_showTrash ? ColorF{ 0.35, 0.25, 0.45, 0.9 } : ColorF{ 0.25, 0.22, 0.32, 0.85 };
	const RectF trashButtonRect = m_trashButtonScreen;
	trashButtonRect.rounded(12).draw(trashButtonColor);
	trashButtonRect.rounded(12).drawFrame(2, 0, frameColor);
	m_bodyFont(Format(U"墓地 (", m_trashOrder.size(), U")")).drawAt(trashButtonRect.center(), ColorF{ 0.95 });

	if (m_showDeck)
	{
		RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0, 0, 0, 0.55 });

		const RectF modal = m_deckModalRect;
		modal.rounded(18).draw(ColorF{ 0.08, 0.1, 0.1, 0.96 });
		modal.rounded(18).drawFrame(3, 0, ColorF{ 0.4, 0.55, 0.55, 0.9 });
		m_bodyFont(U"山札").draw(modal.pos.movedBy(28, 22), ColorF{ 0.95 });

		const RectF closeButton = m_deckCloseButton;
		closeButton.rounded(10).draw(ColorF{ 0.3, 0.32, 0.3, 0.9 });
		closeButton.rounded(10).drawFrame(2, 0, frameColor);
		m_bodyFont(U"X").drawAt(closeButton.center(), ColorF{ 0.95 });

		const auto& cards = m_deck.cards();
		const Vec2 cardSize = TrashCardSize;
		const Vec2 spacing = TrashCardSpacing;
		const Vec2 contentOrigin = modal.pos.movedBy(TrashPaddingLeft, TrashPaddingTop);
		const Vec2 contentAreaSize{
			m_deckModalSize.x - (TrashPaddingLeft + TrashPaddingRight),
			m_deckModalSize.y - (TrashPaddingTop + TrashPaddingBottom)
		};
		const RectF clipRect{ contentOrigin, contentAreaSize };
		clipRect.drawFrame(2, 0, ColorF{ 0.25, 0.32, 0.32, 0.9 });

		const double availableWidth = Max(0.0, contentAreaSize.x);
		const double unitWidth = cardSize.x + spacing.x;
		const int32 maxColumns = (unitWidth > 0.0) ? static_cast<int32>(Math::Floor((availableWidth + spacing.x) / unitWidth)) : 1;
		const int32 columns = Max<int32>(1, maxColumns);
		const double viewTop = contentOrigin.y;
		const double viewBottom = viewTop + contentAreaSize.y;

		Array<size_t> deckOrder = m_drawPile;
		std::reverse(deckOrder.begin(), deckOrder.end());

		int32 index = 0;
		for (const size_t deckIndex : deckOrder)
		{
			if (deckIndex >= cards.size())
			{
				++index;
				continue;
			}

			const auto& card = cards[deckIndex];
			if (not card.definition)
			{
				++index;
				continue;
			}

			const int32 row = index / columns;
			const int32 col = index % columns;
			const Vec2 pos{
				contentOrigin.x + col * unitWidth,
				contentOrigin.y + row * (cardSize.y + spacing.y) - m_deckScroll
			};

			RectF cardRect{ pos, cardSize };
			const double cardBottom = cardRect.y + cardRect.h;
			if ((cardBottom < viewTop) || (cardRect.y > viewBottom))
			{
				++index;
				continue;
			}

			if (const auto textureIt = m_textures.find(card.definition->id); textureIt != m_textures.end())
			{
				textureIt->second.resized(cardRect.size).draw(cardRect.pos);
			}
			cardRect.drawFrame(3, 0, ColorF{ 0.18, 0.25, 0.23, 0.85 });
			++index;
		}

		if (deckOrder.isEmpty())
		{
			m_bodyFont(U"山札は空").drawAt(clipRect.center(), ColorF{ 0.8 });
		}
	}
	else if (m_showTrash)
	{
		RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0, 0, 0, 0.55 });

		const RectF modal = m_trashModalRect;
		modal.rounded(18).draw(ColorF{ 0.08, 0.09, 0.12, 0.96 });
		modal.rounded(18).drawFrame(3, 0, ColorF{ 0.4, 0.45, 0.6, 0.9 });
		m_bodyFont(U"墓地").draw(modal.pos.movedBy(28, 22), ColorF{ 0.95 });

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
		const double viewTop = contentOrigin.y;
		const double viewBottom = viewTop + contentAreaSize.y;

		int32 index = 0;
		for (const size_t deckIndex : m_trashOrder)
		{
			if (deckIndex >= cards.size())
			{
				++index;
				continue;
			}

			const auto& card = cards[deckIndex];
			if (not card.definition)
			{
				++index;
				continue;
			}

			const int32 row = index / columns;
			const int32 col = index % columns;
			const Vec2 pos{
				contentOrigin.x + col * unitWidth,
				contentOrigin.y + row * (cardSize.y + spacing.y) - m_trashScroll
			};

			RectF cardRect{ pos, cardSize };
			const double cardBottom = cardRect.y + cardRect.h;
			if ((cardBottom < viewTop) || (cardRect.y > viewBottom))
			{
				++index;
				continue;
			}

			if (const auto textureIt = m_textures.find(card.definition->id); textureIt != m_textures.end())
			{
				textureIt->second.resized(cardRect.size).draw(cardRect.pos);
			}
			cardRect.drawFrame(3, 0, ColorF{ 0.18, 0.18, 0.25, 0.85 });
			++index;
		}

		if (m_trashOrder.empty())
		{
			const Vec2 emptyMessagePos = clipRect.center();
			m_bodyFont(U"墓地は空").drawAt(emptyMessagePos, ColorF{ 0.8 });
		}
	}
}

Array<String> CardSystem::sampleCardIds(size_t count) const
{
	Array<String> result;
	const auto& defs = m_library.definitions();
	if (defs.isEmpty() || (count == 0))
	{
		return result;
	}

	Array<size_t> indices;
	indices.reserve(defs.size());
	for (size_t i = 0; i < defs.size(); ++i)
	{
		indices << i;
	}

	indices.shuffle();
	const size_t limit = Min(count, indices.size());
	for (size_t i = 0; i < limit; ++i)
	{
		result << defs[indices[i]].id;
	}

	return result;
}

bool CardSystem::addCardToDeck(const String& cardId)
{
	const auto* def = m_library.findDefinition(cardId);
	if (not def)
	{
		return false;
	}

	auto& cards = m_deck.cards();
	const size_t newIndex = cards.size();
	auto& instance = m_deck.addCard(*def, Vec2::Zero(), def->size);
	instance.inHand = false;
	instance.inTrash = false;
	instance.isUsed = false;
	instance.isDragging = false;
	instance.dragOffset = Vec2::Zero();
	instance.homePosition = Vec2::Zero();
	instance.rect.pos = Vec2::Zero();

	if (std::find(m_drawPile.begin(), m_drawPile.end(), newIndex) == m_drawPile.end())
	{
		m_drawPile << newIndex;
	}

	m_drawPile.shuffle();
	return true;
}

bool CardSystem::removeCardFromDeck(const String& cardId, size_t count)
{
	bool removedAny = false;

	for (size_t n = 0; n < count; ++n)
	{
		auto& cards = m_deck.cards();
		bool removedThisIteration = false;

		for (size_t index = 0; index < cards.size(); ++index)
		{
			const auto* def = cards[index].definition;
			if (not def || (def->id != cardId))
			{
				continue;
			}

			auto adjustIndices = [&](Array<size_t>& container)
			{
				Array<size_t> updated;
				updated.reserve(container.size());
				for (const size_t value : container)
				{
					if (value == index)
					{
						continue;
					}

					if (value > index)
					{
						updated << (value - 1);
					}
					else
					{
						updated << value;
					}
				}
				container = std::move(updated);
			};

			adjustIndices(m_drawPile);
			adjustIndices(m_handIndices);
			adjustIndices(m_trashOrder);

			if (m_draggingIndex)
			{
				if (*m_draggingIndex == index)
				{
					m_draggingIndex.reset();
				}
				else if (*m_draggingIndex > index)
				{
					--(*m_draggingIndex);
				}
			}

			cards.erase(cards.begin() + index);
			layoutHand();

			removedAny = true;
			removedThisIteration = true;
			break;
		}

		if (not removedThisIteration)
		{
			break;
		}
	}

	return removedAny;
}

const Texture* CardSystem::textureForCard(const String& cardId) const
{
	if (const auto it = m_textures.find(cardId); it != m_textures.end())
	{
		return &(it->second);
	}
	return nullptr;
}

const CardDefinition* CardSystem::findCardDefinition(const String& cardId) const
{
	return m_library.findDefinition(cardId);
}

Array<String> CardSystem::allCardIds() const
{
	Array<String> result;
	result.reserve(m_library.definitions().size());
	for (const auto& def : m_library.definitions())
	{
		result << def.id;
	}
	return result;
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















