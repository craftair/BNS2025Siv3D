#include "Stage1.h"

Stage1::Stage1(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 1, .nextState = State::Stage2 } }
{
}

