#include "Monolog.h"

Monolog::Monolog(const InitData& init)
	: IScene{ init }
{

}

void Monolog::update()
{

}

void Monolog::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });
}
