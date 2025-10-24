#pragma once
#include "Common.h"

class Stage4 : public App::Scene
{
public:
	Stage4(const InitData& init);
	void update() override;
	void draw() const override;
};

