#include "Tutorial.h"

Tutorial::Tutorial(const InitData& init)
	: IScene{ init }
{

}

void Tutorial::update()
{

}

void Tutorial::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
