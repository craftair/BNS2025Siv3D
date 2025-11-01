#include "Stage2.h"

Stage2::Stage2(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 2, .nextState = State::Stage3 }, *init._s }
{
}

