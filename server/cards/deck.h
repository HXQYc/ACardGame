/*
* @file		deck.h
* @brief	牌堆类
* @details	负责发票洗牌等
*/

#pragma once

#include "card.h"

#include <cstdint>	// 位宽整数
#include <cstddef>
#include <random>	// 随机数工具
#include <span>		// 不拥有数据的连续区域视图
#include <vector>	// 

namespace acard::cards {

/*
* @brief	原地打乱牌的顺序
* @details	原地打乱，不增加或删除牌
*			- 随机数生成器由调用者管理
*			- 允许传入空牌组
*/
void shuffleCards(
	std::vector<Card>& cards,
	std::mt19937& rng
);

/*
* @brief	发牌
* @details	按顺序发完传入的牌组
* @return	外层数组的索引是玩家位置，内层数组保存了该玩家的手牌
* @param	cards	传入的牌组
* @param	player_count	玩家数
* @param	first_player	第一张牌的接收位置，严格小于 player_count
*/
std::vector<std::vector<Card>> dealRoundRobin(
	std::span<const Card> cards,
	std::size_t player_count,
	std::size_t first_player
);

} // namespace acard::cards