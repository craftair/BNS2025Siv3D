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
	}
	else
	{
		startBtnEntered = false;
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
	}
	else
	{
		optionBtnEntered = false;
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0.1, 0.12, 0.18 });

	m_titleCg(texture1(0, 0, 1920, 1080)).draw();

	if (m_startBtn.mouseOver())
	{
		if (!m_startBtn.leftClicked())
		{
			m_startBtn(texture3(0, 0, 534, 132)).draw();
		}
	}
	else
	{
		m_startBtn(texture2(0, 0, 534, 132)).draw();
	}

	if (m_optionBtn.mouseOver())
	{
		if (!m_optionBtn.leftClicked())
		{
			m_optionBtn(texture5(0, 0, 612, 132)).draw();
		}
	}
	else
	{
		m_optionBtn(texture4(0, 0, 612, 132)).draw();
	}
}
