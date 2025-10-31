#include "Tutorial.h"

Tutorial::Tutorial(const InitData& init)
	: StageScene{ init, StageConfig{ .stageIndex = 0, .nextState = State::Stage1, .nextButtonText = U"ステージ1へ" } }
{
}
