#include "CardEffectController.h"
#include "MapSystem.h"
#include "MapObjectTypes.h"
#include "Player.h"
#include "StageScene.h"
#include "CardSystem.h"
#include <array>

namespace
{
	constexpr std::array<Point, 4> CardinalDirections{
		Point{ 1, 0 },
		Point{ -1, 0 },
		Point{ 0, 1 },
		Point{ 0, -1 }
	};
}

void CardEffectController::initialize(MapSystem* mapSystem, Player* player, CardSystem* cardSystem, StageScene* stage)
{
	m_mapSystem = mapSystem;
	m_player = player;
	m_cardSystem = cardSystem;
	m_stage = stage;
	clearAllEffects();
}

void CardEffectController::update()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	if (m_state == EffectState::None)
	{
		tryActivateNextCard();
	}

	if (m_state == EffectState::None)
	{
		return;
	}

	if (isMapTargetState(m_state))
	{
		updateMapTargeting();
		return;
	}

	switch (m_state)
	{
	case EffectState::HandDiscard:
		updateHandDiscard();
		break;
	case EffectState::DeckSelect:
		updateDeckSelection();
		break;
	case EffectState::ChoiceHa2:
	case EffectState::ChoiceKou3:
		updateChoicePrompt();
		break;
	default:
		break;
	}
}

void CardEffectController::draw() const
{
	if (m_state == EffectState::None)
	{
		return;
	}

	if (isMapTargetState(m_state))
	{
		drawMapTargeting();
	}
	else if (m_state == EffectState::HandDiscard)
	{
		drawHandDiscard();
	}
	else if (m_state == EffectState::DeckSelect)
	{
		drawDeckSelection();
	}
	else if ((m_state == EffectState::ChoiceHa2) || (m_state == EffectState::ChoiceKou3))
	{
		drawChoicePrompt();
	}
}

void CardEffectController::onCardPlayed(const String& cardId)
{
	if (cardId == U"ha")
	{
		const size_t repeatCount = 1 + m_nextCardRepeats;
		m_nextCardRepeats = 0;
		m_nextCardRepeats += repeatCount;
		return;
	}

	const size_t repeatCount = 1 + m_nextCardRepeats;
	m_nextCardRepeats = 0;

	for (size_t i = 0; i < repeatCount; ++i)
	{
		if (applyImmediateEffect(cardId))
		{
			continue;
		}

		enqueueTargetedEffect(cardId);
	}
}

void CardEffectController::clearAllEffects()
{
	m_pendingCardEffects.clear();
	m_canBreakBoxesThisTurn = false;
	m_nextCardRepeats = 0;
	m_deckSelectionType = DeckSelectionType::None;
	m_deckSelectionIndices.clear();
	m_deckSelectionRects.clear();
	m_deckSelectionHover.reset();
	m_deckSelectionLimited = false;
	m_choiceOptions.clear();
	m_choiceOptionRects.clear();
	m_choiceHover.reset();
	m_choiceType = ChoiceType::None;
	if (m_cardSystem)
	{
		m_cardSystem->setInputSuppressed(false);
	}
	clearTargeting(false);
}

