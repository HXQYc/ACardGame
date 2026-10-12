/*
* @file		game_types.h
* @brief	单次出牌与手牌类别
*/

#pragma once

#include "cards/card.h"

#include <cstddef>

namespace acard::game {

// 手牌/牌型 类别
enum class HandType {
	Single,				// 单牌
	Pair,				// 对牌
	Straight,			// 顺子
	ConsecutivePairs,	// 连对
	Bomb,				// 炸弹(3)
	Thunder				// 雷(4)
};

// 单词出牌操作
struct HandPattern {
	HandType type;
	std::size_t card_count;

	// 单、对、炸、雷：相同牌的点数
	// 顺、连对：连续点数中最高的点数
	cards::Rank highest_rank;
};

} // namespace acard::game