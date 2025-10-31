#include "MapSystem.h"

namespace
{
	double getNumber(const JSON& node, double fallback)
	{
		if (const auto opt = node.getOpt<double>())
		{
			return *opt;
		}
		return fallback;
	}

}

bool MapSystem::loadFromJSON(const FilePathView& path, const Vec2& virtualSize)
{
	m_virtualSize = virtualSize;
	m_scale = 1.0;
	m_offset = Vec2{ 0, 0 };
	m_tiles.clear();
	m_objectGrid.clear();
	m_tileVisibility.clear();
	m_objectTextures.clear();
	m_objectPlacements.clear();
	m_fogTexture = Texture{};

	const JSON json = JSON::Load(path);
	if (not json)
	{
		return false;
	}

	const JSON tileNode = json[U"tile"];
	if (not tileNode.isObject())
	{
		return false;
	}

	const JSON tileSizeNode = tileNode[U"size"];
	if (tileSizeNode.isArray())
	{
		if (tileSizeNode.size() >= 1)
		{
			m_tileSize.x = getNumber(tileSizeNode[0], m_tileSize.x);
		}
		if (tileSizeNode.size() >= 2)
		{
			m_tileSize.y = getNumber(tileSizeNode[1], m_tileSize.y);
		}
	}

	try
	{
		const auto textureOpt = tileNode[U"texture"].getOpt<String>();
		if (textureOpt && (not textureOpt->isEmpty()))
		{
			m_tileTexture = Texture{ *textureOpt, TextureDesc::Mipped };
		}
		else
		{
			m_tileTexture = Texture{};
		}
	}
	catch (...)
	{
		m_tileTexture = Texture{};
	}

	m_origin = Vec2{
		(m_virtualSize.x * 0.5) - (m_tileSize.x * 2.0),
		(m_virtualSize.y * 0.5) - (m_tileSize.y * 0.5)
	};

	JSON originNode;
	try
	{
		originNode = json[U"origin"];
	}
	catch (...) {}
	if (originNode && originNode.isArray())
	{
		if (originNode.size() >= 1)
		{
			m_origin.x = getNumber(originNode[0], m_origin.x);
		}
		if (originNode.size() >= 2)
		{
			m_origin.y = getNumber(originNode[1], m_origin.y);
		}
	}

	const JSON tilesNode = json[U"tiles"];
	if (not tilesNode.isArray())
	{
		return false;
	}

	m_tiles.clear();
	for (const auto& rowValue : tilesNode.arrayView())
	{
		if (not rowValue.isArray())
		{
			continue;
		}

		Array<int32> row;
		row.reserve(rowValue.size());

		for (const auto& cellValue : rowValue.arrayView())
		{
			row << cellValue.getOr<int32>(0);
		}

		m_tiles << row;
	}

	m_objectGrid.clear();
	m_objectGrid.reserve(m_tiles.size());
	for (const auto& row : m_tiles)
	{
		m_objectGrid << Array<int32>(row.size(), 0);
	}

	initializeVisibility(not m_fogOfWarEnabled);

	// Try to load a default fog texture used to cover unrevealed tiles.
	try
	{
		m_fogTexture = Texture{ U"resources/texture/field/Box2.png", TextureDesc::Mipped };
	}
	catch (...)
	{
		m_fogTexture = Texture{};
	}

	JSON objectDefsNode;
	try
	{
		objectDefsNode = json[U"objectDefinitions"];
	}
	catch (...) {}
	if (objectDefsNode && objectDefsNode.isArray())
	{
		for (const auto& value : objectDefsNode.arrayView())
		{
			if (not value.isObject())
			{
				continue;
			}

			const int32 id = value[U"id"].getOr<int32>(0);
			if (id == 0)
			{
				continue;
			}

			Texture texture;
			const String texturePath = value[U"texture"].getOr<String>(U"");
			if (texturePath)
			{
				texture = Texture{ texturePath, TextureDesc::Mipped };
			}

			if (texture)
			{
				m_objectTextures[id] = std::move(texture);
			}

		}
	}

	m_objectPlacements.clear();
	clearCameraWatchData();
	JSON placementsNode;
	try
	{
		placementsNode = json[U"objectPlacements"];
	}
	catch (...) {}
	if (placementsNode && placementsNode.isArray())
	{
		for (const auto& placementValue : placementsNode.arrayView())
		{
			if (not placementValue.isObject())
			{
				continue;
			}

			const int32 id = placementValue[U"id"].getOr<int32>(0);
			if (id == 0)
			{
				continue;
			}
			const JSON posNode = placementValue[U"position"];
			if ((not posNode.isArray()) || posNode.size() < 2)
			{
				continue;
			}

			const int32 x = posNode[0].getOr<int32>(0);
			const int32 y = posNode[1].getOr<int32>(0);
			const Point gridPos{ x, y };
			m_objectPlacements << ObjectPlacement{ id, gridPos };
			if ((y >= 0) && (y < static_cast<int32>(m_objectGrid.size())))
			{
				auto& row = m_objectGrid[y];
				if ((x >= 0) && (x < static_cast<int32>(row.size())))
				{
					row[x] = id;
				}
			}

			if (ToMapObjectType(id) == MapObjectType::Camera)
			{
				const JSON watchTilesNode = placementValue[U"watchTiles"];
				if (watchTilesNode && watchTilesNode.isArray())
				{
					for (const auto& watchValue : watchTilesNode.arrayView())
					{
						if ((not watchValue.isArray()) || (watchValue.size() < 2))
						{
							continue;
						}

						const int32 watchX = watchValue[0].getOr<int32>(0);
						const int32 watchY = watchValue[1].getOr<int32>(0);
						registerCameraWatchTile(gridPos, Point{ watchX, watchY });
					}
				}
			}
		}
	}

	return true;
}

