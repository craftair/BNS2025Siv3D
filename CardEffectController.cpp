#include "CardEffectController.h"
#include "MapSystem.h"
#include "Player.h"
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

void CardEffectController::initialize(MapSystem* mapSystem, Player* player)
{
	m_mapSystem = mapSystem;
	m_player = player;
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
		if (m_state == EffectState::None)
		{
			return;
		}
	}

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

void CardEffectController::draw() const
{
	if ((m_state == EffectState::None) || (not m_mapSystem))
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

void CardEffectController::onCardPlayed(const String& cardId)
{
	if ((cardId != U"zen") && (cardId != U"choku"))
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

void CardEffectController::clearAllEffects()
{
	m_pendingCardEffects.clear();
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
		if (m_mapSystem->canEnter(target))
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

		while (m_mapSystem->canEnter(next))
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
	m_state = EffectState::None;
	m_targetOptions.clear();
	m_hoverTarget.reset();

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
	if (option.destination != m_player->gridPosition())
	{
		m_player->setGridPosition(option.destination, *m_mapSystem);
	}

	revealPath(option.path);
	m_mapSystem->revealAround(m_player->gridPosition());
	clearTargeting();
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
