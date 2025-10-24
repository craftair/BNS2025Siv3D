#include "Title.h"

Title::Title(const InitData& init)
	: IScene{ init }
{

}

void Title::update()
{

}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
