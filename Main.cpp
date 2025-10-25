# include <Siv3D.hpp> // Siv3D v0.6.16
#include "Common.h"
#include "Title.h"
#include "Monolog.h"
#include "Stage1.h"

void Main()
{
	Window::Resize(1280, 720);

	FontAsset::Register(U"MisakiFont", FontMethod::MSDF, 16, U"resources/font/misaki/misaki_gothic.ttf");

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Monolog>(State::Monolog);
	manager.add<Stage1>(State::Stage1);

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
