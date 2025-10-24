#pragma once
#include "Common.h"

class Ending : public App::Scene
{
public:
	Ending(const InitData& init);
	void update() override;
	void draw() const override;
};

