#pragma once
#include "Common.h"

class MapSystem;
class Player;

class CardEffectController
{
public:
	void initialize(MapSystem* mapSystem, Player* player);
	void update();
	void draw() const;
	void onCardPlayed(const String& cardId);
	void clearAllEffects();
	void cancelTargeting();

private:
	enum class EffectState
	{
		None,
		Zen,
		Choku
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

	MapSystem* m_mapSystem = nullptr;
	Player* m_player = nullptr;
	Array<TargetOption> m_targetOptions;
	Optional<size_t> m_hoverTarget;
	EffectState m_state = EffectState::None;
	Array<String> m_pendingCardEffects;
	bool m_canBreakBoxesThisTurn = false;
};
