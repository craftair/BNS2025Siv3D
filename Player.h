#pragma once
#include "Common.h"
#include <array>

class MapSystem;

class Player
{
public:
	Player(GameData gameData);
	bool init(const FilePathView& texturePath, const MapSystem& map, const Point& startGrid);
	void setGridPosition(const Point& gridPos, const MapSystem& map);
	void update(const MapSystem& map);
	void draw(const MapSystem& map) const;
	bool isAnimating() const;

	Point gridPosition() const { return m_gridPos; }

private:
	enum class Direction
	{
		None,
		Left,
		Right,
		Up,
		Down
	};

	struct AnimationState
	{
		bool active = false;
		Vec2 startPos{ 0, 0 };
		Vec2 endPos{ 0, 0 };
		double duration = 0.2;
		double elapsed = 0.0;
		double frameInterval = 0.08;
		double frameTimer = 0.0;
		size_t frameIndex = 0;
		Direction direction = Direction::None;
	};

	RectF makeRectForGrid(const MapSystem& map, const Point& gridPos) const;
	void updateAnimation(double deltaTime);
	void applyIdlePose();

	Texture m_idleTexture;
	std::array<Texture, 2> m_slideTextures;
	std::array<Texture, 2> m_upTextures;
	std::array<Texture, 2> m_downTextures;
	Point m_gridPos{ 0, 0 };
	RectF m_visualRect{ 0, 0, 0, 0 };
	RectF m_targetRect{ 0, 0, 0, 0 };
	double m_scaleRatio = 0.94;
	AnimationState m_animation;

	GameData m_gameData;
	Audio seMove{ U"resources/audio/move1.ogg" };
};
