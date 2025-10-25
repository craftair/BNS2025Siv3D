#include "Title.h"

Title::Title(const InitData& init)
	: IScene{ init }
{

}

void Title::update()
{
	if (SimpleGUI::Button(U"Stage1へ", Vec2{ 40, 40 }, 180))
	{
		changeScene(State::Stage1);
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0.1, 0.12, 0.18 });
}
