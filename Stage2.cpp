#include "Stage2.h"

Stage2::Stage2(const InitData& init)
	: IScene{ init }
{
	Scene::SetResizeMode(ResizeMode::Keep);

	if (not m_mapSystem.loadFromJSON(U"maps/Stage2.json", Vec2{ 1280, 720 }))
	{
		throw Error{ U"Failed to load maps/Stage2.json" };
	}
}

void Stage2::update()
{
	m_mapSystem.update();
}

void Stage2::draw() const
{
	Scene::SetBackground(ColorF{ 0.1, 0.14, 0.17 });
	m_mapSystem.draw();
}
