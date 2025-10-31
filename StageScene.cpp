#include "StageScene.h"
#include <utility>
#include <algorithm>

StageScene::StageScene(const InitData& init, StageConfig config)
	: IScene{ init }
	, m_config{ std::move(config) }
{
	Scene::SetResizeMode(ResizeMode::Keep);

	const Vec2 virtualSize{ 1280, 720 };
	const String mapPath = U"resources/txt/Stage{}"_fmt(m_config.stageIndex) + U".json";
	String backgroundPath = U"resources/texture/field/background.png";

	if (const JSON stageJson = JSON::Load(mapPath))
	{
		const JSON backgroundNode = stageJson[U"background"];
		if (const auto opt = backgroundNode.getOpt<String>())
		{
			if (not opt->isEmpty())
			{
				backgroundPath = *opt;
			}
		}
	}

	try
	{
		m_backgroundTexture = Texture{ backgroundPath, TextureDesc::Mipped };
	}
	catch (...)
	{
		m_backgroundTexture = Texture{};
	}

	try
	{
		m_kanjiSlotTexture = Texture{ U"resources/texture/kanji/kanjislot.png", TextureDesc::Mipped };
	}
	catch (...)
	{
		m_kanjiSlotTexture = Texture{};
	}

	const Array<std::pair<String, String>> kanjiTexturePairs{
		{ U"進", U"resources/texture/kanji/sin1.png" },
		{ U"心", U"resources/texture/kanji/sin2.png" },
		{ U"新", U"resources/texture/kanji/sin3.png" },
		{ U"神", U"resources/texture/kanji/sin4.png" },
		{ U"信", U"resources/texture/kanji/sin5.png" },
		{ U"侵", U"resources/texture/kanji/sin6.png" }
	};

	for (const auto& [kanjiId, texturePath] : kanjiTexturePairs)
	{
		try
		{
			Texture texture{ texturePath, TextureDesc::Mipped };
			if (texture)
			{
				m_kanjiTextures[kanjiId] = std::move(texture);
			}
		}
		catch (...)
		{
		}
	}

	if (not m_mapSystem.loadFromJSON(mapPath, virtualSize))
	{
		throw Error{ U"Failed {}"_fmt(mapPath) };
	}

	m_mapSystem.setFogOfWarEnabled(true);

	if (not m_player.init(U"resources/texture/player/player.png", m_mapSystem, Point{ 0, 0 }))
	{
		throw Error{ U"Failed texture" };
	}

	m_mapSystem.revealAround(m_player.gridPosition());

	const Array<String> starterIds{ U"zen", U"choku", U"ka" };
	CardHandConfig configHand;
	configHand.virtualSize = virtualSize;
	configHand.margin = 32.0;
	configHand.gap = 18.0;
	configHand.columns = 4;
	configHand.activationOffset = 40.0;

	if (not m_cardSystem.initialize(U"resources/txt/cards.json", starterIds, configHand))
	{
		throw Error{ U"Failed card system" };
	}

	m_cardSystem.setCardValidationCallback([this](const String& cardId)
	{
		return isCardPlayable(cardId);
	});

	m_actionsUsed = 0;
	m_showClearModal = false;
	m_resultRecorded = false;
	m_treasureSelection = TreasureSelection{};
	m_treasureHover.reset();
	m_cardEffects.initialize(&m_mapSystem, &m_player, &m_cardSystem, this);
	m_pendingKanjiReward.reset();
	m_pendingRewardCards.clear();

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

	m_maxActions = 5;
	m_actionsRemaining = m_maxActions;
	m_promoteShinToKami = false;
	m_fullVisibilityActive = false;
	m_fullVisibilityBackup.clear();
	m_ignoreTileDebuffsThisTurn = false;

	auto& data = getData();
	for (const auto& info : KanjiSystem::allKanji())
	{
		if ((info.id == U"進") || (info.id == U"神") || (not data.kanjiOwned.contains(info.id)))
		{
			continue;
		}

		for (const auto& cardId : info.cardIds)
		{
			data.unlockedCards.insert(cardId);
		}
	}

	for (const auto& cardId : data.unlockedCards)
	{
		m_cardSystem.addCardToDeck(cardId);
	}

	m_previousPlayerGrid = m_player.gridPosition();
}

