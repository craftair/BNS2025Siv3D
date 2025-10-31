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
	bool isFinished() const;

private:
	const Texture texture1{ U"resources/texture/message-box.png" };
	const Texture texture2{ U"resources/texture/monolog-cg.png" };
	RectF m_serifWindow{ 221, 528, 837, 186 };
	RectF m_cgArea{ 0, 0, 1280, 720 };
	Array<Message> m_messages;
	int message_index = 0;
	Stopwatch stopwatch;
};