bool CardEffectController::applyImmediateEffect(const String& cardId)
{
	if (cardId == U"ka")
	{
		m_canBreakBoxesThisTurn = true;
		return true;
	}

	if (cardId == U"dou")
	{
		if (m_mapSystem && m_player)
		{
			m_mapSystem->revealRadius(m_player->gridPosition(), 2);
		}
		return true;
	}

	if (cardId == U"dou2")
	{
		if (m_stage)
		{
			m_stage->activateFullMapVision();
		}
		return true;
	}

	if (cardId == U"pin")
	{
		if (m_stage)
		{
			m_stage->modifyActionPoints(1);
		}
		return true;
	}

	if (cardId == U"pin2")
	{
		if (m_stage)
		{
			m_stage->healActionsToFull();
		}
		return true;
	}

	if (cardId == U"ke")
	{
		if (m_cardSystem)
		{
			m_cardSystem->shuffleAllAndDraw(1);
		}
		return true;
	}

	if (cardId == U"soku")
	{
		if (m_cardSystem)
		{
			m_cardSystem->discardHand();
			m_cardSystem->drawCards(4);
		}
		return true;
	}

	if (cardId == U"nyuu")
	{
		if (m_stage)
		{
			m_stage->activateDebuffImmunity();
		}
		return true;
	}

	if (cardId == U"ryaku")
	{
		if (m_mapSystem)
		{
			const auto visibility = m_mapSystem->visibilitySnapshot();
			Array<Point> candidates;

			for (size_t y = 0; y < visibility.size(); ++y)
			{
				const auto& row = visibility[y];
				for (size_t x = 0; x < row.size(); ++x)
				{
					if (not row[x])
					{
						continue;
					}

					const Point target{ static_cast<int32>(x), static_cast<int32>(y) };
					const MapObjectType type = ToMapObjectType(m_mapSystem->objectIdAt(target));
					if ((type == MapObjectType::Box) || (type == MapObjectType::Rock) || (type == MapObjectType::Camera))
					{
						candidates << target;
					}
				}
			}

			if (not candidates.isEmpty())
			{
				const size_t index = Random<size_t>(0, candidates.size() - 1);
				const Point target = candidates[index];
				m_mapSystem->removeObjectAt(target);
				m_mapSystem->revealAround(target);
			}
		}
		return true;
	}

	if (cardId == U"yaku")
	{
		if (m_stage)
		{
			m_stage->activateYakuEffect();
		}
		return true;
	}

	return false;
}

void CardEffectController::enqueueTargetedEffect(const String& cardId)
{
	const bool requiresTarget =
		(cardId == U"zen") ||
		(cardId == U"choku") ||
		(cardId == U"kyuu") ||
		(cardId == U"ann") ||
		(cardId == U"kou") ||
		(cardId == U"ha2") ||
		(cardId == U"kou3") ||
		(cardId == U"you") ||
		(cardId == U"kou2") ||
		(cardId == U"kan");

	if (not requiresTarget)
	{
		return;
	}

	if (m_state != EffectState::None)
	{
		m_pendingCardEffects << cardId;
	}
	else
	{
		activateEffect(cardId);
	}
}

void CardEffectController::updateMapTargeting()
{
	updateHoverTarget();

	if (MouseL.down())
	{
		if (m_hoverTarget)
		{
			applyTargetSelection(*m_hoverTarget);
			return;
		}
	}

	if (MouseR.down() || KeyEscape.down())
	{
		clearTargeting();
	}
}

void CardEffectController::drawMapTargeting() const
{
	if ((not m_mapSystem) || (m_state == EffectState::None))
	{
		return;
	}

	const Transformer2D transformer{ Mat3x2::Scale(m_mapSystem->scale()).translated(m_mapSystem->offset()) };
	for (size_t i = 0; i < m_targetOptions.size(); ++i)
	{
		const auto& option = m_targetOptions[i];
		const bool hovered = (m_hoverTarget && (*m_hoverTarget == i));
		const ColorF pathColor = hovered ? ColorF{ 0.9, 0.82, 0.25, 0.45 } : ColorF{ 0.2, 0.6, 0.95, 0.32 };
		const ColorF borderColor = hovered ? ColorF{ 0.98, 0.95, 0.45, 0.85 } : ColorF{ 0.4, 0.85, 1.0, 0.7 };

		for (const auto& tile : option.path)
		{
			RectF rect{ m_mapSystem->gridToWorld(tile), m_mapSystem->tileSize() };
			rect.stretched(-6).draw(pathColor);
		}

		RectF destRect{ m_mapSystem->gridToWorld(option.destination), m_mapSystem->tileSize() };
		destRect.stretched(-6).drawFrame(4, 0, borderColor);
	}
}

