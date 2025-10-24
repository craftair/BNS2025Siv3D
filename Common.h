#pragma once
#include <Siv3D.hpp>

enum class State
{
	Title,
	Monolog,
	Tutorial,
	Stage1,
	Stage2,
	Stage3,
	Stage4,
	Stage5,
	Ending,
};

struct GameData
{

};

using App = SceneManager<State, GameData>;
