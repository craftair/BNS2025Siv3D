#pragma once
#include "Common.h"

class Stage5 : public App::Scene
{
public:
	Stage5(const InitData& init);
	void update() override;
	void draw() const override;
};

