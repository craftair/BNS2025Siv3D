#include "Stage1.h"

Stage1::Stage1(const InitData& init)
	: IScene{ init }
{
	Scene::SetResizeMode(ResizeMode::Keep);

	const Vec2 virtualSize{ 1280, 720 };
	if (not m_mapSystem.loadFromJSON(U"maps/Stage1.json", virtualSize))
	{
		throw Error{ U"Failed Stage1.json" };
	}

	if (not m_player.init(U"player/player.png", m_mapSystem, Point{ 0, 0 }))
	{
		throw Error{ U"Failed texture" };
	}

	const Array<String> starterIds{ U"zen", U"choku", U"ka" };
	CardHandConfig config;
	config.virtualSize = virtualSize;
	config.margin = 32.0;
	config.gap = 18.0;
	config.columns = starterIds.size();
	config.activationOffset = 40.0;

	if (not m_cardSystem.initialize(U"cards/cards.json", starterIds, config))
	{
		throw Error{ U"Failed card system" };
	}
}

void Stage1::update()
{
	m_mapSystem.update();
	m_player.update(m_mapSystem);

	if (KeyR.down())
	{
		m_cardSystem.resetUsage();
	}

	m_cardSystem.update();
}

void Stage1::draw() const
{
	Scene::SetBackground(ColorF{ 0.12, 0.12, 0.16 });
	m_mapSystem.draw();
	m_player.draw(m_mapSystem);
	m_cardSystem.draw();
}
