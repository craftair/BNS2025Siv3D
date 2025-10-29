#include "Stage4.h"

Stage4::Stage4(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 4, .nextState = State::Stage5 } }
{
}

