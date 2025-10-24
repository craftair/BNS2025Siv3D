# pragma once

# include <Siv3D.hpp>

struct CardDefinition
{
	String typeId;
	String displayName;
	FilePath texturePath;
	Texture texture;
};

struct Card
{
	const CardDefinition* definition = nullptr;
	String instanceId;
	Vec2 baseCenter;
	Vec2 center;
	Vec2 grabOffset = Vec2::Zero();

	RectF rect(const SizeF& size) const;
};
