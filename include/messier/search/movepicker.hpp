
#ifndef MOVEPICKER_HPP
#define MOVEPICKER_HPP

#include <cassert>
#include <algorithm>
#include <functional>
#include <utility>

#include <messier/movegen/move.hpp>
#include <messier/movegen/position.hpp>
#include <messier/search/parameters.hpp>
#include <messier/search/search.hpp>
#include <messier/log.hpp>

static const int mvv_lva_lookup[NPIECE_TYPES][NPIECE_TYPES] = {
    /*           PAWN  KNIGHT BISHOP ROOK QUEEN KING */
    /* PAWN   */ {105, 205,   225,   235, 805,  905},
    /* KNIGHT */ {104, 204,   224,   234, 804,  904},
    /* BISHOP */ {103, 203,   223,   233, 803,  903},
    /* ROOK   */ {102, 202,   222,   232, 802,  902},
    /* QUEEN  */ {101, 201,   221,   231, 801,  901},
    /* KING   */ {100, 200,   220,   230, 800,  900},
};

static const int promotion_base[5] = { 1 << 17, 1 << 11, MAX_HISTORY + 1, 1 << 11, 0 };

enum class Stage : int {
    MAIN_TT,
    CAPTURE_INIT,
    GOOD_CAPTURES,
    QUIET_INIT,
    GOOD_QUIETS,
    BAD_CAPTURES,
    BAD_QUIETS,

    QUIESCENCE_TT,
    QUIESCENCE_INIT,
    QUIESCENCE,

    EVASION_TT,
    EVASION_INIT,
    EVASION,
};

inline Stage& operator++(Stage& s) { s = static_cast<Stage>(static_cast<int>(s) + 1); return s; }

struct ExtMove : public Move {
    public:
        int score;

        ExtMove(const Move& m) : score(0), Move(m) {}
        ExtMove() : score(0), Move(0) {}

        inline bool operator>(const ExtMove& b) const { return score > b.score; }
        inline bool operator<(const ExtMove& b) const { return score < b.score; }
};

/* Borrowed most of the ideas of this MovePicker from Stockfish */
template <Color C>
class MovePicker {
public:
    MovePicker(Position& pos, const Search::SearchContext& ctx, int ply, int depth, bool in_check, Move tt_move) :
        m_pos(pos),
        m_ctx(ctx),
        m_ply(ply),
        m_depth(depth),
        m_ttmove(tt_move),
        m_end_good_quiets(nullptr)
    {
        if (in_check) {
            m_stage = Stage::EVASION_TT;
        }
        else if (depth > 0) {
            m_stage = Stage::MAIN_TT;
        }
        else {
            m_stage = tt_move.is_quiet() ? Stage::QUIESCENCE_INIT : Stage::QUIESCENCE_TT;
        }
    };

