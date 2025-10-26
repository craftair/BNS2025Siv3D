#pragma once
#include "Common.h"
#include "MapSystem.h"

class Stage2 : public App::Scene
{
public:
	Stage2(const InitData& init);
	void update() override;
	void draw() const override;

private:
	MapSystem m_mapSystem;
};

