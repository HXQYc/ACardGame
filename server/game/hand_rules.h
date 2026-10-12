/*
* @file		hand_rules.h
* @brief	点数强度、牌型识别、管牌比较
*/

#pragma once

#include "game_types.h"

#include <cstddef>
#include <optional>
#include <span>

namespace acard::game {

/*
* @brief	点数强度
* @details	5~K == 0~8， A == 9；
*			2、3、4及非法枚举返回 std::nullopt
*/
std::optional<int> rankStrength(cards::Rank rank);

/*
* @brief	识别牌型
* @details	输入顺序不影响结果，也不修改输入
*			以下场景返回 std::nullopt
*			- 空牌
*			- 非法花色或本游戏不允许的点数
*			- 同一实体牌重复出现
*			- 无法组成合法牌型
*/
std::optional<HandPattern> recognizeHand(
    std::span<const cards::Card> selected
);

/*
* @brief    判断牌型大小
* @details  输入两个成功识别牌型的参数
*/
bool canBeat(
    const HandPattern& candidate,
    const HandPattern& target
);

} // namespace acard::game