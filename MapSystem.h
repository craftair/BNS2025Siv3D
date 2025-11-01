#pragma once
#include "Common.h"
#include "MapObjectTypes.h"

class MapSystem
{
public:
	bool loadFromJSON(const FilePathView& path, const Vec2& virtualSize);
	void update();
	void draw() const;
	void setTileTexture(const FilePathView& path);
	Vec2 gridToWorld(const Point& gridPos) const;
	Vec2 tileSize() const { return m_tileSize; }
	Vec2 origin() const { return m_origin; }
	double scale() const { return m_scale; }
	Vec2 offset() const { return m_offset; }
	void setFogOfWarEnabled(bool enabled);
	void revealAround(const Point& gridPos);
	void revealRadius(const Point& gridPos, int32 radius);
	void revealObjectTiles(int32 objectId);
	bool isTileVisible(const Point& gridPos) const;
	bool fogOfWarEnabled() const { return m_fogOfWarEnabled; }
	bool isInBounds(const Point& gridPos) const;
	bool isWalkable(const Point& gridPos) const;
	bool canEnter(const Point& gridPos) const;
	int32 tileValue(const Point& gridPos) const;
	int32 objectIdAt(const Point& gridPos) const;
	bool hasObjectAt(const Point& gridPos) const;
	bool isObjectBlocking(int32 objectId) const;
	void removeObjectAt(const Point& gridPos);
	Array<Array<bool>> visibilitySnapshot() const;
	void applyVisibility(const Array<Array<bool>>& visibility);
	void revealAll();
	bool isCameraWatchTile(const Point& gridPos) const;
	Optional<Point> cameraForWatchTile(const Point& gridPos) const;

private:
	void updateTransform();
	void initializeVisibility(bool visible);
	void setAllVisible(bool visible);
	void clearCameraWatchData();
	void registerCameraWatchTile(const Point& cameraPos, const Point& watchPos);
	void removeCameraWatchTiles(const Point& cameraPos);
	void removeCameraWatchTile(const Point& watchPos);
	void ensureCameraWatchTexture();

	Vec2 m_virtualSize{ 1280, 720 };
	Vec2 m_tileSize{ 96, 96 };
	Vec2 m_origin{ 0, 0 };
	Texture m_tileTexture;
	Array<Array<int32>> m_tiles;
	Array<Array<int32>> m_objectGrid;
	Array<Array<bool>> m_tileVisibility;

	struct ObjectPlacement
	{
		int32 id = 0;
		Point gridPos{ 0, 0 };
	};

	HashTable<int32, Texture> m_objectTextures;
	Array<ObjectPlacement> m_objectPlacements;

	Texture m_fogTexture;
	Texture m_cameraWatchTexture;
	HashTable<Point, Array<Point>> m_cameraWatchTiles;
	HashTable<Point, Point> m_watchTileOwners;
	double m_scale = 1.0;
	Vec2 m_offset{ 0, 0 };
	bool m_fogOfWarEnabled = false;
};
