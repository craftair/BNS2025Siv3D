#pragma once
#include "CardData.h"
#include <functional>
#include "Common.h"

struct CardHandConfig
{
	Vec2 virtualSize{ 1280, 720 };
	double margin = 32.0;
	double gap = 18.0;
	size_t columns = 4;
	double activationOffset = 40.0;
};

class CardSystem
{
public:
	CardSystem(GameData gameData);
	bool initialize(const FilePathView& libraryPath, const Array<String>& deckIds, const CardHandConfig& config = {});
	void update();
	void draw() const;
	void resetUsage();
	const Array<String>& playLog() const { return m_playLog; }
	void setCardPlayCallback(std::function<void(const String& cardId)> callback) { m_cardPlayCallback = callback; }
	void setEndTurnCallback(std::function<void()> callback) { m_endTurnCallback = callback; }
	void setCardValidationCallback(std::function<bool(const String& cardId)> callback) { m_cardPlayValidator = callback; }
	void setInputSuppressed(bool suppressed);
	bool inputSuppressed() const { return m_inputSuppressed; }
	size_t drawCards(size_t count);
	bool discardCardInHand(size_t deckIndex);
	size_t discardHand();
	void shuffleAllAndDraw(size_t drawCount);
	bool addDeckCardToHand(size_t deckIndex);
	Optional<size_t> handCardAtScreenPos(const Vec2& screenPos) const;
	const Array<size_t>& handOrder() const { return m_handIndices; }
	const Array<size_t>& deckOrder() const { return m_drawPile; }
	const RectF& deckButtonRect() const { return m_deckButtonScreen; }
	bool isDeckViewOpen() const { return m_showDeck; }
	bool isTrashViewOpen() const { return m_showTrash; }
	bool isOverlayOpen() const { return m_showDeck || m_showTrash; }
	const CardInstance* instanceAt(size_t deckIndex) const;
	Array<String> sampleCardIds(size_t count) const;
	bool addCardToDeck(const String& cardId);
	bool removeCardFromDeck(const String& cardId, size_t count = 1);
	const Texture* textureForCard(const String& cardId) const;
	const CardDefinition* findCardDefinition(const String& cardId) const;
	Array<String> allCardIds() const;

private:
	void loadTextures();
	void layoutHand();
	void resetDeckState();
	size_t drawFromDeck(size_t count);
	void removeFromHand(size_t cardIndex);
	void drawHand(size_t desiredCount);
	void endTurn();
	void reloadDeckFromTrash();
	void updateTransform();
	void updateCards();
	void updateDragging(const Vec2& cursorVirtual);
	void drawScene() const;
	void drawCard(const CardInstance& card) const;
	void drawUI() const;
	Vec2 toVirtual(const Vec2& screenPos) const;
	bool isInsideVirtual(const Vec2& pos) const;

	CardHandConfig m_config;

	Vec2 m_virtualSize{ 1280, 720 };
	double m_scale = 1.0;
	Vec2 m_offset{ 0, 0 };
	double m_activationLine = 260.0;
	Optional<size_t> m_draggingIndex;

	CardLibrary m_library;
	CardDeck m_deck;
	Array<String> m_playLog;
	HashTable<String, Texture> m_textures;
	Font m_bodyFont{ 18 };
	Texture m_deckButtonTexture;
	Texture m_trashButtonTexture;
	Texture m_endTurnTexture;

	Array<size_t> m_trashOrder;
	bool m_showTrash = false;
	bool m_trashJustOpened = false;
	bool m_showDeck = false;
	bool m_deckJustOpened = false;
	RectF m_trashButtonScreen{ 0, 0, 0, 0 };
	RectF m_endTurnButtonScreen{ 0, 0, 0, 0 };
	RectF m_deckButtonScreen{ 0, 0, 0, 0 };
	Vec2 m_trashModalSize{ 940, 580 };
	Vec2 m_deckModalSize{ 940, 580 };
	RectF m_trashModalRect{ 0, 0, 0, 0 };
	RectF m_deckModalRect{ 0, 0, 0, 0 };
	RectF m_trashCloseButton{ 0, 0, 0, 0 };
	RectF m_deckCloseButton{ 0, 0, 0, 0 };
	double m_trashScroll = 0.0;
	double m_deckScroll = 0.0;
	double m_trashScrollMax = 0.0;
	double m_deckScrollMax = 0.0;
	RectF m_trashContentRect{ 0, 0, 0, 0 };
	RectF m_deckContentRect{ 0, 0, 0, 0 };
	RectF m_trashScrollbarTrack{ 0, 0, 0, 0 };
	RectF m_deckScrollbarTrack{ 0, 0, 0, 0 };
	RectF m_trashScrollbarThumb{ 0, 0, 0, 0 };
	RectF m_deckScrollbarThumb{ 0, 0, 0, 0 };
	RectF m_trashScrollUpButton{ 0, 0, 0, 0 };
	RectF m_trashScrollDownButton{ 0, 0, 0, 0 };
	RectF m_deckScrollUpButton{ 0, 0, 0, 0 };
	RectF m_deckScrollDownButton{ 0, 0, 0, 0 };
	bool m_trashScrollbarDragging = false;
	bool m_deckScrollbarDragging = false;
	double m_trashScrollbarGrabOffset = 0.0;
	double m_deckScrollbarGrabOffset = 0.0;
	Array<size_t> m_drawPile;
	Array<size_t> m_handIndices;
	bool m_inputSuppressed = false;
	Optional<size_t> m_hoverCardIndex;
	double m_hoverLift = 34.0;
	std::function<void(const String& cardId)> m_cardPlayCallback;
	std::function<void()> m_endTurnCallback;
	std::function<bool(const String& cardId)> m_cardPlayValidator;

	GameData m_gameData;
	Audio seCursor1{ U"resources/audio/cursor1.ogg" };
};
