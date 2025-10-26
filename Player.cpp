#include "Player.h"
#include "MapSystem.h"

bool Player::init(const FilePathView& texturePath, const MapSystem& map, const Point& startGrid)
{
	m_texture = Texture{ texturePath, TextureDesc::Mipped };
	if (not m_texture)
	{
		return false;
	}

	m_gridPos = startGrid;
	updateVisualRect(map);
	return true;
}

void Player::setGridPosition(const Point& gridPos, const MapSystem& map)
{
	m_gridPos = gridPos;
	updateVisualRect(map);
}

void Player::update(const MapSystem& map)
{
	updateVisualRect(map);
}

void Player::draw(const MapSystem& map) const
{
	const Transformer2D transformer{ Mat3x2::Scale(map.scale()).translated(map.offset()) };
	if (m_texture)
	{
		m_texture.resized(m_visualRect.size).draw(m_visualRect.pos);
	}
	else
	{
		m_visualRect.draw(ColorF{ 0.95, 0.85, 0.2, 0.9 });
	}
}

void Player::updateVisualRect(const MapSystem& map)
{
	const Vec2 worldPos = map.gridToWorld(m_gridPos);
	const Vec2 tile = map.tileSize();
	const double scale = Min(m_scaleRatio, 0.95);
	const Vec2 targetSize = tile * scale;
	const double textureRatio = (m_texture) ? (static_cast<double>(m_texture.size().x) / m_texture.size().y) : 1.0;

	Vec2 finalSize = targetSize;
	if (targetSize.x / targetSize.y > textureRatio)
	{
		finalSize.x = targetSize.y * textureRatio;
	}
	else
	{
		finalSize.y = targetSize.x / textureRatio;
	}

	const Vec2 offset = (tile - finalSize) * 0.5;
	m_visualRect = RectF{ worldPos + offset, finalSize };
}
