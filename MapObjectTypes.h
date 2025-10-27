#pragma once
#include <Siv3D.hpp>

enum class MapObjectType : int32
{
	None = 0,
	Box = 1,
	Treasure = 2,
	Rock = 3,
	FieldGlass = 4,
	Heal = 5,
	Camera = 6,
	Pillar = 7,
};

inline MapObjectType ToMapObjectType(const int32 id)
{
	return static_cast<MapObjectType>(id);
}

inline bool IsBlockingObject(const MapObjectType type)
{
	return (type == MapObjectType::Box) || (type == MapObjectType::Rock);
}

