#pragma once
#include "Common.h"

class Monolog : public App::Scene
{
public:
	Monolog(const InitData& init);
	void update() override;
	void draw() const override;
};

