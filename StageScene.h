#pragma once
#include "Common.h"
#include "CardSystem.h"
#include "MapSystem.h"
#include "Player.h"
#include "CardEffectController.h"
#include "MapObjectTypes.h"
#include "KanjiSystem.h"

struct StageConfig
{
	int32 stageIndex = 1;
	Optional<State> nextState;
	String nextButtonText = U"次のステージへ";
};

class StageScene : public App::Scene
{
public:
	StageScene(const InitData& init, StageConfig config);
	void update() override;
	void draw() const override;

protected:
	const StageConfig m_config;

private:
	friend class CardEffectController;

	void activateYakuEffect();
	void healActionsToFull();
	void activateFullMapVision();
	void deactivateFullMapVision();
	void activateDebuffImmunity();
	void clearDebuffImmunity();

	void drawActionCounter() const;
	void onEndTurn();
	void modifyActionPoints(int32 delta);
	void handleGameOver();
	void handleMovement(const Point& previous, const Point& current);
	void handleTileInteractions(const Point& previous, const Point& current);
	void beginTreasureSelection(const Point& location);
	void handleTreasureSelectionInput();
	void drawTreasureSelection() const;
	void completeTreasureSelection(size_t optionIndex);
	void handleGoalReached();
	void handleClearModalInput();
	void drawClearModal() const;
	void drawKanjiPanel() const;
	int32 totalActionsTaken() const;
	bool isCardPlayable(const String& cardId) const;
	Array<String> playableCardPool() const;
	void prepareKanjiReward();
	bool shouldGrantKanjiReward() const;
	void unlockCardsForKanji(const KanjiInfo& info);

	struct ClearModalLayout
	{
		RectF modal;
		RectF stageRect;
		RectF totalRect;
		RectF nextButton;
		RectF titleButton;
	};

	ClearModalLayout makeClearModalLayout() const;

	MapSystem m_mapSystem;
	CardSystem m_cardSystem;
	Player m_player;
	CardEffectController m_cardEffects;
	Point m_settledPlayerGrid{ 0, 0 };
	int32 m_actionsUsed = 0;

	struct TreasureSelection
	{
		bool active = false;
		Point location{ 0, 0 };
		Array<String> cardIds;
		Array<RectF> cardRects;
	};

	TreasureSelection m_treasureSelection;
	Optional<size_t> m_treasureHover;
	bool m_showClearModal = false;
	bool m_resultRecorded = false;
	int32 m_maxActions = 5;
	int32 m_actionsRemaining = 5;
	bool m_promoteShinToKami = false;
	bool m_fullVisibilityActive = false;
	Array<Array<bool>> m_fullVisibilityBackup;
	bool m_ignoreTileDebuffsThisTurn = false;
	bool m_gameOver = false;
	Font m_clearCountFont{ 64, Typeface::Bold };
	Optional<String> m_pendingKanjiReward;
	Array<String> m_pendingRewardCards;
	Texture m_backgroundTexture;
	Texture m_kanjiSlotTexture;
	HashTable<String, Texture> m_kanjiTextures;
};
