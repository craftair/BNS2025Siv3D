#pragma once
#include "Common.h"
#include "CardSystem.h"
#include "MapSystem.h"
#include "Player.h"
#include "CardEffectController.h"
#include "MapObjectTypes.h"

class Stage1 : public App::Scene
{
public:
	Stage1(const InitData& init);
	void update() override;
	void draw() const override;

private:
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

	MapSystem m_mapSystem;
	CardSystem m_cardSystem;
	Player m_player;
	CardEffectController m_cardEffects;
	Point m_previousPlayerGrid{ 0, 0 };

	struct TreasureSelection
	{
		bool active = false;
		Point location{ 0, 0 };
		Array<String> cardIds;
		Array<RectF> cardRects;
	};

	TreasureSelection m_treasureSelection;
	Optional<size_t> m_treasureHover;
	int32 m_actionsRemaining = 5;
	bool m_gameOver = false;
};
