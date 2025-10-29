#include "Stage3.h"

Stage3::Stage3(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 3, .nextState = State::Stage4 } }
{
}

