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
}

void Monolog::update()
{
	if (m_serifWindow.leftClicked())
	{
		message_index++;
		if (message_index >= m_messages.size())
		{
			changeScene(State::Title);
		}
	}
}

void Monolog::draw() const
{
	Scene::SetBackground(ColorF{ 1.0 });

	m_serifWindow(texture1(0, 0, 837, 186)).draw();

	const Font& font = FontAsset(U"MisakiFont");
	if (message_index < m_messages.size())
	{
		font(U"{}"_fmt(m_messages[message_index].text)).draw(32, m_serifWindow.tl() + Vec2{ 32, 32 }, ColorF{ 0.0 });
	}
	else
	{
		font(U"{}"_fmt(m_messages[m_messages.size() - 1].text)).draw(32, m_serifWindow.tl() + Vec2{32, 32}, ColorF{0.0});
	}
}