bool CardEffectController::isMapTargetState(EffectState state) const
{
	switch (state)
	{
	case EffectState::Zen:
	case EffectState::Choku:
	case EffectState::Kyuu:
	case EffectState::Ann:
	case EffectState::Kou2:
	case EffectState::DestroyObstacle:
	case EffectState::DestroyCamera:
		return true;
	default:
		return false;
	}
}

void CardEffectController::completeMovementSelection(const TargetOption& option)
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearTargeting();
		return;
	}

	if (option.destination != m_player->gridPosition())
	{
		m_player->setGridPosition(option.destination, *m_mapSystem);
	}

	destroyBoxesAlong(option.path);
	revealPath(option.path);
	m_mapSystem->revealAround(m_player->gridPosition());
	clearTargeting();
}

void CardEffectController::handleObstacleRemoval(const TargetOption& option)
{
	if (not m_mapSystem)
	{
		clearTargeting();
		return;
	}

	const Point target = option.selection;
	m_mapSystem->removeObjectAt(target);
	m_mapSystem->revealAround(target);
	clearTargeting();
}

void CardEffectController::handleCameraRemoval(const TargetOption& option)
{
	if (not m_mapSystem)
	{
		clearTargeting();
		return;
	}

	const Point target = option.selection;
	if (ToMapObjectType(m_mapSystem->objectIdAt(target)) == MapObjectType::Camera)
	{
		m_mapSystem->removeObjectAt(target);
		m_mapSystem->revealAround(target);
	}
	clearTargeting();
}

void CardEffectController::cancelTargeting()
{
	clearTargeting(false);
}

void CardEffectController::activateEffect(const String& cardId)
{
	if (cardId == U"zen")
	{
		startZenTargeting();
	}
	else if (cardId == U"choku")
	{
		startChokuTargeting();
	}
	else if (cardId == U"kyuu")
	{
		startKyuuTargeting();
	}
	else if (cardId == U"ann")
	{
		startAnnTargeting();
	}
	else if (cardId == U"kou2")
	{
		startKou2Targeting();
	}
	else if (cardId == U"kan")
	{
		startDestroyObstacleTargeting();
	}
	else if (cardId == U"you")
	{
		beginHandDiscard();
	}
	else if (cardId == U"kou")
	{
		beginDeckSelection(DeckSelectionType::AddToHand);
	}
	else if (cardId == U"ha2")
	{
		beginChoicePrompt(ChoiceType::Ha2, { U"次のカードを2回発動", U"監視カメラを無効化" });
	}
	else if (cardId == U"kou3")
	{
		beginChoicePrompt(ChoiceType::Kou3, { U"山札から選ぶ", U"2マス進む" });
	}
}

void CardEffectController::startZenTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::Zen;

	const Point origin = m_player->gridPosition();
	for (const auto& dir : CardinalDirections)
	{
		const Point target = origin + dir;
		if (canTraverse(target))
		{
			TargetOption option;
			option.selection = target;
			option.destination = target;
			option.path << target;
			m_targetOptions << option;
		}
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::beginHandDiscard()
{
	if (not m_cardSystem)
	{
		clearTargeting();
		return;
	}

	m_cardSystem->drawCards(2);
	m_cardSystem->setInputSuppressed(true);
	m_state = EffectState::HandDiscard;
}

void CardEffectController::updateHandDiscard()
{
	if (not m_cardSystem)
	{
		clearTargeting();
		return;
	}

	if (m_cardSystem->handOrder().isEmpty())
	{
		m_cardSystem->setInputSuppressed(false);
		clearTargeting();
		return;
	}

	if (MouseL.down())
	{
		if (const Optional<size_t> index = m_cardSystem->handCardAtScreenPos(Cursor::PosF()))
		{
			m_cardSystem->discardCardInHand(*index);
			m_cardSystem->setInputSuppressed(false);
			clearTargeting();
			return;
		}
	}
}

void CardEffectController::drawHandDiscard() const
{
	const double panelHeight = 120.0;
	const RectF panel{ 0.0, Scene::Height() - panelHeight, Scene::Width(), panelHeight };
	panel.draw(ColorF{ 0.05, 0.08, 0.12, 0.75 });
	panel.drawFrame(2, 0, ColorF{ 0.92, 0.95, 1.0, 0.25 });

	const String message = U"捨てるカードをクリックしてください";
	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(message).drawAt(panel.center(), ColorF{ 0.96 });
	}
	else
	{
		static const Font fallback{ 32 };
		fallback(message).drawAt(panel.center(), ColorF{ 0.96 });
	}
}

