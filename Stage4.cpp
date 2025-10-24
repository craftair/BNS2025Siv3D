#include "Stage4.h"

Stage4::Stage4(const InitData& init)
	: IScene{ init }
{

}

void Stage4::update()
{

}

void Stage4::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
