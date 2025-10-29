#include "Ending.h"

Ending::Ending(const InitData& init)
	: IScene{ init }
{
	Scene::SetResizeMode(ResizeMode::Keep);
}

void Ending::update()
{
	const RectF button = buttonRect();
	handleButtonInput(button);
}

void Ending::draw() const
{
	Scene::SetBackground(ColorF{ 0.04, 0.05, 0.09 });
	const RectF full{ 0, 0, Scene::Width(), Scene::Height() };
	full.draw(Arg::top = ColorF{ 0.12, 0.14, 0.2 }, Arg::bottom = ColorF{ 0.02, 0.03, 0.05 });

	const Vec2 headingCenter{ Scene::CenterF().x, Scene::Height() * 0.22 };
	m_titleFont(U"終わり(仮)").drawAt(headingCenter, ColorF{ 0.96 });
	//m_bodyFont(U"あなたの冒険はここで完結です").drawAt(headingCenter.movedBy(0, 56), ColorF{ 0.88 });

	const Vec2 focusCenter = Scene::CenterF().movedBy(0, 24);
	Circle{ focusCenter, 220 }.draw(ColorF{ 0.45, 0.55, 0.92, 0.18 });

	if (m_illustration)
	{
		const Size imageSize = m_illustration.size();
		Vec2 drawSize = Vec2{ imageSize };
		if ((imageSize.x > 0) && (imageSize.y > 0))
		{
			const Vec2 targetSize{ 520, 360 };
			const double scale = Min(targetSize.x / drawSize.x, targetSize.y / drawSize.y);
			drawSize *= scale;
		}
		const Vec2 drawPos = focusCenter - drawSize * 0.5;
		m_illustration.resized(drawSize.x, drawSize.y).draw(drawPos);
	}
	else
	{
		Circle{ focusCenter, 160 }.draw(ColorF{ 0.6, 0.72, 0.96, 0.35 });
	}

	const RectF button = buttonRect();
	const bool hovered = button.mouseOver();
	const ColorF fill = hovered ? ColorF{ 0.48, 0.68, 0.98, 0.95 } : ColorF{ 0.34, 0.52, 0.92, 0.95 };
	button.rounded(14).draw(fill);
	button.rounded(14).drawFrame(2, 0, ColorF{ 1.0, 1.0, 1.0, 0.3 });

	m_bodyFont(U"タイトルに戻る").drawAt(button.center(), ColorF{ 0.98 });
}

bool Ending::handleButtonInput(const RectF& buttonRect)
{
	if (buttonRect.leftClicked() || KeyEnter.down())
	{
		changeScene(State::Title);
		return true;
	}
	return false;
}

RectF Ending::buttonRect() const
{
	const double x = Scene::Width() - m_buttonSize.x - m_buttonMargin;
	const double y = Scene::Height() - m_buttonSize.y - m_buttonMargin;
	return RectF{ Vec2{ x, y }, m_buttonSize };
}