void CardEffectController::beginChoicePrompt(ChoiceType type, const Array<String>& options)
{
	if (m_cardSystem)
	{
		m_cardSystem->setInputSuppressed(true);
	}

	m_choiceType = type;
	m_choiceOptions = options;
	m_choiceOptionRects.clear();
	m_choiceHover.reset();

	const double buttonWidth = 260.0;
	const double buttonHeight = 66.0;
	const double spacing = 28.0;
	const double totalWidth = (buttonWidth * options.size()) + spacing * Max<size_t>(0, options.size() ? options.size() - 1 : 0);
	double cursorX = (Scene::Width() - totalWidth) * 0.5;

	const double modalWidth = 520.0;
	const double modalHeight = 200.0;
	const double modalY = (Scene::Height() - modalHeight) * 0.35;
	const double buttonY = (modalY + modalHeight * 0.55) - (buttonHeight * 0.5);

	for (size_t i = 0; i < options.size(); ++i)
	{
		m_choiceOptionRects << RectF{ cursorX, buttonY, buttonWidth, buttonHeight };
		cursorX += buttonWidth + spacing;
	}

	m_state = (type == ChoiceType::Ha2) ? EffectState::ChoiceHa2 : EffectState::ChoiceKou3;
}

void CardEffectController::updateChoicePrompt()
{
	m_choiceHover.reset();
	const Vec2 cursor = Cursor::PosF();

	for (size_t i = 0; i < m_choiceOptionRects.size(); ++i)
	{
		if (m_choiceOptionRects[i].contains(cursor))
		{
			m_choiceHover = i;
			break;
		}
	}

	if (MouseL.down())
	{
		if (m_choiceHover)
		{
			processChoiceSelection(*m_choiceHover);
			return;
		}
	}

	if (MouseR.down() || KeyEscape.down())
	{
		if (m_cardSystem)
		{
			m_cardSystem->setInputSuppressed(false);
		}
		resetChoiceState();
		clearTargeting();
	}
}

void CardEffectController::drawChoicePrompt() const
{
	RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.02, 0.04, 0.08, 0.55 });

	const double modalWidth = 520.0;
	const double modalHeight = 200.0;
	const RectF modal{ (Scene::Width() - modalWidth) * 0.5, (Scene::Height() - modalHeight) * 0.35, modalWidth, modalHeight };
	modal.rounded(18).draw(ColorF{ 0.08, 0.1, 0.16, 0.92 });
	modal.rounded(18).drawFrame(2, 0, ColorF{ 0.95, 0.92, 0.8, 0.28 });

	const String title = (m_choiceType == ChoiceType::Ha2)
		? U"効果を選択してください"
		: U"進むか、カードを選びますか？";

	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(title).drawAt(modal.center().x, modal.y + 40, ColorF{ 0.96 });
	}
	else
	{
		static const Font fallback{ 32 };
		fallback(title).drawAt(modal.center().x, modal.y + 40, ColorF{ 0.96 });
	}

	for (size_t i = 0; i < m_choiceOptionRects.size(); ++i)
	{
		RectF rect = m_choiceOptionRects[i];

		const bool hover = (m_choiceHover && (*m_choiceHover == i));
		const ColorF base = hover ? ColorF{ 0.36, 0.55, 0.95, 0.95 } : ColorF{ 0.24, 0.32, 0.52, 0.9 };
		rect.rounded(14).draw(base);
		rect.rounded(14).drawFrame(2, 0, ColorF{ 1.0, 1.0, 1.0, hover ? 0.55 : 0.35 });

		const String& label = m_choiceOptions[i];
		if (FontAsset::IsRegistered(U"MisakiFont"))
		{
			FontAsset(U"MisakiFont")(label).drawAt(rect.center(), ColorF{ 0.96 });
		}
		else
		{
			static const Font fallback{ 28 };
			fallback(label).drawAt(rect.center(), ColorF{ 0.96 });
		}
	}
}

