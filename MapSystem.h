#pragma once
#include "Common.h"

class MapSystem
{
public:
	bool loadFromJSON(const FilePathView& path, const Vec2& virtualSize);
	void update();
	void draw() const;
	Vec2 gridToWorld(const Point& gridPos) const;
	Vec2 tileSize() const { return m_tileSize; }
	Vec2 origin() const { return m_origin; }
	double scale() const { return m_scale; }
	Vec2 offset() const { return m_offset; }

private:
	void updateTransform();

	Vec2 m_virtualSize{ 1280, 720 };
	Vec2 m_tileSize{ 96, 96 };
	Vec2 m_origin{ 0, 0 };
	Texture m_tileTexture;
	Array<Array<int32>> m_tiles;

	struct ObjectPlacement
	{
		int32 id = 0;
		Point gridPos{ 0, 0 };
	};

	HashTable<int32, Texture> m_objectTextures;
	Array<ObjectPlacement> m_objectPlacements;

	double m_scale = 1.0;
	Vec2 m_offset{ 0, 0 };
};