void StageScene::activateYakuEffect()
{
	m_promoteShinToKami = true;
	m_cardSystem.addCardToDeck(U"soku");
}

void StageScene::healActionsToFull()
{
	m_actionsRemaining = m_maxActions;
}

void StageScene::activateFullMapVision()
{
	if (not m_mapSystem.fogOfWarEnabled())
	{
		return;
	}

	if (not m_fullVisibilityActive)
	{
		m_fullVisibilityBackup = m_mapSystem.visibilitySnapshot();
		m_mapSystem.revealAll();
		m_fullVisibilityActive = true;
	}
}

void StageScene::deactivateFullMapVision()
{
	if (not m_fullVisibilityActive)
	{
		return;
	}

	if (m_mapSystem.fogOfWarEnabled() && (not m_fullVisibilityBackup.isEmpty()))
	{
		m_mapSystem.applyVisibility(m_fullVisibilityBackup);
	}

	m_fullVisibilityBackup.clear();
	m_fullVisibilityActive = false;
}

void StageScene::activateDebuffImmunity()
{
	m_ignoreTileDebuffsThisTurn = true;
}

void StageScene::clearDebuffImmunity()
{
	m_ignoreTileDebuffsThisTurn = false;
}

void StageScene::update()
{
	m_mapSystem.update();
	m_player.update(m_mapSystem);

	if (m_showClearModal)
	{
		handleClearModalInput();
		return;
	}

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
		deactivateFullMapVision();
		clearDebuffImmunity();
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

void StageScene::draw() const
{
	Scene::SetBackground(ColorF{ 0.0 });

	if (m_backgroundTexture)
	{
		const Size texSize = m_backgroundTexture.size();
		if ((texSize.x > 0) && (texSize.y > 0))
		{
			const double scaleX = Scene::Width() / static_cast<double>(texSize.x);
			const double scaleY = Scene::Height() / static_cast<double>(texSize.y);
			const double scale = Max(scaleX, scaleY);
			m_backgroundTexture.scaled(scale).drawAt(Scene::Center());
		}
		else
		{
			RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.12, 0.12, 0.16 });
		}
	}
	else
	{
		RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.12, 0.12, 0.16 });
	}

	m_mapSystem.draw();
	m_player.draw(m_mapSystem);
	const bool overlayOpen = m_cardSystem.isOverlayOpen();
	if (overlayOpen)
	{
		drawKanjiPanel();
	}
	m_cardSystem.draw();
	if (not overlayOpen)
	{
		drawKanjiPanel();
	}
	drawTreasureSelection();
	if (not m_gameOver)
	{
		m_cardEffects.draw();
	}
	drawClearModal();
	drawActionCounter();
}

void StageScene::drawActionCounter() const
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

void StageScene::onEndTurn()
{
	if (m_gameOver)
	{
		return;
	}

	m_cardEffects.clearAllEffects();
	deactivateFullMapVision();
	clearDebuffImmunity();
	modifyActionPoints(-1);
}

void StageScene::modifyActionPoints(int32 delta)
{
	if ((delta == 0) || m_gameOver)
	{
		return;
	}

	if (delta < 0)
	{
		m_actionsUsed += static_cast<int32>(-delta);
	}

	const int64 updated = static_cast<int64>(m_actionsRemaining) + static_cast<int64>(delta);
	const int64 clamped = Clamp<int64>(updated, 0, static_cast<int64>(m_maxActions));
	m_actionsRemaining = static_cast<int32>(clamped);

	if (m_actionsRemaining <= 0)
	{
		m_actionsRemaining = 0;
		handleGameOver();
	}
}

void StageScene::handleGameOver()
{
	if (m_gameOver)
	{
		return;
	}

	m_gameOver = true;
	m_cardEffects.clearAllEffects();
	deactivateFullMapVision();
	clearDebuffImmunity();
	changeScene(State::Ending);
}

void StageScene::handleMovement(const Point& previous, const Point& current)
{
	handleTileInteractions(previous, current);
}