void CardEffectController::resetChoiceState()
{
	m_choiceOptions.clear();
	m_choiceOptionRects.clear();
	m_choiceHover.reset();
	m_choiceType = ChoiceType::None;
}

void CardEffectController::processChoiceSelection(size_t index)
{
	if (index >= m_choiceOptions.size())
	{
		return;
	}

	if (m_choiceType == ChoiceType::Ha2)
	{
		if (index == 0)
		{
			m_nextCardRepeats += 1;
			resetChoiceState();
			if (m_cardSystem)
			{
				m_cardSystem->setInputSuppressed(false);
			}
			clearTargeting();
		}
		else if (index == 1)
		{
			resetChoiceState();
			if (m_cardSystem)
			{
				m_cardSystem->setInputSuppressed(false);
			}
			startDestroyCameraTargeting();
		}
	}
	else if (m_choiceType == ChoiceType::Kou3)
	{
		if (index == 0)
		{
			resetChoiceState();
			if (m_cardSystem)
			{
				m_cardSystem->setInputSuppressed(false);
			}
			beginDeckSelection(DeckSelectionType::AddToHand);
		}
		else if (index == 1)
		{
			resetChoiceState();
			if (m_cardSystem)
			{
				m_cardSystem->setInputSuppressed(false);
			}
			startKou2Targeting();
		}
	}
}

void CardEffectController::beginDeckSelection(DeckSelectionType type)
{
	if (not m_cardSystem)
	{
		clearTargeting();
		return;
	}

	const auto& deckOrder = m_cardSystem->deckOrder();
	if (deckOrder.isEmpty())
	{
		m_cardSystem->setInputSuppressed(false);
		clearTargeting();
		return;
	}

	m_deckSelectionType = type;
	m_deckSelectionIndices = deckOrder;
	m_deckSelectionIndices.reverse();

	static constexpr size_t MaxDeckDisplayCount = 12;
	m_deckSelectionLimited = (m_deckSelectionIndices.size() > MaxDeckDisplayCount);
	if (m_deckSelectionLimited)
	{
		m_deckSelectionIndices.resize(MaxDeckDisplayCount);
	}

	m_deckSelectionRects.clear();
	m_deckSelectionHover.reset();

	const double cardWidth = 120.0;
	const double cardHeight = 168.0;
	const double spacingX = 26.0;
	const double spacingY = 30.0;
	const size_t columns = Max<size_t>(1, Min<size_t>(4, m_deckSelectionIndices.size()));
	const size_t rows = (m_deckSelectionIndices.size() + columns - 1) / columns;

	const double totalWidth = columns * cardWidth + (columns - 1) * spacingX;
	const double totalHeight = rows * cardHeight + (rows - 1) * spacingY;
	const double startX = (Scene::Width() - totalWidth) * 0.5;
	const double startY = (Scene::Height() * 0.5) - (totalHeight * 0.5);

	for (size_t i = 0; i < m_deckSelectionIndices.size(); ++i)
	{
		const size_t col = i % columns;
		const size_t row = i / columns;
		const double x = startX + col * (cardWidth + spacingX);
		const double y = startY + row * (cardHeight + spacingY);
		m_deckSelectionRects << RectF{ x, y, cardWidth, cardHeight };
	}

	m_state = EffectState::DeckSelect;
	m_cardSystem->setInputSuppressed(true);
}

