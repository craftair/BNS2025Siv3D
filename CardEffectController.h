#pragma once
#include "Common.h"

class MapSystem;
class Player;
class CardSystem;
class StageScene;

class CardEffectController
{
public:
	void initialize(MapSystem* mapSystem, Player* player, CardSystem* cardSystem, StageScene* stage);
	void update();
	void draw() const;
	void onCardPlayed(const String& cardId);
	void clearAllEffects();
	void cancelTargeting();

private:
	enum class ChoiceType
	{
		None,
		Ha2,
		Kou3
	};

	enum class DeckSelectionType
	{
		None,
		AddToHand
	};

	enum class EffectState
	{
		None,
		Zen,
		Choku,
		Kyuu,
		Ann,
		Kou2,
		DestroyObstacle,
		DestroyCamera,
		HandDiscard,
		DeckSelect,
		ChoiceHa2,
		ChoiceKou3
	};

	struct TargetOption
	{
		Point selection;
		Point destination;
		Array<Point> path;
	};

	void activateEffect(const String& cardId);
	void startZenTargeting();
	void startChokuTargeting();
	void startKyuuTargeting();
	void startAnnTargeting();
	void startKou2Targeting();
	void startDestroyObstacleTargeting();
	void startDestroyCameraTargeting();
	void clearTargeting(bool processQueue = true);
	void tryActivateNextCard();
	void updateHoverTarget();
	Optional<Point> screenToGrid(const Vec2& screenPos) const;
	bool optionContains(const TargetOption& option, const Point& gridPos) const;
	void applyTargetSelection(size_t optionIndex);
	void revealPath(const Array<Point>& path) const;
	bool canTraverse(const Point& gridPos) const;
	void destroyBoxesAlong(const Array<Point>& path);
	bool isBox(const Point& gridPos) const;
	bool applyImmediateEffect(const String& cardId);
	void removeCardForStage(const String& cardId);
	void enqueueTargetedEffect(const String& cardId);
	void updateMapTargeting();
	void drawMapTargeting() const;
	bool isMapTargetState(EffectState state) const;
	void completeMovementSelection(const TargetOption& option);
	void handleObstacleRemoval(const TargetOption& option);
	void handleCameraRemoval(const TargetOption& option);
	void beginHandDiscard();
	void updateHandDiscard();
	void drawHandDiscard() const;
	void beginChoicePrompt(ChoiceType type, const Array<String>& options);
	void updateChoicePrompt();
	void drawChoicePrompt() const;
	void resetChoiceState();
	void processChoiceSelection(size_t index);
	void beginDeckSelection(DeckSelectionType type);
	void updateDeckSelection();
	void drawDeckSelection() const;
	bool hasPendingEffects() const;

	DeckSelectionType m_deckSelectionType = DeckSelectionType::None;
	Array<size_t> m_deckSelectionIndices;
	Array<RectF> m_deckSelectionRects;
	Optional<size_t> m_deckSelectionHover;
	bool m_deckSelectionLimited = false;
	Array<String> m_choiceOptions;
	Array<RectF> m_choiceOptionRects;
	Optional<size_t> m_choiceHover;
	ChoiceType m_choiceType = ChoiceType::None;

	MapSystem* m_mapSystem = nullptr;
	Player* m_player = nullptr;
	CardSystem* m_cardSystem = nullptr;
	StageScene* m_stage = nullptr;
	Array<TargetOption> m_targetOptions;
	Optional<size_t> m_hoverTarget;
	EffectState m_state = EffectState::None;
	Array<String> m_pendingCardEffects;
	int32 m_boxBreakCharges = 0;
	size_t m_nextCardRepeats = 0;
};
