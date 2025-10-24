#pragma once
#include "Common.h"

class Tutorial : public App::Scene
{
public:
	Tutorial(const InitData& init);
	void update() override;
	void draw() const override;
};

