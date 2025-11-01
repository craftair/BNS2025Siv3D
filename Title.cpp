#include "Title.h"

Title::Title(const InitData& init)
	: IScene{ init }
{

}

void Title::update()
{
	m_titleCg(texture1(0, 0, 1920, 1080)).draw();

	if (m_startBtn.mouseOver())
	{
		if (m_startBtn.leftClicked())
		{
			changeScene(State::Monolog);
		}
		else
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
		if (m_optionBtn.leftClicked())
		{
			getData().showSettings = !getData().showSettings;
		}
		else
		{
			m_optionBtn(texture5(0, 0, 612, 132)).draw();
		}
	}
	else
	{
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
	if (SimpleGUI::Button(U"Endingへ", Vec2{240, 100 }, 180))
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
