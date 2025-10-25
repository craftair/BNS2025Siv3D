#pragma once
#include "Common.h"
#include "CardSystem.h"

class Stage1 : public App::Scene
{
public:
	Stage1(const InitData& init);
	void update() override;
	void draw() const override;

private:
	CardSystem m_cardSystem;
};