    /* 
        More or less, in normal tree-search the state of the MovePicker behaves as follows:
            TT move,
            Generate all captures,
            Iterate over all "good" captures,
            Generate all quiets,
            Iterate over all "good" quiets,
            Iterate over all "bad" captures,
            Iterate over all "bad" quiets

        In the case of an evasion situation, we have:
            TT move,
            Generate all evasions,
            Iterate over all possible evasions

        In the case of quiescence-search, we have:
            TT move,
            Generate all caputres,
            Iterate over all possible captures
    */
    Move next() 
    {
    top: 
        switch (m_stage) {
            case Stage::MAIN_TT:
            case Stage::QUIESCENCE_TT:
            case Stage::EVASION_TT: {
                ++m_stage;
                if (m_ttmove != Move::none() 
                    && m_pos.is_pseudo_legal(m_ttmove) 
                    && m_pos.is_legal<C>(m_ttmove)) 
                return m_ttmove;
                goto top;
            }
                
            case Stage::CAPTURE_INIT: {
                MoveList<GenType::CAPTURES, C> list(m_pos);

                m_cur = m_end_bad_captures = m_moves;
                m_end_cur = m_end_captures = m_end_generated = score(list);

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::QUIESCENCE_INIT: {
                MoveList<GenType::QUIESCENCE, C> list(m_pos);

                m_cur = m_end_bad_captures = m_moves;
                m_end_cur = m_end_captures = m_end_generated = score(list);

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::GOOD_CAPTURES: {
                Move m = select([&]() {
                    if (m_pos.see<C>(*m_cur, good_capture_threshold))
                        return true;
                        
                    // Since re-searching the array of capures once again is expensive, we move the bad captures to the front
                    // and we keep track of the end of the list of bad captures
                    std::swap(*m_end_bad_captures++, *m_cur);
                    return false;
                });

                if (m != Move::none()) 
                    return m;
                    
                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::QUIET_INIT: {
                assert(m_cur == m_end_captures);
                MoveList<GenType::QUIETS, C> list(m_pos);
                    
                m_end_cur = m_end_generated = score(list);

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::GOOD_QUIETS: {
                Move m = select<true>([&]() { return m_cur->score > good_quiet_threshold; });
                if (m != Move::none()) 
                    return m;

                m_end_good_quiets = m_cur;

                // Prepare the iterators to loop over bad captures 
                m_cur = m_moves;
                m_end_cur = m_end_bad_captures;

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::BAD_CAPTURES: {
                Move m = select([&]() { return true; });
                if (m != Move::none()) return m;

                // Prepare the iterators to loop over all quiets again 
                m_cur = m_end_good_quiets;
                m_end_cur = m_end_generated;

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::BAD_QUIETS:  {
                return select([&]() { return true; });
            }

            case Stage::EVASION_INIT: {
                MoveList<GenType::EVASIONS, C> list(m_pos);

                m_cur = m_moves;
                m_end_cur = m_end_generated = score(list);

                m_stage_counter = 0;
                ++m_stage;
                goto top;
            }

            case Stage::QUIESCENCE: {
                Move m = select([]() { return true; });
                return m;
            }

            case Stage::EVASION: {
                Move m = select([]() { return true; });
                return m;
            }
        }

        assert(false);
        return Move::none();
    }

private:
    Position&                    m_pos;
    const Search::SearchContext& m_ctx;
    int                          m_ply;
    int                          m_depth;
    Move                         m_ttmove;
    ExtMove                      m_moves[MAX_MOVES];
    ExtMove*                     m_cur, *m_end_cur, *m_end_bad_captures, *m_end_captures, *m_end_good_quiets, *m_end_generated;
    Stage                        m_stage;
    int                          m_stage_counter;

    inline ExtMove* begin() { return m_cur; }
    inline ExtMove* end()   { return m_end_cur; }

    template <bool stop_on_rejection = false, typename Pred>
    Move select(Pred predicate) {
        if (m_stage_counter == moves_before_sorting)
            std::sort(m_cur, m_end_cur, std::greater<ExtMove>());

        for (; m_cur < m_end_cur; ++m_cur) {
            if (m_stage_counter < moves_before_sorting) {
                ExtMove *best = m_cur;

                for (ExtMove *it = m_cur; it < m_end_cur; ++it)
                    if (it->score > best->score)
                        best = it;

                std::swap(*best, *m_cur);
            }

            if (*m_cur == m_ttmove)
                continue;

            if (predicate()) {
                m_stage_counter++;
                return *m_cur++;
            }
            
            if constexpr (stop_on_rejection)
                return Move::none();
        }
            
        return Move::none();
    }

    /* This function both scores and inserts the moves of the provided move list into the MovePicker's internal engine */
    template <GenType type>
    ExtMove* score(MoveList<type, C>& list)
    {
        static_assert(type == GenType::CAPTURES || type == GenType::QUIETS || type == GenType::EVASIONS || type == GenType::QUIESCENCE, "Incorrect type");

        Bitboard threat_by_lesser[NPIECE_TYPES] = {0}; 
        // if constexpr (type == GenType::QUIETS) {
        //     threat_by_lesser[PAWN] = 0;
        //     threat_by_lesser[KNIGHT] = threat_by_lesser[BISHOP] = m_pos.attacks_by<PAWN, ~C>(); 
        //     threat_by_lesser[ROOK] = threat_by_lesser[BISHOP] | m_pos.attacks_by<BISHOP, ~C>() | m_pos.attacks_by<KNIGHT, ~C>();
        //     threat_by_lesser[QUEEN] = threat_by_lesser[ROOK] | m_pos.attacks_by<ROOK, ~C>();
        //     threat_by_lesser[KING] = threat_by_lesser[QUEEN] | m_pos.attacks_by<QUEEN, ~C>();
        // }

        ExtMove* it = m_cur;
        for (auto move : list) {
            ExtMove& m = *it++;
            m = move;

            const Square    from     = m.from();
            const Square    to       = m.to();
            const Piece     pc       = m_pos.at(from);
            const PieceType pt       = type_of(pc);
            const Piece     captured = m.is_enpassant() ? m_pos.at(to + relative_dir<C>(SOUTH)) : m_pos.at(to);

            if constexpr (type == GenType::CAPTURES) {
                PieceType captured_type = captured == NO_PIECE ? PAWN : type_of(captured);
                m.score = mvv_lva_lookup[type_of(pc)][captured_type];
            }

            else if constexpr (type == GenType::QUIETS) {
                // History heuristic
                m.score = m_ctx.quiet.board[from][to][static_cast<size_t>(C)];

                // Killer heuristic
                m.score += m_ctx.killer.moves[m_ply][0] == m ? (1 << 16) : 0;
                m.score += m_ctx.killer.moves[m_ply][1] == m ? (1 << 16) - 1 : 0;

                // Assign a bonus for escaping a threat by a lesser piece
                // int v = (threat_by_lesser[pt] & (1ULL << to)) ? 
                //     -30 : 40 * bool((threat_by_lesser[pt] & (1ULL << from)));
                // m.score += piece_value[pt] * v;
            }

            else if constexpr (type == GenType::QUIESCENCE) {
                if (m.is_capture()) {
                    PieceType captured_type = captured == NO_PIECE ? PAWN : type_of(captured);
                    m.score = mvv_lva_lookup[type_of(pc)][captured_type];
                }
            }

            else if constexpr (type == GenType::EVASIONS) {
                if (m.is_capture())
                    m.score = (1 << 20) + mvv_lva_lookup[pt][type_of(captured)];
                else 
                    m.score = m_ctx.quiet.board[from][to][static_cast<size_t>(C)];
            }

            if (m.is_promotion())
                m.score += promotion_base[static_cast<int>(type)] + piece_value[m.promotion()] - piece_value[PAWN];
        }

        return it;
    }
};

#endif // MOVEPICKER_HPP