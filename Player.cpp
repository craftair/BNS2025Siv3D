#include "Player.h"
#include "MapSystem.h"

namespace
{
	Texture loadTextureOrEmpty(const FilePathView& path)
	{
		try
		{
			return Texture{ path, TextureDesc::Mipped };
		}
		catch (...)
		{
			return Texture{};
		}
	}
}

Player::Player(GameData gameData)
	:m_gameData{ gameData }
{

}

bool Player::init(const FilePathView& texturePath, const MapSystem& map, const Point& startGrid)
{
	m_idleTexture = loadTextureOrEmpty(texturePath);
	if (not m_idleTexture)
	{
		return false;
	}

	m_slideTextures[0] = loadTextureOrEmpty(U"resources/texture/player/slide1.png");
	m_slideTextures[1] = loadTextureOrEmpty(U"resources/texture/player/slide2.png");
	m_upTextures[0] = loadTextureOrEmpty(U"resources/texture/player/up1.png");
	m_upTextures[1] = loadTextureOrEmpty(U"resources/texture/player/up2.png");
	m_downTextures[0] = loadTextureOrEmpty(U"resources/texture/player/down1.png");
	m_downTextures[1] = loadTextureOrEmpty(U"resources/texture/player/down2.png");

	m_gridPos = startGrid;
	m_targetRect = makeRectForGrid(map, startGrid);
	m_visualRect = m_targetRect;
	m_animation.active = false;
	applyIdlePose();
	return true;
}

void Player::setGridPosition(const Point& gridPos, const MapSystem& map)
{
	const Point previous = m_gridPos;
	const bool sameTile = (previous == gridPos);
	RectF startRect = m_visualRect;
	if (not m_animation.active)
	{
		startRect = m_targetRect;
	}

	m_gridPos = gridPos;
	m_targetRect = makeRectForGrid(map, m_gridPos);

	if (sameTile)
	{
		m_animation.active = false;
		m_visualRect = m_targetRect;
		applyIdlePose();
		return;
	}

	Direction direction = Direction::None;
	const Point delta = gridPos - previous;
	if (delta.x > 0)
	{
		direction = Direction::Right;
	}
	else if (delta.x < 0)
	{
		direction = Direction::Left;
	}
	else if (delta.y < 0)
	{
		direction = Direction::Up;
	}
	else if (delta.y > 0)
	{
		direction = Direction::Down;
	}

	m_animation.active = true;
	m_animation.elapsed = 0.0;
	m_animation.frameTimer = 0.0;
	m_animation.frameIndex = 0;
	m_animation.direction = direction;
	m_animation.startPos = startRect.pos;
	m_animation.endPos = m_targetRect.pos;

	// Ensure size updated for the new tile whilst animating.
	m_visualRect.size = startRect.size;
	m_visualRect.pos = startRect.pos;

	seMove.playOneShot(m_gameData.seVolume);
}

void Player::update(const MapSystem& map)
{
	m_targetRect = makeRectForGrid(map, m_gridPos);
	m_visualRect.size = m_targetRect.size;
	const double deltaTime = Scene::DeltaTime();
	if (m_animation.active)
	{
		m_animation.endPos = m_targetRect.pos;
		updateAnimation(deltaTime);
	}
	else
	{
		m_visualRect = m_targetRect;
	}
}

void Player::draw(const MapSystem& map) const
{
	const Transformer2D transformer{ Mat3x2::Scale(map.scale()).translated(map.offset()) };

	auto drawTextureRegion = [&](const Texture& texture, bool mirrorHorizontal = false)
		{
			if (texture)
			{
				TextureRegion region = texture;
				if (mirrorHorizontal)
				{
					region = region.mirrored();
				}
				region.resized(m_visualRect.size).draw(m_visualRect.pos);
			}
			else
			{
				m_visualRect.draw(ColorF{ 0.95, 0.85, 0.2, 0.9 });
			}
		};

	if (m_animation.active)
	{
		switch (m_animation.direction)
		{
		case Direction::Right:
			drawTextureRegion(m_slideTextures[m_animation.frameIndex]);
			return;
		case Direction::Left:
			drawTextureRegion(m_slideTextures[m_animation.frameIndex], true);
			return;
		case Direction::Up:
			drawTextureRegion(m_upTextures[m_animation.frameIndex]);
			return;
		case Direction::Down:
			drawTextureRegion(m_downTextures[m_animation.frameIndex]);
			return;
		case Direction::None:
			break;
		}
	}

	drawTextureRegion(m_idleTexture);
}

RectF Player::makeRectForGrid(const MapSystem& map, const Point& gridPos) const
{
	const Vec2 worldPos = map.gridToWorld(gridPos);
	const Vec2 tile = map.tileSize();
	const double scale = Min(m_scaleRatio, 0.95);
	const Vec2 targetSize = tile * scale;

	const Texture* referenceTexture = nullptr;
	if (m_idleTexture)
	{
		referenceTexture = &m_idleTexture;
	}
	else if (m_slideTextures[0])
	{
		referenceTexture = &m_slideTextures[0];
	}

	double textureRatio = 1.0;
	if (referenceTexture)
	{
		const Size size = referenceTexture->size();
		if ((size.y > 0) && (size.x > 0))
		{
			textureRatio = static_cast<double>(size.x) / size.y;
		}
	}

	Vec2 finalSize = targetSize;
	if (textureRatio > 0.0)
	{
		if (targetSize.x / targetSize.y > textureRatio)
		{
			finalSize.x = targetSize.y * textureRatio;
		}
		else
		{
			finalSize.y = targetSize.x / textureRatio;
		}
	}

	const Vec2 offset = (tile - finalSize) * 0.5;
	return RectF{ worldPos + offset, finalSize };
}

void Player::updateAnimation(double deltaTime)
{
	if (not m_animation.active)
	{
		return;
	}

	m_animation.elapsed += deltaTime;
	const double t = Clamp(m_animation.elapsed / m_animation.duration, 0.0, 1.0);
	m_visualRect.pos = Math::Lerp(m_animation.startPos, m_animation.endPos, t);

	m_animation.frameTimer += deltaTime;
	if (m_animation.frameTimer >= m_animation.frameInterval)
	{
		m_animation.frameTimer -= m_animation.frameInterval;
		m_animation.frameIndex = (m_animation.frameIndex + 1) % 2;
	}

	if (m_animation.elapsed >= m_animation.duration)
	{
		m_animation.active = false;
		m_visualRect = m_targetRect;
		applyIdlePose();
	}
}

void Player::applyIdlePose()
{
	m_animation.direction = Direction::None;
	m_animation.frameIndex = 0;
	m_animation.frameTimer = 0.0;
}

bool Player::isAnimating() const
{
	return m_animation.active;
}
