
#include <engine/search/Search.hpp>

static const int mvv_lva_lookup[NPIECE_TYPES][NPIECE_TYPES] = {
    /*           PAWN  KNIGHT BISHOP ROOK QUEEN KING */
    /* PAWN */   {105, 205,   225,   235, 805,  905},
    /* KNIGHT */ {104, 204,   224,   234, 804,  904},
    /* BISHOP */ {103, 203,   223,   233, 803,  903},
    /* ROOK */   {102, 202,   222,   232, 802,  902},
    /* QUEEN  */ {101, 201,   221,   231, 801,  901},
    /* KING  */  {100, 200,   220,   230, 800,  900},
};

namespace Search {

    int mvv_lva(const Move &m_, const Position &p_) {
        if (m_.flags() != MoveFlags::CAPTURE) {
            return 0;
        }

        PieceType attacker = type_of(p_.at(m_.from()));
        PieceType victim = type_of(p_.at(m_.to()));

        return mvv_lva_lookup[attacker][victim];
    }
   
    int score_move(const Move& m_, const std::shared_ptr<SearchContext>& ctx, int ply) {
        // Score the move from the previous iterative search pv higher 
        if (ctx->info.depth != 0 && m_ == ctx->data.pv_table[0][ply]) {
            return MAX_MOVE_SCORE;
        }

        if (m_.flags() == MoveFlags::CAPTURE) {
            return mvv_lva(m_, ctx->board) + 10000;
        }

        if (m_ == ctx->data.killer_moves[0][ply]) {
            return 9000;
        }
        if (m_ == ctx->data.killer_moves[1][ply]) {
            return 8000;
        }

        return ctx->data.history_moves[m_.from()][m_.to()];
    }

} // namespace Search
