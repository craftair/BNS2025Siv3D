#include "Ending.h"

Ending::Ending(const InitData& init)
	: IScene{ init }
{

}

void Ending::update()
{

}

void Ending::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
