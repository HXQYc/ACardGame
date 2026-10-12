/*
* @file	game_deck.h
* @brief	生成捉黑 A 的牌组，并判断身份牌
*/

#pragma once

#include "cards/card.h"

#include <cstddef>
#include <vector>

namespace acard::game {

/*
* @brief 生成捉黑 A 使用的 40 张牌。
* @details 每种花色包含 5～K 和 A，不包含 2、3、4及大小王。
*          返回固定顺序的牌组，不负责洗牌或发牌。
*/
std::vector<cards::Card> makeGameDeck();

/*
    * @brief 判断一张牌是否为当前玩法的身份牌。
    * @param card 待判断的牌。
    * @param player_count 本局人数，只允许 3 或 4。
    * @return 三人局：黑桃 A；四人局：黑桃 A、梅花 A。
    * @throws std::invalid_argument 人数不是 3 或 4。
    */
bool isIdentityCard(
    const cards::Card& card,
    std::size_t player_count
);
} // namespace acard::game