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
	m_objectTextures.clear();
	m_objectPlacements.clear();

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
			m_objectPlacements << ObjectPlacement{ id, Point{ x, y } };
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
			if (row[x] == 0)
			{
				continue;
			}

			const RectF rect{ gridToWorld(Point{ static_cast<int32>(x), static_cast<int32>(y) }), m_tileSize };
			if (m_tileTexture)
			{
				m_tileTexture.resized(rect.size).draw(rect.pos);
			}
			else
			{
				rect.draw(ColorF{ 0.25, 0.3, 0.38, 0.95 });
			}
		}
	}

	for (const auto& placement : m_objectPlacements)
	{
		const RectF rect{ gridToWorld(placement.gridPos), m_tileSize };
		const Vec2 inset = m_tileSize * 0.18;
		const RectF objectRect = RectF{ rect.pos + inset, rect.size - inset * 2.0 };
		if (const auto it = m_objectTextures.find(placement.id); it != m_objectTextures.end())
		{
			it->second.resized(objectRect.size).draw(objectRect.pos);
		}
		else
		{
			objectRect.draw(ColorF{ 0.9, 0.2, 0.2, 0.8 });
		}
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


