#include "Stage1.h"

Stage1::Stage1(const InitData& init)
	: IScene{ init }
{
	Scene::SetResizeMode(ResizeMode::Keep);

	const Array<String> starterIds{ U"mae", U"ushiro", U"kougeki", U"mawatte" };
	CardHandConfig config;
	config.virtualSize = Vec2{ 1280, 720 };
	config.margin = 32.0;
	config.gap = 18.0;
	config.columns = starterIds.size();
	config.activationOffset = 40.0;

	if (not m_cardSystem.initialize(U"cards/cards.json", starterIds, config))
	{
		throw Error{ U"Failed to initialize card system" };
	}
}

void Stage1::update()
{
	if (KeyR.down())
	{
		m_cardSystem.resetUsage();
	}

	m_cardSystem.update();
}

void Stage1::draw() const
{
	Scene::SetBackground(ColorF{ 0.12, 0.12, 0.16 });
	m_cardSystem.draw();
}
