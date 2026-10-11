#include "deck.h"

#include <algorithm>
#include <stdexcept>

namespace acard::cards {

void shuffleCards(
	std::vector<Card>& cards,
	std::mt19937& rng
) {
	std::shuffle(cards.begin(), cards.end(), rng);
}

std::vector<std::vector<Card>> dealRoundRobin(
	std::span<const Card> cards,
	std::size_t player_count,
	std::size_t first_player
) {
	// 验证参数
	if (player_count == 0) {
		throw std::invalid_argument(
			"player_count must be greater than zero"
		);
	}

	if (first_player >= player_count) {
		throw std::invalid_argument(
			"first_player must be less than player_count"
		);
	}

	// 外层数组的索引是玩家位置，内层数组保存了该玩家的手牌
	std::vector<std::vector<Card>> hands(player_count);

	const std::size_t base_count = cards.size() / player_count;
	const std::size_t extra_count = cards.size() % player_count;

	// 提前预留容量
	const std::size_t max_hand_size = base_count + (extra_count > 0 ? 1 : 0);

	for (auto& hand : hands) {
		// 发牌数不同时向上取整
		hand.reserve(max_hand_size);
	}

	// 按输入顺序逐张发牌
	std::size_t player = first_player;
	for (const Card& card : cards) {
		hands[player].push_back(card);

		player = (player + 1) % player_count;
	}

	return hands;

}


} // namespace acard::cards