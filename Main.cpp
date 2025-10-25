# include <Siv3D.hpp> // Siv3D v0.6.16
#include "Common.h"
#include "Title.h"
#include "Stage1.h"

void Main()
{
	Window::Resize(1280, 720);

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Stage1>(State::Stage1);

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
