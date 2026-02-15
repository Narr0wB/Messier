
#ifndef MOVEPICKER_HPP
#define MOVEPICKER_HPP

#include <cassert>

#include "movegen/move.hpp"
#include "movegen/position.hpp"
#include "search/search.hpp"

static const int mvv_lva_lookup[NPIECE_TYPES][NPIECE_TYPES] = {
    /*           PAWN  KNIGHT BISHOP ROOK QUEEN KING */
    /* PAWN */   {105, 205,   225,   235, 805,  905},
    /* KNIGHT */ {104, 204,   224,   234, 804,  904},
    /* BISHOP */ {103, 203,   223,   233, 803,  903},
    /* ROOK */   {102, 202,   222,   232, 802,  902},
    /* QUEEN  */ {101, 201,   221,   231, 801,  901},
    /* KING  */  {100, 200,   220,   230, 800,  900},
};

#define GOOD_CAPTURE_THRESHOLD 10;
#define GOOD_QUIET_THRESHOLD   30;

enum Stage : int {
    MAIN_TT,
    CAPURE_INIT,
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

struct ExtMove : public Move {
    public:
        int score;
        inline bool operator>(const ExtMove& b) { return score > b.score; }
};

/* Static Exchange Evaluation */
template <Color Us>
int SEE(Position& pos, Square to);


/* Borrowed most of the ideas of this MovePicker from Stockfish */
class MovePicker {
    public:
        MovePicker(const Position&, const Search::SearchContext&, int, int, Move);
        MovePicker(const Position&, const Search::SearchContext&, int, Move);

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
        template <Color C>
        Move next() 
        {
        top: 
            switch (m_stage) {
                case MAIN_TT:
                case QUIESCENCE_TT:
                case EVASION_TT:
                    ++m_stage;
                    return m_ttmove;
                
                case QUIESCENCE_INIT:
                case CAPTURE_INIT: {
                    MoveList<GenType::CAPTURES, C> list(m_pos);

                    m_cur = m_end_bad_captures = m_moves;
                    m_end_cur = m_end_captures = m_end_generated = score(list);

                    std::sort(m_cur, m_end_cur);
                    ++m_stage;
                    goto top
                }

                case GOOD_CAPURES: {
                    Move m = select([&]() {
                        if (SEE(m_pos, m_cur->to()) > GOOD_CAPTURE_THRESHOLD)
                            return true;
                        
                        std::swap(*m_end_bad_captures++, *m_cur);
                        return false;
                    });
                    if (m != NO_MOVE) return m;
                    
                    ++m_stage;
                    // Intentional fallthrough to the next stage
                }

                case QUIET_INIT: {
                    MoveList<GenType::QUIETS, C> list(m_pos);
                    
                    m_end_cur = m_end_generated = score(list);

                    std::sort(m_cur, m_end_cur);
                    ++m_stage;
                    // Intentional fallthrough to the next stage
                }

                case GOOD_QUIETS: {
                    Move m = select([&]() { return m_cur->score > GOOD_QUIET_THRESHOLD; });
                    if (m != NO_MOVE) return m;

                    // Prepare the iterators to loop over bad captures 
                    m_cur = moves;
                    m_end_cur = m_end_bad_captures;

                    ++m_stage;
                    // Intentional fallthrough to the next stage
                }

                case BAD_CAPTURES: {
                    Move m = select([&]() { return true; });
                    if (m != NO_MOVE) return m;

                    // Prepare the iterators to loop over all quiets again 
                    m_cur = m_end_captures;
                    m_end_cur = m_end_generated;

                    ++m_stage:
                    // Intentional fallthrough to the next stage
                }

                case BAD_QUIETS: 
                    return select([&]() { return m_cur->score < GOOD_QUIET_THRESHOLD; });

                case EVATION_INIT: {
                    MoveList<GenType::EVASIONS, C> list(m_pos);

                    m_cur = moves;
                    m_end_cur = m_end_generated =  score(list);

                    ++m_stage;
                    // Intentional fallthrough to the next stage
                }

                case QUIESCENCE:
                case EVATION: 
                    return select([]() { return true; });
            }

            assert(false);
            return NO_MOVE;
        }

    private:
        const Position&              m_pos;
        const Search::SearchContext& m_ctx;
        int                          m_ply;
        int                          m_depth;
        Move                         m_ttmove;
        ExtMove                      m_moves[MAX_MOVES];
        ExtMove*                     m_cur, *m_end_cur, *m_end_bad_captures, *m_end_captures, *m_end_generated;
        Stage                        m_stage;

        inline ExtMove* begin() { return m_cur; }
        inline ExtMove* end()   { return m_end_cur; }

        template <typename Pred>
        Move select(Pred predicate) {
            for (; m_cur < m_end_cur; ++m_cur)
                if (*m_cur != m_ttmove && predicate())
                    return *m_cur++;
            
            return NO_MOVE;
        }

        /* This function both scores and inserts the moves of the provided move list into the MovePicker's internal engine */
        template <GenType type, Color C>
        ExtMove* score(MoveList<type, C>& list)
        {
            static_assert(type == GenType::CAPTURES || type == GenType::QUIETS || type == GenType::EVASIONS, "Incorrect type");

            Bitboard threat_by_lesser[NPIECE_TYPES] = {0}; 
            if constexpr (type == GenType::QUIETS) {
                threat_by_lesser[PAWN] = 0;
                threat_by_lesser[KNIGHT] = thread_by_lesser[BISHOP] = m_pos.attacks_by<PAWN, ~C>(); 
                threat_by_lesser[QUEEN] = threat_by_lesser[BISHOP] | m_pos.attacks_by<KNIGHT, ~C>() | m_pos.attacks_by<BISHOP, ~C>();
                threat_by_lesser[KING] = threat_by_lesser[QUEEN] | m_pos.attacks_by<QUEEN, ~C>();
            }

            ExtMove* it = m_cur;
            for (auto move& : list) {
                ExtMove& m = *it++;
                m = move;

                const Square    from     = m.from();
                const Square    to       = m.to();
                const Piece     pc       = m_pos.at(from);
                const PieceType pt       = type_of(pt);
                const Piece     captured = m_pos.at(to);
                
                if constexpr (type == GenType::CAPTURES) {
                    m.score = mvv_lva_lookup[type_of(pc)][type_of(captured)];
                }
                else if constexpr (type == GenType::QUIETS) {
                    // History heuristic
                    m.score += m_ctx.history_moves[from][to];
                    
                    // Assign a bonus for escaping a threat by a lesser piece
                    int v = (threat_by_lesser[pt] & (1 << to)) ? -30 : 40 * bool((threat_by_lesser[pt] & (1 << from)));
                    m.score += piece_value[pt] * v;
                }
                else {
                    // GenType::EVASIONS

                    if (m.flags & MoveFlags::CAPTURES)
                        m.score = piece_value[pt] + (1 << 20);
                    else 
                        m.score = m_ctx.history_moves[from][to];
                }
            }

            return it;
        }
};

#endif // MOVEPICKER_HPP