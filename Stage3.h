#pragma once
#include "Common.h"

class Stage3 : public App::Scene
{
public:
	Stage3(const InitData& init);
	void update() override;
	void draw() const override;
};

