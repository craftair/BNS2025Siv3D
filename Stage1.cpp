#include "Stage1.h"

Stage1::Stage1(const InitData& init)
	: IScene{ init }
{

}

void Stage1::update()
{

}

void Stage1::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
