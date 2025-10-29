#include "KanjiSystem.h"

namespace
{
	const Array<KanjiInfo> s_kanjiInfos = {
		{ U"進", { U"zen", U"choku", U"ka" } },
		{ U"心", { U"you", U"ann", U"ke" } },
		{ U"新", { U"dou", U"pin", U"yaku" } },
		{ U"神", { U"dou2", U"pin2", U"soku" } },
		{ U"信", { U"kou", U"kan", U"ha2" } },
		{ U"侵", { U"kou2", U"ryaku", U"nyuu" } },
	};

	HashTable<String, Array<String>> BuildRequirements()
	{
		HashTable<String, Array<String>> table;
		for (const auto& info : s_kanjiInfos)
		{
			for (const auto& cardId : info.cardIds)
			{
				table[cardId] << info.id;
			}
		}

		table[U"kou3"] = { U"信", U"侵" };
		return table;
	}

	const HashTable<String, Array<String>> s_requirements = BuildRequirements();
	const Array<String> s_empty;
}

const Array<KanjiInfo>& KanjiSystem::allKanji()
{
	return s_kanjiInfos;
}

const KanjiInfo* KanjiSystem::findKanji(const String& id)
{
	for (const auto& info : s_kanjiInfos)
	{
		if (info.id == id)
		{
			return &info;
		}
	}
	return nullptr;
}

const Array<String>& KanjiSystem::requirementsForCard(const String& cardId)
{
	const auto it = s_requirements.find(cardId);
	if (it != s_requirements.end())
	{
		return it->second;
	}
	return s_empty;
}

bool KanjiSystem::hasRequirements(const HashSet<String>& ownedKanji, const String& cardId)
{
	const auto& requirements = requirementsForCard(cardId);
	for (const auto& kanji : requirements)
	{
		if (not ownedKanji.contains(kanji))
		{
			return false;
		}
	}
	return true;
}

