#pragma once
#include "Common.h"

class Ending : public App::Scene
{
public:
	explicit Ending(const InitData& init);
	void update() override;
	void draw() const override;

private:
	bool handleButtonInput(const RectF& buttonRect);
	RectF buttonRect() const;

	Texture m_illustration;
	Vec2 m_buttonSize{ 240, 68 };
	double m_buttonMargin = 36.0;
	Font m_titleFont{ 48, Typeface::Bold };
	Font m_bodyFont{ 28 };
};
