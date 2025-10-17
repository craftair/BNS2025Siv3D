#pragma once
#include <Siv3D.hpp>

class Object
{
public:
	Vec2 position = { 0, 0 };
	Texture texture;
	Array<P2Body> bodies;

	Object(const Vec2& pos, const Texture& _texture) : position(pos), texture(_texture)
	{

	}
	Object() = default;
	virtual ~Object() {}
	virtual void Update()
	{
		texture.drawAt(position);
	}
};

