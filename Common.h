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
	bool showSettings = false;
	double bgmVolume = 1.0;
	double seVolume = 1.0;
	int32 totalActions = 0;
	HashSet<String> kanjiOwned = { U"進" };
	HashSet<int32> kanjiRewardStagesClaimed;
	HashSet<String> unlockedCards;
	bool completedGame = false;
};

using App = SceneManager<State, GameData>;
