#include "Stage5.h"

Stage5::Stage5(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 5, .nextState = State::Ending, .nextButtonText = U"エンディングへ" }, *init._s }
{
}

