# include <Siv3D.hpp> // Siv3D v0.6.16
# include "card.h"

namespace
{
	Array<CardDefinition> LoadCardDefinitions(const FilePathView path)
	{
		const JSON json = JSON::Load(path);
		if (!json)
		{
			throw Error{ U"Failed to load card definition file: {}"_fmt(path) };
		}

		Array<CardDefinition> definitions;

		if (!json.isArray())
		{
			throw Error{ U"Card definition file must be a JSON array: {}"_fmt(path) };
		}

		definitions.reserve(json.size());

		for (const auto& entry : json.arrayView())
		{
			CardDefinition definition;
			definition.typeId = entry[U"id"].getString();
			definition.displayName = entry[U"name"].getString();
			definition.texturePath = entry[U"texture"].getString();
			definition.texture = Texture{ definition.texturePath };

			if (!definition.texture)
			{
				throw Error{ U"Failed to load texture for card '{}': {}"_fmt(definition.typeId, definition.texturePath) };
			}

			definitions << std::move(definition);
		}

		return definitions;
	}

	SizeF GetCardVisualSize(const CardDefinition& definition, const double cardScale)
	{
		const Size textureSize = definition.texture.size();
		return SizeF{
			textureSize.x * cardScale,
			textureSize.y * cardScale
		};
	}
}

void Main()
{
	Window::Resize(1920, 1080);
	Scene::SetResizeMode(ResizeMode::Keep);
	Scene::SetBackground(ColorF(0.08, 0.1, 0.14));

	const Array<CardDefinition> cardDefinitions = LoadCardDefinitions(U"cards.json");
	if (cardDefinitions.isEmpty())
	{
		throw Error{ U"No card definitions found in cards.json" };
	}

	const Vec2 sceneSize{ static_cast<double>(Scene::Width()), static_cast<double>(Scene::Height()) };
	const Vec2 referenceScene{ 1920.0, 1080.0 };
	const double layoutScale = Min(sceneSize.x / referenceScene.x, sceneSize.y / referenceScene.y);
	const double cardScale = (2.0 / 3.0) * layoutScale;

	const SizeF layoutCardSize = GetCardVisualSize(cardDefinitions.front(), cardScale);

	const double margin = 40.0 * layoutScale;
	const double spacing = 30.0 * layoutScale;
	const double useThresholdY = sceneSize.y * 0.57;

	Array<Card> cards;
	cards.reserve(4);

	uint64 nextInstanceId = 1;

	const Vec2 anchor = Vec2{
		sceneSize.x - margin - (layoutCardSize.x * 0.5),
		sceneSize.y - margin - (layoutCardSize.y * 0.5)
	};

	for (int32 i = 0; i < 4; ++i)
	{
		const CardDefinition& definition = cardDefinitions[i % cardDefinitions.size()];
		const Vec2 baseCenter = anchor - Vec2{ (layoutCardSize.x + spacing) * i, 0.0 };

		Card card;
		card.definition = &definition;
		card.instanceId = U"deck_{:03}"_fmt(nextInstanceId++);
		card.baseCenter = baseCenter;
		card.center = baseCenter;
		cards << card;
	}

	Optional<size_t> activeCard;
	const Font instructionFont{ 28, Typeface::Heavy };

	while (System::Update())
	{
		Line{ 0, useThresholdY, Scene::Width(), useThresholdY }.draw(4, ColorF(1.0, 0.3, 0.3, 0.25));
		//instructionFont(U"ドラッグして赤線より上で離すとカードを使用します").draw(Vec2{ 40, 40 }, Palette::Lightyellow);

		if (!activeCard && MouseL.down())
		{
			for (size_t i = cards.size(); i > 0; --i)
			{
				const size_t index = i - 1;
				const SizeF cardSize = GetCardVisualSize(*cards[index].definition, cardScale);

				if (cards[index].rect(cardSize).contains(Cursor::PosF()))
				{
					Card card = cards[index];
					cards.erase(cards.begin() + index);
					cards << card;
					activeCard = cards.size() - 1;
					cards[*activeCard].grabOffset = Cursor::PosF() - cards[*activeCard].center;
					break;
				}
			}
		}

		if (activeCard)
		{
			const size_t index = *activeCard;
			const SizeF cardSize = GetCardVisualSize(*cards[index].definition, cardScale);

			if (MouseL.pressed())
			{
				cards[index].center = Cursor::PosF() - cards[index].grabOffset;
			}
			else
			{
				const double topY = cards[index].center.y - (cardSize.y * 0.5);

				if (topY < useThresholdY)
				{
					const Card& usedCard = cards[index];
					Print << U"Used card type={} instance={}"_fmt(usedCard.definition->typeId, usedCard.instanceId);
					cards.erase(cards.begin() + index);
				}
				else
				{
					cards[index].center = cards[index].baseCenter;
				}

				activeCard.reset();
			}
		}

		for (size_t i = 0; i < cards.size(); ++i)
		{
			const Card& card = cards[i];
			const SizeF cardSize = GetCardVisualSize(*card.definition, cardScale);
			const bool isHovered = card.rect(cardSize).mouseOver();
			const bool isActive = (activeCard && (*activeCard == i));
			const double scale = isActive ? 1.05 : (isHovered ? 1.02 : 1.0);
			const ColorF tint = ColorF(1.0, isActive ? 1.0 : (isHovered ? 0.95 : 0.85));

			card.definition->texture.scaled(cardScale * scale).drawAt(card.center, tint);
		}
	}
}
