#pragma once
#include "Common.h"
#include "CardSystem.h"
#include "MapSystem.h"
#include "Player.h"

class Stage1 : public App::Scene
{
public:
	Stage1(const InitData& init);
	void update() override;
	void draw() const override;

private:
	MapSystem m_mapSystem;
	CardSystem m_cardSystem;
	Player m_player;
};