void MapSystem::update()
{
	updateTransform();
}

void MapSystem::draw() const
{
	const Transformer2D transformer{ Mat3x2::Scale(m_scale).translated(m_offset) };

	for (size_t y = 0; y < m_tiles.size(); ++y)
	{
		const auto& row = m_tiles[y];
		for (size_t x = 0; x < row.size(); ++x)
		{
			const Point gridPos{ static_cast<int32>(x), static_cast<int32>(y) };
			const bool visible = isTileVisible(gridPos);
			const int32 tileValue = row[x];

			if (tileValue == 0)
			{
				continue;
			}

			const RectF rect{ gridToWorld(gridPos), m_tileSize };
			if (visible)
			{
				if (m_tileTexture)
				{
					m_tileTexture.resized(rect.size).draw(rect.pos);
				}
				else
				{
					rect.draw(ColorF{ 0.25, 0.3, 0.38, 0.95 });
				}
			}
			else
			{
				if (m_fogTexture)
				{
					m_fogTexture.resized(rect.size).draw(rect.pos);
				}
				else
				{
					rect.draw(ColorF{ 0.05, 0.05, 0.07, 0.95 });
				}
			}
		}
	}

	for (const auto& placement : m_objectPlacements)
	{
		if (objectIdAt(placement.gridPos) != placement.id)
		{
			continue;
		}

		if (not isTileVisible(placement.gridPos))
		{
			continue;
		}

		const RectF rect{ gridToWorld(placement.gridPos), m_tileSize };
		const MapObjectType type = ToMapObjectType(placement.id);
		const Vec2 inset = (type == MapObjectType::CameraWatch) ? Vec2{ 0, 0 } : (m_tileSize * 0.18);
		const RectF objectRect = RectF{ rect.pos + inset, rect.size - inset * 2.0 };
		const auto textureIt = m_objectTextures.find(placement.id);

		if (textureIt != m_objectTextures.end())
		{
			if (type == MapObjectType::CameraWatch)
			{
				const ColorF tint{ 1.0, 1.0, 1.0, 0.55 };
				textureIt->second.resized(rect.size).draw(rect.pos, tint);
			}
			else
			{
				textureIt->second.resized(objectRect.size).draw(objectRect.pos);
			}
		}
		else
		{
			if (type == MapObjectType::CameraWatch)
			{
				rect.draw(ColorF{ 0.3, 0.45, 0.8, 0.4 });
			}
			else
			{
				objectRect.draw(ColorF{ 0.9, 0.2, 0.2, 0.8 });
			}
		}
	}
}

void MapSystem::setTileTexture(const FilePathView& path)
{
	try
	{
		m_tileTexture = Texture{ path, TextureDesc::Mipped };
	}
	catch (...)
	{
		m_tileTexture = Texture{};
	}
}

void MapSystem::updateTransform()
{
	const double sceneWidth = Scene::Width();
	const double sceneHeight = Scene::Height();

	const double scaleX = sceneWidth / m_virtualSize.x;
	const double scaleY = sceneHeight / m_virtualSize.y;

	m_scale = Math::Min(scaleX, scaleY);
	m_offset = Vec2{ (sceneWidth - m_virtualSize.x * m_scale) * 0.5, (sceneHeight - m_virtualSize.y * m_scale) * 0.5 };
}

Vec2 MapSystem::gridToWorld(const Point& gridPos) const
{
	return m_origin + Vec2{ m_tileSize.x * gridPos.x, m_tileSize.y * gridPos.y };
}