void StageScene::handleTileInteractions(const Point& previous, const Point& current)
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
		if (m_ignoreTileDebuffsThisTurn)
		{
			m_mapSystem.removeObjectAt(current);
			return;
		}
		m_player.setGridPosition(previous, m_mapSystem);
		m_mapSystem.revealAround(m_player.gridPosition());
		return;
	case MapObjectType::CameraWatch:
		if (m_ignoreTileDebuffsThisTurn)
		{
			return;
		}
		m_player.setGridPosition(previous, m_mapSystem);
		m_mapSystem.revealAround(m_player.gridPosition());
		return;
	case MapObjectType::Pillar:
		handleGoalReached();
		return;
	}
}

void StageScene::beginTreasureSelection(const Point& location)
{
	Array<String> pool = playableCardPool();
	if (pool.isEmpty())
	{
		m_mapSystem.removeObjectAt(location);
		return;
	}

	pool.shuffle();
	if (pool.size() > 3)
	{
		pool.resize(3);
	}

	Array<String> options = pool;

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

	m_cardEffects.cancelTargeting();
}

void StageScene::handleTreasureSelectionInput()
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

void StageScene::drawTreasureSelection() const
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

		//if (FontAsset::IsRegistered(U"MisakiFont"))
		//{
		//	if (const CardDefinition* def = m_cardSystem.findCardDefinition(m_treasureSelection.cardIds[i]))
		//	{
		//		FontAsset(U"MisakiFont")(def->name).drawAt(rect.center().movedBy(0, rect.h * 0.58), ColorF{ 0.95 });
		//	}
		//}
	}
}

void StageScene::completeTreasureSelection(size_t optionIndex)
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

void StageScene::handleGoalReached()
{
	if (m_showClearModal)
	{
		return;
	}

	m_gameOver = true;
	m_cardEffects.clearAllEffects();
	deactivateFullMapVision();
	clearDebuffImmunity();
	m_showClearModal = true;
	m_treasureSelection = TreasureSelection{};
	m_treasureHover.reset();
	prepareKanjiReward();

	if (not m_resultRecorded)
	{
		auto& data = getData();
		data.totalActions += m_actionsUsed;
		m_resultRecorded = true;
	}
}

StageScene::ClearModalLayout StageScene::makeClearModalLayout() const
{
	ClearModalLayout layout;
	const Vec2 modalSize{ 760, 360 };
	const Vec2 modalPos{ (Scene::Width() - modalSize.x) * 0.5, (Scene::Height() - modalSize.y) * 0.5 };
	layout.modal = RectF{ modalPos, modalSize };

	const double padding = 32.0;
	const double sectionSpacing = 32.0;
	const double headerHeight = 88.0;
	const double sectionWidth = (modalSize.x - padding * 2.0 - sectionSpacing) * 0.5;
	const double sectionHeight = modalSize.y - headerHeight - padding - 72.0;
	const double sectionY = modalPos.y + headerHeight;
	const double stageX = modalPos.x + padding;
	const double totalX = stageX + sectionWidth + sectionSpacing;
	layout.stageRect = RectF{ Vec2{ stageX, sectionY }, Vec2{ sectionWidth, sectionHeight } };
	layout.totalRect = RectF{ Vec2{ totalX, sectionY }, Vec2{ sectionWidth, sectionHeight } };

	const Vec2 buttonSize{ 220, 56 };
	const double buttonSpacing = 18.0;
	const double buttonY = modalPos.y + modalSize.y - padding - buttonSize.y;
	const double buttonsRight = modalPos.x + modalSize.x - padding;
	const double nextButtonX = buttonsRight - buttonSize.x;
	const double titleButtonX = nextButtonX - buttonSpacing - buttonSize.x;
	layout.titleButton = RectF{ Vec2{ titleButtonX, buttonY }, buttonSize };
	layout.nextButton = RectF{ Vec2{ nextButtonX, buttonY }, buttonSize };

	return layout;
}

