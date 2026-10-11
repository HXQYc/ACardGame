/*
* @file		card.h
* @brief	纸牌类
* 
*/

#pragma once

#include <cstdint>

namespace acard::cards {

/*
* @brief	花色
* @details	包含四种花色，仅用于程序内部表示，不用于网络协议编码
*/
enum class Suit : std::uint8_t {
	Clubs,		// 梅花
	Diamonds,	// 方块
	Hearts,		// 红桃
	Spades		// 黑桃
};

/*
* @brief	点数
* @details	
*/
enum class Rank : std::uint8_t {
	Ace = 1,
	Two,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Ten,
	Jack,
	Queen,
	King
};


/*
* @class	Card
* @brief	表示一张扑克牌
* @details	包含花色和点数两个属性
*			使用时需初始化
*/
struct Card {
	Suit suit_;
	Rank rank_;

	// 相等判定：花色和点数均相等才相等
	// 大小判定由游戏规则决定
	bool operator==(const Card&) const = default;
};


} // namespace acard::cards