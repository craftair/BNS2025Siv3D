#include "Stage2.h"

Stage2::Stage2(const InitData& init)
	: IScene{ init }
{

}

void Stage2::update()
{

}

void Stage2::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
