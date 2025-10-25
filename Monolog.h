#pragma once
#include "Common.h"

class Message
{
public:
	String text;
};

class Monolog : public App::Scene
{
public:
	Monolog(const InitData& init);
	void update() override;
	void draw() const override;

private:
	const Texture texture1{ U"resources/texture/message-box.png" };
	RectF m_serifWindow{ 221, 528, 837, 186 };
	Array<Message> m_messages;
	int message_index = 0;
};
