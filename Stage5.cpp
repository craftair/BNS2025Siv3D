#include "Stage5.h"

Stage5::Stage5(const InitData& init)
	: IScene{ init }
{

}

void Stage5::update()
{

}

void Stage5::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
