#pragma once
#include "Common.h"

class Title : public App::Scene
{
public:
	Title(const InitData& init);
	void update() override;
	void draw() const override;

private:
	const Texture texture1{ U"resources/texture/title-cg.png" };
	const Texture texture2{ U"resources/texture/start-btn.png" };
	const Texture texture3{ U"resources/texture/start-btn-selected.png" };
	const Texture texture4{ U"resources/texture/option-btn.png" };
	const Texture texture5{ U"resources/texture/option-btn-selected.png" };
	RectF m_titleCg{ 0, 0, 1280, 720 };
	RectF m_startBtn{ 462, 393, 356, 88 };
	RectF m_optionBtn{ 436, 513, 408, 88 };
};
