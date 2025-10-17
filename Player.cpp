#include "stdafx.h"
#include "Player.h"
#include <Siv3D.hpp>

void Player::Update()
{
	position = bodies[0].getPos();
	Object::Update();
}
