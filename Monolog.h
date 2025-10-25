#pragma once
#include "Common.h"

class Monolog : public App::Scene
{
public:
	Monolog(const InitData& init);
	void update() override;
	void draw() const override;

private:
	const Texture texture1{ U"resources/texture/serif-window.png" };
	RectF m_serifWindow{ 221, 528, 837, 186 };
};