void MapSystem::setFogOfWarEnabled(bool enabled)
{
	if (m_fogOfWarEnabled == enabled)
	{
		return;
	}

	m_fogOfWarEnabled = enabled;
	if (m_fogOfWarEnabled)
	{
		setAllVisible(false);
	}
	else
	{
		setAllVisible(true);
	}
}

void MapSystem::revealAround(const Point& gridPos)
{
	if (not m_fogOfWarEnabled)
	{
		return;
	}

	static constexpr Point offsets[] = {
		{ 0, 0 },
		{ 1, 0 },
		{ -1, 0 },
		{ 0, 1 },
		{ 0, -1 }
	};

	for (const auto& offset : offsets)
	{
		const Point target = gridPos + offset;
		if (isInBounds(target))
		{
			m_tileVisibility[target.y][target.x] = true;
		}
	}
}

bool MapSystem::isTileVisible(const Point& gridPos) const
{
	if (not m_fogOfWarEnabled)
	{
		return true;
	}

	if (not isInBounds(gridPos))
	{
		return false;
	}

	return m_tileVisibility[gridPos.y][gridPos.x];
}

void MapSystem::initializeVisibility(bool visible)
{
	m_tileVisibility.clear();
	m_tileVisibility.reserve(m_tiles.size());
	for (const auto& row : m_tiles)
	{
		m_tileVisibility << Array<bool>(row.size(), visible);
	}
}

void MapSystem::setAllVisible(bool visible)
{
	for (auto& row : m_tileVisibility)
	{
		for (auto& cell : row)
		{
			cell = visible;
		}
	}
}

bool MapSystem::isInBounds(const Point& gridPos) const
{
	if ((gridPos.y < 0) || (gridPos.y >= static_cast<int32>(m_tiles.size())))
	{
		return false;
	}

	if (gridPos.x < 0)
	{
		return false;
	}

	const auto rowSize = static_cast<int32>(m_tiles[gridPos.y].size());
	return (gridPos.x < rowSize);
}

void MapSystem::revealRadius(const Point& gridPos, int32 radius)
{
	if (not m_fogOfWarEnabled)
	{
		return;
	}

	if (radius < 0)
	{
		return;
	}

	for (int32 dy = -radius; dy <= radius; ++dy)
	{
		for (int32 dx = -radius; dx <= radius; ++dx)
		{
			const Point target = gridPos + Point{ dx, dy };
			if (isInBounds(target))
			{
				m_tileVisibility[target.y][target.x] = true;
			}
		}
	}
}

bool MapSystem::isCameraWatchTile(const Point& gridPos) const
{
	return m_watchTileOwners.contains(gridPos);
}

Optional<Point> MapSystem::cameraForWatchTile(const Point& gridPos) const
{
	if (const auto it = m_watchTileOwners.find(gridPos); it != m_watchTileOwners.end())
	{
		return it->second;
	}

	return none;
}

void MapSystem::clearCameraWatchData()
{
	m_cameraWatchTiles.clear();
	m_watchTileOwners.clear();
	m_cameraWatchTexture = Texture{};
	m_objectTextures.erase(static_cast<int32>(MapObjectType::CameraWatch));
}

void MapSystem::ensureCameraWatchTexture()
{
	const int32 watchId = static_cast<int32>(MapObjectType::CameraWatch);
	if (m_objectTextures.contains(watchId))
	{
		return;
	}

	if (not m_cameraWatchTexture)
	{
		try
		{
			m_cameraWatchTexture = Texture{ U"resources/texture/field/Box3.png", TextureDesc::Mipped };
		}
		catch (...)
		{
			m_cameraWatchTexture = Texture{};
		}
	}

	if (m_cameraWatchTexture)
	{
		m_objectTextures[watchId] = m_cameraWatchTexture;
	}
}

void MapSystem::registerCameraWatchTile(const Point& cameraPos, const Point& watchPos)
{
	if ((watchPos.y < 0) || (watchPos.y >= static_cast<int32>(m_objectGrid.size())))
	{
		return;
	}

	auto& row = m_objectGrid[watchPos.y];
	if ((watchPos.x < 0) || (watchPos.x >= static_cast<int32>(row.size())))
	{
		return;
	}

	const int32 watchId = static_cast<int32>(MapObjectType::CameraWatch);
	if ((row[watchPos.x] != 0) && (row[watchPos.x] != watchId))
	{
		return;
	}

	if (m_watchTileOwners.contains(watchPos))
	{
		return;
	}

	auto& watchList = m_cameraWatchTiles[cameraPos];
	if (watchList.contains(watchPos))
	{
		return;
	}

	ensureCameraWatchTexture();
	row[watchPos.x] = watchId;
	m_objectPlacements << ObjectPlacement{ watchId, watchPos };
	watchList << watchPos;
	m_watchTileOwners[watchPos] = cameraPos;
}