void CardEffectController::updateDeckSelection()
{
	if (not m_cardSystem)
	{
		clearTargeting();
		return;
	}

	if (m_deckSelectionIndices.isEmpty())
	{
		m_cardSystem->setInputSuppressed(false);
		clearTargeting();
		return;
	}

	m_deckSelectionHover.reset();
	const Vec2 cursor = Cursor::PosF();

	for (size_t i = 0; i < m_deckSelectionRects.size(); ++i)
	{
		if (m_deckSelectionRects[i].contains(cursor))
		{
			m_deckSelectionHover = i;
			break;
		}
	}

	if (MouseL.down())
	{
		if (m_deckSelectionHover && (*m_deckSelectionHover < m_deckSelectionIndices.size()))
		{
			const size_t deckIndex = m_deckSelectionIndices[*m_deckSelectionHover];
			bool success = false;

			if (m_deckSelectionType == DeckSelectionType::AddToHand)
			{
				success = m_cardSystem->addDeckCardToHand(deckIndex);
			}

			m_cardSystem->setInputSuppressed(false);
			clearTargeting();

			if (success)
			{
				return;
			}
		}
	}

	if (MouseR.down() || KeyEscape.down())
	{
		m_cardSystem->setInputSuppressed(false);
		clearTargeting();
	}
}

void CardEffectController::drawDeckSelection() const
{
	RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.02, 0.04, 0.08, 0.55 });

	const double modalWidth = Scene::Width() * 0.8;
	const double modalHeight = Scene::Height() * 0.7;
	const RectF modal{ (Scene::Width() - modalWidth) * 0.5, (Scene::Height() - modalHeight) * 0.5, modalWidth, modalHeight };
	modal.rounded(18).draw(ColorF{ 0.08, 0.1, 0.16, 0.92 });
	modal.rounded(18).drawFrame(2, 0, ColorF{ 0.95, 0.92, 0.8, 0.28 });

	const String title = U"手札に加えるカードを選択してください";
	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(title).drawAt(modal.center().x, modal.y + 36, ColorF{ 0.96 });
	}
	else
	{
		static const Font fallback{ 32 };
		fallback(title).drawAt(modal.center().x, modal.y + 36, ColorF{ 0.96 });
	}

	for (size_t i = 0; i < m_deckSelectionIndices.size() && i < m_deckSelectionRects.size(); ++i)
	{
		const size_t deckIndex = m_deckSelectionIndices[i];
		RectF rect = m_deckSelectionRects[i];

		const bool hover = (m_deckSelectionHover && (*m_deckSelectionHover == i));
		const ColorF frameColor = hover ? ColorF{ 0.42, 0.65, 0.98, 0.9 } : ColorF{ 0.3, 0.36, 0.52, 0.75 };

		const CardInstance* instance = m_cardSystem->instanceAt(deckIndex);
		const Texture* texture = nullptr;
		if (instance && instance->definition)
		{
			texture = m_cardSystem->textureForCard(instance->definition->id);
		}

		if (texture)
		{
			texture->resized(rect.size).draw(rect.pos);
		}
		else
		{
			rect.draw(ColorF{ 0.24, 0.28, 0.36, 0.85 });
		}

		rect.drawFrame(4, 0, frameColor);

	}

	if (m_deckSelectionLimited)
	{
		const String note = U"(上から12枚までを表示しています)";
		if (FontAsset::IsRegistered(U"MisakiFont"))
		{
			FontAsset(U"MisakiFont")(note).drawAt(modal.center().x, modal.y + modalHeight - 40, ColorF{ 0.78 });
		}
		else
		{
			static const Font fallback{ 20 };
			fallback(note).drawAt(modal.center().x, modal.y + modalHeight - 40, ColorF{ 0.78 });
		}
	}
}