void StageScene::handleClearModalInput()
{
	if (not m_showClearModal)
	{
		return;
	}

	const auto layout = makeClearModalLayout();

	const bool hasNext = m_config.nextState.has_value();
	if (hasNext && (layout.nextButton.leftClicked() || KeyEnter.down()))
	{
		changeScene(*m_config.nextState);
		return;
	}

	if (layout.titleButton.leftClicked() || KeyEscape.down())
	{
		changeScene(State::Title);
		return;
	}
}

void StageScene::drawClearModal() const
{
	if (not m_showClearModal)
	{
		return;
	}

	const auto layout = makeClearModalLayout();

	RectF{ 0, 0, Scene::Width(), Scene::Height() }.draw(ColorF{ 0.02, 0.03, 0.05, 0.72 });
	layout.modal.rounded(18).draw(ColorF{ 0.1, 0.12, 0.18, 0.96 });
	layout.modal.rounded(18).drawFrame(3, 0, ColorF{ 0.52, 0.62, 0.95, 0.85 });

	const Vec2 titlePos = layout.modal.pos.movedBy(34, 32);
	const String clearText = U"ステージ{} クリア！"_fmt(m_config.stageIndex);
	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(clearText).draw(titlePos, ColorF{ 0.95 });
	}
	else
	{
		m_clearCountFont(clearText).draw(titlePos, ColorF{ 0.95 });
	}

	const ColorF panelColor{ 0.14, 0.16, 0.24, 0.7 };
	const ColorF panelFrame{ 0.48, 0.55, 0.85, 0.75 };
	layout.stageRect.rounded(12).draw(panelColor);
	layout.stageRect.rounded(12).drawFrame(2, 0, panelFrame);
	layout.totalRect.rounded(12).draw(panelColor);
	layout.totalRect.rounded(12).drawFrame(2, 0, panelFrame);

	const Vec2 stageLabelCenter = layout.stageRect.center().movedBy(0, -layout.stageRect.h * 0.28);
	const Vec2 stageValueCenter = layout.stageRect.center().movedBy(0, layout.stageRect.h * 0.12);
	const Vec2 totalLabelCenter = layout.totalRect.center().movedBy(0, -layout.totalRect.h * 0.28);
	const Vec2 totalValueCenter = layout.totalRect.center().movedBy(0, layout.totalRect.h * 0.12);

	if (FontAsset::IsRegistered(U"MisakiFont"))
	{
		FontAsset(U"MisakiFont")(U"今回の行動数").drawAt(stageLabelCenter, ColorF{ 0.9 });
		FontAsset(U"MisakiFont")(U"累計行動数").drawAt(totalLabelCenter, ColorF{ 0.9 });
	}

	m_clearCountFont(Format(m_actionsUsed)).drawAt(stageValueCenter, ColorF{ 0.98 });
	m_clearCountFont(Format(totalActionsTaken())).drawAt(totalValueCenter, ColorF{ 0.98 });

	const auto drawButton = [&](const RectF& rect, const String& text, bool primary, bool enabled)
	{
		const bool hovered = enabled && rect.mouseOver();
		const ColorF baseColor = primary ? ColorF{ 0.34, 0.55, 0.95, 0.92 } : ColorF{ 0.28, 0.32, 0.48, 0.92 };
		const ColorF hoverColor = primary ? ColorF{ 0.45, 0.68, 1.0, 0.96 } : ColorF{ 0.38, 0.42, 0.58, 0.96 };
		const ColorF fill = hovered ? hoverColor : baseColor;
		rect.rounded(14).draw(fill);
		rect.rounded(14).drawFrame(2, 0, ColorF{ 1.0, 1.0, 1.0, 0.28 });

		const ColorF textColor = enabled ? ColorF{ 0.98 } : ColorF{ 0.75 };
		if (FontAsset::IsRegistered(U"MisakiFont"))
		{
			FontAsset(U"MisakiFont")(text).drawAt(rect.center(), textColor);
		}
		else
		{
			m_clearCountFont(text).drawAt(rect.center(), textColor);
		}
	};

	const bool hasNext = m_config.nextState.has_value();
	drawButton(layout.titleButton, U"タイトルへ戻る", false, true);

	if (hasNext)
	{
		drawButton(layout.nextButton, U"次のステージへ", true, true);
	}
	else
	{
		drawButton(layout.nextButton, U"ステージ終了", true, false);
	}

	if (m_pendingKanjiReward)
	{
		const Vec2 messagePos{ layout.modal.pos.x + 34, layout.nextButton.y + 100 };
		const Vec2 detailPos = messagePos.movedBy(0, 36);

		const String message = U"新しいシン「{}」を獲得しました！"_fmt(*m_pendingKanjiReward);
		const ColorF messageColor{ 0.98, 0.91, 0.45 };
		const ColorF detailColor{ 0.9 };

		if (FontAsset::IsRegistered(U"MisakiFont"))
		{
			FontAsset(U"MisakiFont")(message).draw(messagePos, messageColor);

			if (not m_pendingRewardCards.isEmpty())
			{
				String cardsLine = U"対応カード: ";
				for (size_t i = 0; i < m_pendingRewardCards.size(); ++i)
				{
					if (i > 0)
					{
						cardsLine += U" / ";
					}
					cardsLine += m_pendingRewardCards[i];
				}
				FontAsset(U"MisakiFont")(cardsLine).draw(detailPos, detailColor);
			}
		}
		else
		{
			m_clearCountFont(message).draw(messagePos, messageColor);
		}
	}
}

