#include "Title.h"

Title::Title(const InitData& init)
	: IScene{ init }
{
	if (getData().bgmName != U"resources/audio/title-bgm.ogg")
	{
		getData().bgmName = U"resources/audio/title-bgm.ogg";
		getData().bgm = Audio{ Audio::Stream, U"resources/audio/title-bgm.ogg", Loop::Yes };
		getData().bgm.setVolume(getData().bgmVolume);
		getData().bgm.play();
	}

	getData().showSettings = false;
	getData().totalActions = 0;
	getData().kanjiOwned = { U"進" };
	getData().kanjiRewardStagesClaimed.clear();
	getData().unlockedCards.clear();
	getData().completedGame = false;
}

void Title::update()
{
	m_titleCg(texture1(0, 0, 1920, 1080)).draw();

	if (m_startBtn.mouseOver())
	{
		if (!startBtnEntered)
		{
			seCursor1.playOneShot(getData().seVolume);
			startBtnEntered = true;
		}
		if (m_startBtn.leftClicked())
		{
			seStart1.playOneShot(getData().seVolume);
			changeScene(State::Monolog);
		}
		else
		{
			m_startBtn(texture3(0, 0, 534, 132)).draw();
		}
	}
	else
	{
		startBtnEntered = false;
		m_startBtn(texture2(0, 0, 534, 132)).draw();
	}

	if (m_optionBtn.mouseOver())
	{
		if (!optionBtnEntered)
		{
			seCursor1.playOneShot(getData().seVolume);
			optionBtnEntered = true;
		}
		if (m_optionBtn.leftClicked())
		{
			MouseL.clearInput();
			seSelect1.playOneShot(getData().seVolume);
			getData().showSettings = !getData().showSettings;
		}
		else
		{
			m_optionBtn(texture5(0, 0, 612, 132)).draw();
		}
	}
	else
	{
		optionBtnEntered = false;
		m_optionBtn(texture4(0, 0, 612, 132)).draw();
	}

	if (SimpleGUI::Button(U"Monologへ", Vec2{ 240, 40 }, 180))
	{
		changeScene(State::Monolog);
	}
	if (SimpleGUI::Button(U"チュートリアルへ", Vec2{ 40, 40 }, 180))
	{
		changeScene(State::Tutorial);
	}
	if (SimpleGUI::Button(U"Stage1へ", Vec2{ 40, 100 }, 180))
	{
		changeScene(State::Stage1);
	}
	if (SimpleGUI::Button(U"Endingへ", Vec2{ 240, 100 }, 180))
	{
		changeScene(State::Ending);
	}
	if (getData().completedGame == false)
	{
		if (SimpleGUI::Button(U"completed: false", Vec2{ 40, 160 }, 360))
		{
			getData().completedGame = true;
		}
	}
	else
	{
		if (SimpleGUI::Button(U"completed: true", Vec2{ 40, 160 }, 360))
		{
			getData().completedGame = false;
		}
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0.1, 0.12, 0.18 });

	//m_titleCg(texture1(0, 0, 1920, 1080)).draw();
}
