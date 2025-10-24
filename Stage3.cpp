#include "Stage3.h"

Stage3::Stage3(const InitData& init)
	: IScene{ init }
{

}

void Stage3::update()
{

}

void Stage3::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