void CardEffectController::startKyuuTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::Kyuu;

	const Point origin = m_player->gridPosition();
	for (const auto& dir : CardinalDirections)
	{
		const Point firstStep = origin + dir;
		const Point secondStep = firstStep + dir;

		if (not canTraverse(firstStep) || not canTraverse(secondStep))
		{
			continue;
		}

		TargetOption option;
		option.selection = secondStep;
		option.destination = secondStep;
		option.path << firstStep;
		option.path << secondStep;
		m_targetOptions << option;
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::startAnnTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::Ann;

	const Point origin = m_player->gridPosition();
	const Array<Array<bool>> visibility = m_mapSystem->visibilitySnapshot();

	for (size_t y = 0; y < visibility.size(); ++y)
	{
		const auto& row = visibility[y];
		for (size_t x = 0; x < row.size(); ++x)
		{
			if (not row[x])
			{
				continue;
			}

			const Point target{ static_cast<int32>(x), static_cast<int32>(y) };
			if (target == origin)
			{
				continue;
			}

			if (not m_mapSystem->canEnter(target))
			{
				continue;
			}

			TargetOption option;
			option.selection = target;
			option.destination = target;
			option.path << target;
			m_targetOptions << option;
		}
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::startKou2Targeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::Kou2;

	const Point origin = m_player->gridPosition();

	for (const auto& dir1 : CardinalDirections)
	{
		const Point first = origin + dir1;
		if (not canTraverse(first))
		{
			continue;
		}

		for (const auto& dir2 : CardinalDirections)
		{
			const Point second = first + dir2;
			if (not canTraverse(second))
			{
				continue;
			}

			if (second == origin)
			{
				continue;
			}

			TargetOption option;
			option.selection = second;
			option.destination = second;
			option.path << first;
			option.path << second;
			m_targetOptions << option;
		}
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::startDestroyObstacleTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::DestroyObstacle;

	const Array<Array<bool>> visibility = m_mapSystem->visibilitySnapshot();

	for (size_t y = 0; y < visibility.size(); ++y)
	{
		const auto& row = visibility[y];
		for (size_t x = 0; x < row.size(); ++x)
		{
			if (not row[x])
			{
				continue;
			}

			const Point tile{ static_cast<int32>(x), static_cast<int32>(y) };
			const MapObjectType type = ToMapObjectType(m_mapSystem->objectIdAt(tile));
			if ((type != MapObjectType::Box) && (type != MapObjectType::Rock))
			{
				continue;
			}

			TargetOption option;
			option.selection = tile;
			option.destination = m_player->gridPosition();
			option.path << tile;
			m_targetOptions << option;
		}
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::startDestroyCameraTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::DestroyCamera;

	const Array<Array<bool>> visibility = m_mapSystem->visibilitySnapshot();

	for (size_t y = 0; y < visibility.size(); ++y)
	{
		const auto& row = visibility[y];
		for (size_t x = 0; x < row.size(); ++x)
		{
			if (not row[x])
			{
				continue;
			}

			const Point tile{ static_cast<int32>(x), static_cast<int32>(y) };
			const MapObjectType type = ToMapObjectType(m_mapSystem->objectIdAt(tile));
			if (type != MapObjectType::Camera)
			{
				continue;
			}

			TargetOption option;
			option.selection = tile;
			option.destination = m_player->gridPosition();
			option.path << tile;
			m_targetOptions << option;
		}
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::startChokuTargeting()
{
	if ((not m_mapSystem) || (not m_player))
	{
		clearAllEffects();
		return;
	}

	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_state = EffectState::Choku;

	const Point origin = m_player->gridPosition();
	for (const auto& dir : CardinalDirections)
	{
		Array<Point> path;
		Point current = origin;
		Point next = current + dir;

		while (canTraverse(next))
		{
			path << next;
			current = next;
			next = current + dir;
		}

		if (path.isEmpty())
		{
			continue;
		}

		TargetOption option;
		option.selection = path.front();
		option.destination = path.back();
		option.path = path;
		m_targetOptions << option;
	}

	if (m_targetOptions.isEmpty())
	{
		clearTargeting();
	}
}

void CardEffectController::clearTargeting(bool processQueue)
{
	if (m_cardSystem)
	{
		m_cardSystem->setInputSuppressed(false);
	}

	m_state = EffectState::None;
	m_targetOptions.clear();
	m_hoverTarget.reset();
	m_deckSelectionType = DeckSelectionType::None;
	m_deckSelectionIndices.clear();
	m_deckSelectionRects.clear();
	m_deckSelectionHover.reset();
	m_deckSelectionLimited = false;
	m_choiceOptions.clear();
	m_choiceOptionRects.clear();
	m_choiceHover.reset();
	m_choiceType = ChoiceType::None;

	if (processQueue)
	{
		tryActivateNextCard();
	}
}

void CardEffectController::tryActivateNextCard()
{
	if (m_state != EffectState::None)
	{
		return;
	}

	if (m_pendingCardEffects.isEmpty())
	{
		return;
	}

	const String next = m_pendingCardEffects.front();
	m_pendingCardEffects.erase(m_pendingCardEffects.begin());
	activateEffect(next);
}

void CardEffectController::updateHoverTarget()
{
	m_hoverTarget.reset();

	const Optional<Point> gridPos = screenToGrid(Cursor::PosF());
	if (not gridPos)
	{
		return;
	}

	for (size_t i = 0; i < m_targetOptions.size(); ++i)
	{
		if (optionContains(m_targetOptions[i], *gridPos))
		{
			m_hoverTarget = i;
			break;
		}
	}
}

Optional<Point> CardEffectController::screenToGrid(const Vec2& screenPos) const
{
	if (not m_mapSystem)
	{
		return none;
	}

	const double scale = m_mapSystem->scale();
	if (scale <= 0.0)
	{
		return none;
	}

	const Vec2 local = (screenPos - m_mapSystem->offset()) / scale;
	const Vec2 relative = local - m_mapSystem->origin();
	if ((relative.x < 0.0) || (relative.y < 0.0))
	{
		return none;
	}

	const Vec2 tile = m_mapSystem->tileSize();
	if ((tile.x <= 0.0) || (tile.y <= 0.0))
	{
		return none;
	}

	const int32 gridX = static_cast<int32>(relative.x / tile.x);
	const int32 gridY = static_cast<int32>(relative.y / tile.y);
	const Point gridPos{ gridX, gridY };

	if (not m_mapSystem->isInBounds(gridPos))
	{
		return none;
	}

	return gridPos;
}

bool CardEffectController::optionContains(const TargetOption& option, const Point& gridPos) const
{
	for (const auto& tile : option.path)
	{
		if (tile == gridPos)
		{
			return true;
		}
	}
	return false;
}

void CardEffectController::applyTargetSelection(size_t optionIndex)
{
	if ((optionIndex >= m_targetOptions.size()) || (not m_mapSystem) || (not m_player))
	{
		return;
	}

	const auto& option = m_targetOptions[optionIndex];

	switch (m_state)
	{
	case EffectState::Zen:
	case EffectState::Choku:
	case EffectState::Kyuu:
	case EffectState::Ann:
	case EffectState::Kou2:
		completeMovementSelection(option);
		break;
	case EffectState::DestroyObstacle:
		handleObstacleRemoval(option);
		break;
	case EffectState::DestroyCamera:
		handleCameraRemoval(option);
		break;
	default:
		clearTargeting();
		break;
	}
}

void CardEffectController::revealPath(const Array<Point>& path) const
{
	if (not m_mapSystem)
	{
		return;
	}

	for (const auto& tile : path)
	{
		m_mapSystem->revealAround(tile);
	}
}

bool CardEffectController::canTraverse(const Point& gridPos) const
{
	if (not m_mapSystem)
	{
		return false;
	}

	if (m_mapSystem->canEnter(gridPos))
	{
		return true;
	}

	return (m_canBreakBoxesThisTurn && isBox(gridPos));
}

void CardEffectController::destroyBoxesAlong(const Array<Point>& path)
{
	if ((not m_mapSystem) || (not m_canBreakBoxesThisTurn))
	{
		return;
	}

	for (const auto& tile : path)
	{
		if (isBox(tile))
		{
			m_mapSystem->removeObjectAt(tile);
			m_mapSystem->revealAround(tile);
		}
	}
}

bool CardEffectController::isBox(const Point& gridPos) const
{
	if (not m_mapSystem)
	{
		return false;
	}

	return (ToMapObjectType(m_mapSystem->objectIdAt(gridPos)) == MapObjectType::Box);
}