void MapSystem::removeCameraWatchTile(const Point& watchPos)
{
	const int32 watchId = static_cast<int32>(MapObjectType::CameraWatch);
	if ((watchPos.y >= 0) && (watchPos.y < static_cast<int32>(m_objectGrid.size())))
	{
		auto& row = m_objectGrid[watchPos.y];
		if ((watchPos.x >= 0) && (watchPos.x < static_cast<int32>(row.size())))
		{
			if (row[watchPos.x] == watchId)
			{
				row[watchPos.x] = 0;
			}
		}
	}

	m_objectPlacements.remove_if([&](const ObjectPlacement& placement)
	{
		return (placement.gridPos == watchPos) && (placement.id == watchId);
	});

	if (const auto owner = cameraForWatchTile(watchPos))
	{
		if (auto it = m_cameraWatchTiles.find(*owner); it != m_cameraWatchTiles.end())
		{
			it->second.remove_if([&](const Point& value)
			{
				return value == watchPos;
			});
			if (it->second.isEmpty())
			{
				m_cameraWatchTiles.erase(*owner);
			}
		}
	}

	m_watchTileOwners.erase(watchPos);
}

void MapSystem::removeCameraWatchTiles(const Point& cameraPos)
{
	const auto it = m_cameraWatchTiles.find(cameraPos);
	if (it == m_cameraWatchTiles.end())
	{
		return;
	}

	const Array<Point> watchCopy = it->second;
	for (const auto& watchPos : watchCopy)
	{
		removeCameraWatchTile(watchPos);
	}
	m_cameraWatchTiles.erase(cameraPos);
}

Array<Array<bool>> MapSystem::visibilitySnapshot() const
{
	return m_tileVisibility;
}

void MapSystem::applyVisibility(const Array<Array<bool>>& visibility)
{
	if (visibility.size() != m_tileVisibility.size())
	{
		return;
	}

	for (size_t y = 0; y < visibility.size(); ++y)
	{
		if (visibility[y].size() != m_tileVisibility[y].size())
		{
			return;
		}
	}

	m_tileVisibility = visibility;
}

void MapSystem::revealAll()
{
	setAllVisible(true);
}

int32 MapSystem::tileValue(const Point& gridPos) const
{
	if (not isInBounds(gridPos))
	{
		return 0;
	}

	return m_tiles[gridPos.y][gridPos.x];
}

bool MapSystem::isWalkable(const Point& gridPos) const
{
	return (tileValue(gridPos) != 0);
}

bool MapSystem::canEnter(const Point& gridPos) const
{
	if (not isWalkable(gridPos))
	{
		return false;
	}

	const int32 objectId = objectIdAt(gridPos);
	if (objectId == 0)
	{
		return true;
	}

	return (not isObjectBlocking(objectId));
}

int32 MapSystem::objectIdAt(const Point& gridPos) const
{
	if ((gridPos.y < 0) || (gridPos.y >= static_cast<int32>(m_objectGrid.size())))
	{
		return 0;
	}

	const auto& row = m_objectGrid[gridPos.y];
	if ((gridPos.x < 0) || (gridPos.x >= static_cast<int32>(row.size())))
	{
		return 0;
	}

	return row[gridPos.x];
}

bool MapSystem::hasObjectAt(const Point& gridPos) const
{
	return (objectIdAt(gridPos) != 0);
}

bool MapSystem::isObjectBlocking(int32 objectId) const
{
	return IsBlockingObject(ToMapObjectType(objectId));
}

void MapSystem::removeObjectAt(const Point& gridPos)
{
	if ((gridPos.y < 0) || (gridPos.y >= static_cast<int32>(m_objectGrid.size())))
	{
		return;
	}

	auto& row = m_objectGrid[gridPos.y];
	if ((gridPos.x < 0) || (gridPos.x >= static_cast<int32>(row.size())))
	{
		return;
	}

	const int32 objectId = row[gridPos.x];
	if (objectId == 0)
	{
		return;
	}

	const MapObjectType type = ToMapObjectType(objectId);
	if (type == MapObjectType::CameraWatch)
	{
		removeCameraWatchTile(gridPos);
		return;
	}

	row[gridPos.x] = 0;
	m_objectPlacements.remove_if([&](const ObjectPlacement& placement)
	{
		return placement.gridPos == gridPos;
	});

	if (type == MapObjectType::Camera)
	{
		removeCameraWatchTiles(gridPos);
	}
}



