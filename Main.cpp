# include <Siv3D.hpp> // Siv3D v0.6.16
#include "Common.h"
#include "Title.h"
#include "Monolog.h"
#include "Tutorial.h"
#include "Stage1.h"
#include "Stage2.h"
#include "Stage3.h"
#include "Stage4.h"
#include "Stage5.h"
#include "Ending.h"

void Settings(GameData& data, App& manager, Array<Texture> textures, Array<Audio> audio)
{
	const RectF backgroundRect{ 0, 0, 1280, 720 };
	backgroundRect.draw(ColorF{ 0.0, 0.5 });
	const RectF modalRect{ 320, 168, 640, 384 };
	modalRect(textures[0](0, 0, 640, 384)).draw();

	const RectF closeBtnRect{ modalRect.tr() + Vec2{-72, 24}, 48, 48 };
	closeBtnRect(textures[1](0, 0, 48, 48)).draw();

	const Vec2 basePos = modalRect.tl();

	SimpleGUI::Slider(U" BGM", data.bgmVolume, 0.0, 1.0, basePos + Vec2{ 48, 96 }, 96, 400);
	data.bgm.setVolume(data.bgmVolume);

	SimpleGUI::Slider(U" SE", data.seVolume, 0.0, 1.0, basePos + Vec2{ 48, 192 }, 96, 400);

	if (SimpleGUI::Button(U"タイトルに戻る", basePos + Vec2{ 48, 288 }))
	{
		audio[0].playOneShot(data.seVolume);
		data.showSettings = false;
		manager.changeScene(State::Title);
	}

	if (backgroundRect.leftClicked() && not modalRect.leftClicked())
	{
		audio[1].playOneShot(data.seVolume);
		data.showSettings = false;
	}
	if (closeBtnRect.leftClicked())
	{
		audio[1].playOneShot(data.seVolume);
		data.showSettings = false;
	}
}

void Main()
{
	Window::Resize(1280, 720);

	FontAsset::Register(U"MisakiFont", FontMethod::MSDF, 16, U"resources/font/misaki/misaki_gothic.ttf");

	Array<Texture> settingsTextures = {
		Texture{ U"resources/texture/settings-box.png" },
		Texture{ U"resources/texture/close-btn.png" }
	};

	Array<Audio> settingsAudio = {
		Audio{ U"resources/audio/select1.ogg" },
		Audio{ U"resources/audio/cancel1.ogg" }
	};

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Monolog>(State::Monolog);
	manager.add<Tutorial>(State::Tutorial);
	manager.add<Stage1>(State::Stage1);
	manager.add<Stage2>(State::Stage2);
	manager.add<Stage3>(State::Stage3);
	manager.add<Stage4>(State::Stage4);
	manager.add<Stage5>(State::Stage5);
	manager.add<Ending>(State::Ending);

	auto gameData = manager.get();

	GlobalAudio::SetVolume(gameData->bgmVolume);

	while (System::Update())
	{
		if (not gameData->showSettings)
		{
			manager.update();
		}

		manager.drawScene();

		if (gameData->showSettings)
		{
			Settings(*gameData, manager, settingsTextures, settingsAudio);
		}
	}
}
