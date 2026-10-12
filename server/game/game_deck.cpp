#include "game_deck.h"

#include <array>
#include <stdexcept>

namespace acard::game {

std::vector<cards::Card> makeGameDeck() {
	constexpr std::array<cards::Suit, 4> suits{
		cards::Suit::Clubs,
		cards::Suit::Diamonds,
		cards::Suit::Hearts,
		cards::Suit::Spades
	};

	constexpr std::array<cards::Rank, 10> ranks{
		cards::Rank::Five,
		cards::Rank::Six,
		cards::Rank::Seven,
		cards::Rank::Eight,
		cards::Rank::Nine,
		cards::Rank::Ten,
		cards::Rank::Jack,
		cards::Rank::Queen,
		cards::Rank::King,
		cards::Rank::Ace
	};

	std::vector<cards::Card> deck;

	deck.reserve(suits.size() * ranks.size());

	for (const cards::Suit suit : suits) {
		for (const cards::Rank rank : ranks) {
			deck.push_back(cards::Card{ suit, rank });
		}
	}

	return deck;
}

bool isIdentityCard(
	const cards::Card& card,
	std::size_t player_count
) {
	if (player_count != 3 && player_count != 4) {
		throw std::invalid_argument(
			"player_count must be three or four."
		);
	}

	if (card.rank_ != cards::Rank::Ace) {
		return false;
	}

	// 黑桃 A 在三/四人局中都是身份牌
	if (card.suit_ == cards::Suit::Spades) {
		return true;
	}

	// 梅花 A 仅在四人局中是身份牌
	return player_count == 4 && card.suit_ == cards::Suit::Clubs;
}

} // namespace acard::game
