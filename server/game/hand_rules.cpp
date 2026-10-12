#include "hand_rules.h"
#include "game_deck.h"
#include "game_types.h"

#include <array>
#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace acard::game {
	
namespace {

// 显示映射花色，拒绝非法枚举值
std::optional<std::size_t> suitIndex(cards::Suit suit) {
	switch (suit) {
	case cards::Suit::Clubs:
		return 0;
    case cards::Suit::Diamonds:
        return 1;
    case cards::Suit::Hearts:
        return 2;
    case cards::Suit::Spades:
        return 3;
    default:
        return std::nullopt;
    }
}

} // namespace

std::optional<int> rankStrength(cards::Rank rank) {
    switch (rank) {
    case cards::Rank::Five:
        return 0;
    case cards::Rank::Six:
        return 1;
    case cards::Rank::Seven:
        return 2;
    case cards::Rank::Eight:
        return 3;
    case cards::Rank::Nine:
        return 4;
    case cards::Rank::Ten:
        return 5;
    case cards::Rank::Jack:
        return 6;
    case cards::Rank::Queen:
        return 7;
    case cards::Rank::King:
        return 8;
    case cards::Rank::Ace:
        return 9;
    default:
        return std::nullopt;
    }
}

std::optional<HandPattern> recognizeHand(
    std::span<const cards::Card> selected
) {
    // 合法校验
    if (selected.empty() || selected.size() > 40) {
        // 空或超过 40 张，则非法
        return std::nullopt;
    }

    // 记录每种点数选了几张
    std::array<std::size_t, 10> rank_counts{ };

    // 记录 4 花色 × 10 点数
    std::array<std::array<bool, 10>, 4> seen{ };

    int lowest_strength = 10;
    int highest_strength = -1;
    cards::Rank highest_rank = cards::Rank::Five;

    for (const cards::Card& card : selected) {
        const auto strength = rankStrength(card.rank_);
        const auto suit_index = suitIndex(card.suit_);

        // 合法校验
        if (!strength || !suit_index) {
            return std::nullopt;
        }

        const auto rank_index = static_cast<std::size_t>(*strength);

        if (seen[*suit_index][rank_index]) {
            return std::nullopt;
        }

        seen[*suit_index][rank_index] = true;
        ++rank_counts[rank_index];

        if (*strength < lowest_strength) {
            lowest_strength = *strength;
        }

        if (*strength > highest_strength) {
            highest_strength = *strength;
            highest_rank = card.rank_;
        }
    }

    // 所有牌点数相同时
    if (lowest_strength == highest_strength) {
        switch (selected.size()) {
        case 1:
            return HandPattern{
                HandType::Single, 1, highest_rank
            };
        case 2:
            return HandPattern{
                HandType::Pair, 2, highest_rank
            };
        case 3:
            return HandPattern{
                HandType::Bomb, 3, highest_rank
            };
        case 4:
            return HandPattern{
                HandType::Thunder, 4, highest_rank
            };
        default:
            return std::nullopt;
        }
    }

    // 顺子 / 连对
    const int rank_span =
        highest_strength - lowest_strength + 1;

    if (rank_span < 3) {
        return std::nullopt;
    }

    bool is_straight = true;
    bool is_consecutive_pairs = true;

    // 最低到最高点数之间必须没有缺口。
    // 顺子每种点数恰好一张，连对每种点数恰好两张。
    for (int strength = lowest_strength;
        strength <= highest_strength;
        ++strength) {
        const auto count = rank_counts[
            static_cast<std::size_t>(strength)
        ];

        if (count != 1) {
            is_straight = false;
        }

        if (count != 2) {
            is_consecutive_pairs = false;
        }
    }

    if (is_straight) {
        return HandPattern{
            HandType::Straight,
            selected.size(),
            highest_rank
        };
    }

    if (is_consecutive_pairs) {
        return HandPattern{
            HandType::ConsecutivePairs,
            selected.size(),
            highest_rank
        };
    }

    return std::nullopt;
}

bool canBeat(
    const HandPattern& candidate,
    const HandPattern& target
) {
    const auto candidate_strength = rankStrength(candidate.highest_rank);
    const auto target_strength = rankStrength(target.highest_rank);

    // 合法校验
    if (!candidate_strength || !target_strength) {
        return false;
    }

    if (candidate.type == target.type) {
        // 顺子必须同张数，连对必须同对数。
        // 连对每对两张，因此比较总张数即可。
        if ((candidate.type == HandType::Straight ||
            candidate.type == HandType::ConsecutivePairs) &&
            candidate.card_count != target.card_count) {
            return false;
        }

        // 同类严格更大才允许管牌，花色不参与比较。
        return *candidate_strength > *target_strength;
    }

    // 不同类别时，雷可以压过炸弹和全部普通牌型。
    if (candidate.type == HandType::Thunder) {
        return true;
    }

    // 其他牌型不能压过雷。
    if (target.type == HandType::Thunder) {
        return false;
    }

    // 此时 target 不可能是雷。
    // 不同类别的炸弹可以压过全部普通牌型。
    if (candidate.type == HandType::Bomb) {
        return true;
    }

    // 普通牌型不能跨类别管牌，也不能压过炸弹。
    return false;
}

bool respectsIdentityPriority(
    std::span<const cards::Card> hand,
    std::span<const cards::Card> selected,
    std::size_t player_count
) {
    // 合法校验
    if (player_count != 3 && player_count != 4) {
        throw std::invalid_argument(
            "player_count must be three or four."
        );
    }

    std::size_t held_identity_count = 0;
    std::size_t selected_ace_count = 0;
    std::size_t selected_identity_count = 0;

    // 统计当前完整手牌中身份牌数量
    for (const cards::Card& card : hand) {
        if (isIdentityCard(card, player_count)) {
            ++held_identity_count;
        }
    }

    // 统计本次打出的 A，以及其中的身份牌
    for (const cards::Card& card : selected) {
        if (card.rank_ == cards::Rank::Ace) {
            ++selected_ace_count;
        }
        if (isIdentityCard(card, player_count)) {
            ++selected_identity_count;
        }
    }

    // 判断：是否先出了身份牌
    const std::size_t required_identity_count =
        std::min(held_identity_count, selected_ace_count);

    return selected_identity_count == required_identity_count;
}

}