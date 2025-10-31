#include "Monolog.h"

Monolog::Monolog(const InitData& init)
	: IScene{ init }
{
	const JSON json = JSON::Load(U"resources/txt/jp.json");

	if (not json)
	{
		throw Error{ U"Failed to load `jp.json`" };
	}

	for (auto&& [index, object] : json[U"monolog"])
	{
		Message message;
		message.text = object[U"text"].getString();
		m_messages.push_back(message);
	}

	stopwatch.start();
}

void Monolog::update()
{
	if (isFinished() && m_serifWindow.leftClicked())
	{
		message_index++;
		stopwatch.restart();
		if (message_index >= m_messages.size())
		{
			changeScene(State::Title);
		}
	}
}

void Monolog::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });

	m_cgArea(texture2(0, 0, 1920, 1080)).draw();
	m_serifWindow(texture1(0, 0, 837, 186)).draw();

	const Font& font = FontAsset(U"MisakiFont");

	const int32 count = Max(((stopwatch.ms() - 200) / 24), 0);

	if (message_index < m_messages.size())
	{
		font(U"{}"_fmt(m_messages[message_index].text.substr(0, count))).draw(32, m_serifWindow.tl() + Vec2{ 32, 32 }, ColorF{ 0.0 });
	}
	else
	{
		font(U"{}"_fmt(m_messages[m_messages.size() - 1].text)).draw(32, m_serifWindow.tl() + Vec2{ 32, 32 }, ColorF{ 0.0 });
	}

	if (isFinished())
	{
		Triangle{ m_serifWindow.br().movedBy(-48, -42), 20, 180_deg }.draw(ColorF{ 0.0, Periodic::Sine0_1(2.0s) });
	}
}

bool Monolog::isFinished() const
{
	if (message_index >= m_messages.size())
	{
		return true;
	}
	const int32 count = Max(((stopwatch.ms() - 200) / 24), 0);
	return (static_cast<int32>(m_messages[message_index].text.length()) <= count);
}
