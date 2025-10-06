#pragma once

#include "common.hpp"
#include "position.hpp"

#include <algorithm>

namespace Clockwork {

enum class EvalBound : u8 {
    FailVeryLow = 0,
    FailLow = 1,
    Exact = 2,
    FailHigh = 3,
    FailVeryHigh = 4,
    None = 5
};

constexpr u8 EVAL_BOUND_NB = static_cast<u8>(EvalBound::None);

inline EvalBound get_eval_bound(Value eval, Value alpha, Value beta) {
    return eval + 420 <= alpha ? EvalBound::FailVeryLow
         : eval <= alpha       ? EvalBound::FailLow
         : eval < beta         ? EvalBound::Exact
         : eval < beta + 420   ? EvalBound::FailHigh
                               : EvalBound::FailVeryHigh;
}

using MainHistory   = std::array<std::array<std::array<i32, 4>, 4096>, 2>;
using ContHistEntry = std::array<std::array<std::array<i32, 64>, 6>, 2>;
using ContHistory   = std::array<std::array<std::array<ContHistEntry, 64>, 6>, 2>;
// king can't get captured
using CaptHistory       = std::array<std::array<std::array<std::array<i32, 64>, 6>, 6>, 2>;
using CorrectionHistoryEntry = std::array<i32, EVAL_BOUND_NB>;
using CorrectionHistory = std::array<std::array<CorrectionHistoryEntry, 16384>, 2>;

constexpr i32 HISTORY_MAX                     = 16384;
constexpr u64 CORRECTION_HISTORY_ENTRY_NB     = 16384;
constexpr i32 CORRECTION_HISTORY_GRAIN        = 256;
constexpr i32 CORRECTION_HISTORY_WEIGHT_SCALE = 512;
constexpr i32 CORRECTION_HISTORY_MAX          = CORRECTION_HISTORY_GRAIN * 32;

namespace Search {
struct Stack;
}

class History {
public:
    History() = default;

    ContHistEntry& get_cont_hist_entry(const Position& pos, Move move) {
        usize     stm_idx = static_cast<usize>(pos.active_color());
        PieceType pt      = pos.piece_at(move.from());
        usize     pt_idx  = static_cast<usize>(pt) - static_cast<usize>(PieceType::Pawn);
        return m_cont_hist[stm_idx][pt_idx][move.to().raw];
    }

    i32  get_conthist(const Position& pos, Move move, i32 ply, Search::Stack* ss) const;
    i32  get_quiet_stats(const Position& pos, Move move, i32 ply, Search::Stack* ss) const;
    void update_quiet_stats(const Position& pos, Move move, i32 ply, Search::Stack* ss, i32 bonus);

    i32  get_noisy_stats(const Position& pos, Move move) const;
    void update_noisy_stats(const Position& pos, Move move, i32 bonus);

    void update_correction_history(const Position& pos, i32 depth, i32 diff, EvalBound eval_bound);
    i32  get_correction(const Position& pos, EvalBound eval_bound);

    void clear();

private:
    static void update_hist_entry(i32& entry, i32 bonus) {
        entry += bonus - entry * std::abs(bonus) / HISTORY_MAX;
    }

    static void update_hist_entry_banger(i32& entry, i32 base, i32 bonus) {
        entry += bonus - base * std::abs(bonus) / HISTORY_MAX;
        entry = std::clamp(entry, -2 * HISTORY_MAX, 2 * HISTORY_MAX);
    }

    MainHistory                      m_main_hist          = {};
    ContHistory                      m_cont_hist          = {};
    CaptHistory                      m_capt_hist          = {};
    CorrectionHistory                m_pawn_corr_hist     = {};
    std::array<CorrectionHistory, 2> m_non_pawn_corr_hist = {};
    CorrectionHistory                m_major_corr_hist    = {};
};

}
