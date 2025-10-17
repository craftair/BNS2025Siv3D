#pragma once
#include <Siv3D.hpp>
#include "Object.h"

class Player : public Object
{
public:
	Player(const Vec2& pos, const Texture& _texture, P2World& world) : Object(pos, _texture)
	{
		bodies << world.createCircle(P2Dynamic, Vec2{ pos }, 10);
	}
	void Update();
};

