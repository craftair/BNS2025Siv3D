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

	m_serifWindow(texture1(0, 0, 837, 186)).draw();
}
