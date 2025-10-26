#pragma once
#include "Common.h"

class MapSystem;

class Player
{
public:
	bool init(const FilePathView& texturePath, const MapSystem& map, const Point& startGrid);
	void setGridPosition(const Point& gridPos, const MapSystem& map);
	void update(const MapSystem& map);
	void draw(const MapSystem& map) const;

	Point gridPosition() const { return m_gridPos; }

private:
	void updateVisualRect(const MapSystem& map);

	Texture m_texture;
	Point m_gridPos{ 0, 0 };
	RectF m_visualRect{ 0, 0, 0, 0 };
	double m_scaleRatio = 0.78;
};
