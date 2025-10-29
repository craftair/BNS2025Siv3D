#pragma once
#include <Siv3D.hpp>

struct KanjiInfo
{
	String id;
	Array<String> cardIds;
};

namespace KanjiSystem
{
	const Array<KanjiInfo>& allKanji();
	const KanjiInfo* findKanji(const String& id);
	const Array<String>& requirementsForCard(const String& cardId);
	bool hasRequirements(const HashSet<String>& ownedKanji, const String& cardId);
}