int32 StageScene::totalActionsTaken() const
{
	return getData().totalActions;
}
bool StageScene::isCardPlayable(const String& cardId) const
{
	const auto& data = getData();
	const auto& requirements = KanjiSystem::requirementsForCard(cardId);
	const String shimKanji = U"\u795e"; // 神
	const String shinKanji = U"\u65b0"; // 新

	for (const auto& kanji : requirements)
	{
		if (data.kanjiOwned.contains(kanji))
		{
			continue;
		}

		const bool promotedMatch = (m_promoteShinToKami && (kanji == shimKanji) && data.kanjiOwned.contains(shinKanji));
		if (promotedMatch)
		{
			continue;
		}

		return false;
	}

	return true;
}

Array<String> StageScene::playableCardPool() const
{
	Array<String> pool;
	HashSet<String> seen;
	for (const auto& cardId : m_cardSystem.allCardIds())
	{
		if (seen.contains(cardId))
		{
			continue;
		}

		if (isCardPlayable(cardId))
		{
			pool << cardId;
			seen.insert(cardId);
		}
	}

	return pool;
}

void StageScene::prepareKanjiReward()
{
	m_pendingKanjiReward.reset();
	m_pendingRewardCards.clear();

	if (not shouldGrantKanjiReward())
	{
		return;
	}

	auto& data = getData();
	if (data.kanjiRewardStagesClaimed.contains(m_config.stageIndex))
	{
		return;
	}

	Array<String> candidates;
	for (const auto& info : KanjiSystem::allKanji())
	{
		if ((info.id == U"神") || data.kanjiOwned.contains(info.id))
		{
			continue;
		}

		candidates << info.id;
	}

	if (candidates.isEmpty())
	{
		return;
	}

	const size_t index = static_cast<size_t>(Random<int32>(0, static_cast<int32>(candidates.size() - 1)));
	const String reward = candidates[index];
	data.kanjiOwned.insert(reward);
	data.kanjiRewardStagesClaimed.insert(m_config.stageIndex);
	m_pendingKanjiReward = reward;

	if (const KanjiInfo* info = KanjiSystem::findKanji(reward))
	{
		m_pendingRewardCards = info->cardIds;
		unlockCardsForKanji(*info);
	}

}

bool StageScene::shouldGrantKanjiReward() const
{
	return (m_config.stageIndex == 1) || (m_config.stageIndex == 3);
}

void StageScene::unlockCardsForKanji(const KanjiInfo& info)
{
	auto& data = getData();

	if (info.id == U"神")
	{
		auto removeAllCopies = [&](const String& cardId)
		{
			data.unlockedCards.erase(cardId);
			while (m_cardSystem.removeCardFromDeck(cardId))
			{
			}
		};

		removeAllCopies(U"dou");
		removeAllCopies(U"pin");
		removeAllCopies(U"yaku");

		auto ensureCard = [&](const String& cardId)
		{
			if (data.unlockedCards.insert(cardId).second)
			{
				m_cardSystem.addCardToDeck(cardId);
			}
		};

		ensureCard(U"dou2");
		ensureCard(U"pin2");
		ensureCard(U"soku");
		return;
	}

	for (const auto& cardId : info.cardIds)
	{
		const auto result = data.unlockedCards.insert(cardId);
		if (result.second)
		{
			m_cardSystem.addCardToDeck(cardId);
		}
	}
}

void StageScene::drawKanjiPanel() const
{
	static const Array<String> orderedKanji{
		U"進", U"心", U"新", U"神", U"信", U"侵"
	};

	const auto& data = getData();
	const RectF deckRect = m_cardSystem.deckButtonRect();
	if ((deckRect.w <= 0.0) || (deckRect.h <= 0.0))
	{
		return;
	}

	Array<String> ownedOrdered;
	ownedOrdered.reserve(orderedKanji.size());
	for (const auto& kanji : orderedKanji)
	{
		if (data.kanjiOwned.contains(kanji))
		{
			ownedOrdered << kanji;
			if (ownedOrdered.size() >= 3)
			{
				break;
			}
		}
	}

	const size_t slotCount = 3;
	if (slotCount == 0)
	{
		return;
	}

	Vec2 slotSize{ 108, 108 };
	if (m_kanjiSlotTexture)
	{
		const Vec2 nativeSize = Vec2{ m_kanjiSlotTexture.size() };
		if ((nativeSize.x > 0.0) && (nativeSize.y > 0.0))
		{
			const double scale = Min(1.0, 112.0 / nativeSize.x);
			slotSize = nativeSize * scale;
		}
	}

	const double spacing = slotSize.x * 0.18;
	const double totalWidth = slotSize.x * slotCount + spacing * (slotCount - 1);
	const double gapFromDeck = 18.0;
	const double unclampedX = deckRect.center().x - totalWidth * 0.5;
	const double baseY = deckRect.y - slotSize.y - gapFromDeck;
	const double clampedX = Clamp(unclampedX, 20.0, Scene::Width() - totalWidth - 20.0);
	const double clampedY = Max(16.0, baseY);
	const Vec2 basePos{ clampedX, clampedY };

	for (size_t i = 0; i < slotCount; ++i)
	{
		const Vec2 slotPos = basePos + Vec2{ (slotSize.x + spacing) * i, 0.0 };
		const RectF slotRect{ slotPos, slotSize };

		if (m_kanjiSlotTexture)
		{
			m_kanjiSlotTexture.resized(slotRect.size).draw(slotRect.pos);
		}
		else
		{
			slotRect.rounded(14).draw(ColorF{ 0.12, 0.16, 0.22, 0.85 });
			slotRect.rounded(14).drawFrame(2, 0, ColorF{ 0.5, 0.6, 0.8, 0.35 });
		}

		if (i >= ownedOrdered.size())
		{
			continue;
		}

		const String& kanji = ownedOrdered[i];
		const auto it = m_kanjiTextures.find(kanji);
		if (it != m_kanjiTextures.end())
		{
			const Texture& kanjiTexture = it->second;
			Vec2 textureSize = Vec2{ kanjiTexture.size() };
			if ((textureSize.x <= 0.0) || (textureSize.y <= 0.0))
			{
				continue;
			}

			Vec2 drawSize = textureSize;
			const double maxWidth = slotRect.w * 0.78;
			const double maxHeight = slotRect.h * 0.78;
			const double scale = Min(maxWidth / drawSize.x, maxHeight / drawSize.y);
			if (scale < 1.0)
			{
				drawSize *= scale;
			}

			const Vec2 drawPos = slotRect.center() - drawSize * 0.5;
			kanjiTexture.resized(drawSize).draw(drawPos);
		}
		else
		{
			const Font font = FontAsset::IsRegistered(U"MisakiFont") ? FontAsset(U"MisakiFont") : m_clearCountFont;
			font(kanji).drawAt(slotRect.center(), ColorF{ 0.95 });
		}
	}
}

