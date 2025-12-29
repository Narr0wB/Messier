
#include "evaluate.hpp"

using namespace std::chrono_literals;

int evaluate(Position& position) 
{
    int score = 0;

    for (int p = 0; p < NPIECE_TYPES; ++p) {
        Bitboard white_piece_bb = position.bitboard_of(make_piece(WHITE, (PieceType)p));
        Bitboard black_piece_bb = position.bitboard_of(make_piece(BLACK, (PieceType)p));

        while (white_piece_bb) {   
            Square piece_sq = pop_lsb(&white_piece_bb);
            if (p != PieceType::KING) score += piece_position_value[p][piece_sq];
            score += piece_value[p];
        }

        while (black_piece_bb) {
            Square piece_sq = pop_lsb(&black_piece_bb);
            if (p != PieceType::KING) score -= piece_position_value[p + 5][piece_sq];
            score -= piece_value[p];
        }
    }

    return score;
}

int score_move(const Move& m_, const Search::SearchContext& ctx, const Position& pos, int ply, Move ttmove) 
{
    // Score the move from the previous iterative search pv higher 
    if (m_ == ctx.pv_table[0][ply]) {
        return MAX_MOVE_SCORE;
    }

    else if (m_ == ttmove) {
        return MAX_MOVE_SCORE - 1000; 
    }

    else if (m_.flags() == MoveFlags::CAPTURE) {
        return mvv_lva(m_, pos) + 1000;
    }

    else if (m_ == ctx.killer_moves[ply][0]) {
        return 900;
    }

    else if (m_ == ctx.killer_moves[ply][1]) {
        return 800;
    }

    return ctx.history_moves[m_.from()][m_.to()];
}

static const int mvv_lva_lookup[NPIECE_TYPES][NPIECE_TYPES] = {
    /*           PAWN  KNIGHT BISHOP ROOK QUEEN KING */
    /* PAWN */   {105, 205,   225,   235, 805,  905},
    /* KNIGHT */ {104, 204,   224,   234, 804,  904},
    /* BISHOP */ {103, 203,   223,   233, 803,  903},
    /* ROOK */   {102, 202,   222,   232, 802,  902},
    /* QUEEN  */ {101, 201,   221,   231, 801,  901},
    /* KING  */  {100, 200,   220,   230, 800,  900},
};

int mvv_lva(const Move &m_, const Position &p_) 
{
    if (m_.flags() != MoveFlags::CAPTURE) {
        return 0;
    }

    PieceType attacker = type_of(p_.at(m_.from()));
    PieceType victim = type_of(p_.at(m_.to()));

    return mvv_lva_lookup[attacker][victim];
}

// int piece_to_idx(Piece& p) {
//     switch (p)
//     {
//     case NO_PIECE:
//         return -1;
//         break;
//     case WHITE_PAWN:
//         return 0;
//         break;
//     case WHITE_KNIGHT:
//         return 1;
//         break;
//     case WHITE_BISHOP:
//         return 2;
//         break;
//     case WHITE_ROOK:
//         return 3;
//         break;
//     case WHITE_QUEEN:
//         return 4;
//         break;
//     case WHITE_KING:
//         return 5;
//         break;
//     case BLACK_PAWN:
//         return 0;
//         break;
//     case BLACK_KNIGHT:
//         return 1;
//         break;
//     case BLACK_BISHOP:
//         return 2;
//         break;
//     case BLACK_ROOK:
//         return 3;
//         break;
//     case BLACK_QUEEN:
//         return 4;
//         break;
//     case BLACK_KING:
//         return 5;
//         break;
//     }
// }

// int piece_color(Piece& p) {
//     switch (p)
//     {
//     case NO_PIECE:
//         return -1;
//         break;
//     case WHITE_PAWN:
//         return 0;
//         break;
//     case WHITE_KNIGHT:
//         return 0;
//         break;
//     case WHITE_BISHOP:
//         return 0;
//         break;
//     case WHITE_ROOK:
//         return 0;
//         break;
//     case WHITE_QUEEN:
//         return 0;
//         break;
//     case WHITE_KING:
//         return 0;
//         break;
//     case BLACK_PAWN:
//         return 1;
//         break;
//     case BLACK_KNIGHT:
//         return 1;
//         break;
//     case BLACK_BISHOP:
//         return 1;
//         break;
//     case BLACK_ROOK:
//         return 1;
//         break;
//     case BLACK_QUEEN:
//         return 1;
//         break;
//     case BLACK_KING:
//         return 1;
//         break;
//     }
// }

// int piece_value(bool mg, Piece& square) {
//     int pV = 0;
//     switch (square) {
//     case NO_PIECE:
//         break;
//     case WHITE_PAWN:
//         mg ? pV = 124 : pV = 206;
//         break;
//     case WHITE_KNIGHT:
//         mg ? pV = 781 : pV = 854;
//         break;
//     case WHITE_BISHOP:
//         mg ? pV = 825 : pV = 915;
//         break;
//     case WHITE_ROOK:
//         mg ? pV = 1276 : pV = 1380;
//         break;
//     case WHITE_QUEEN:
//         mg ? pV = 2538 : pV = 2682;
//         break;
//     case BLACK_PAWN:
//         mg ? pV = 124 : pV = 206;
//         break;
//     case BLACK_KNIGHT:
//         mg ? pV = 781 : pV = 854;
//         break;
//     case BLACK_BISHOP:
//         mg ? pV = 825 : pV = 915;
//         break;
//     case BLACK_ROOK:
//         mg ? pV = 1276 : pV = 1380;
//         break;
//     case BLACK_QUEEN:
//         mg ? pV = 2538 : pV = 2682;
//         break;
//     }
    
//     return pV;
// }

// int psqt_value(bool mg, Piece& square, int sq) {
//     int rank = (int)sq / 8;
//     int file = sq % 8;
//     int bonus[5][8][4] = {
//         {{-175, -92, -74, -73}, {-77, -41, -27, -15}, {-61, -17, 6, 12}, {-35, 8, 40, 49}, {-34, 13, 44, 51}, {-9, 22, 58, 53}, {-67, -27, 4, 37}, {-201, -83, -56, -26}},
//             {{-53, -5, -8, -23}, {-15, 8, 19, 4}, {-7, 21, -5, 17}, {-5, 11, 25, 39}, {-12, 29, 22, 31}, {-16, 6, 1, 11}, {-17, -14, 5, 0}, {-48, 1, -14, -23}},
//             {{-31, -20, -14, -5}, {-21, -13, -8, 6}, {-25, -11, -1, 3}, {-13, -5, -4, -6}, {-27, -15, -4, 3}, {-22, -2, 6, 12}, {-2, 12, 16, 18}, {-17, -19, -1, 9}},
//             {{3, -5, -5, 4}, {-3, 5, 8, 12}, {-3, 6, 13, 7}, {4, 5, 9, 8}, {0, 14, 12, 5}, {-4, 10, 6, 8}, {-5, 6, 10, 8}, {-2, -2, 1, -2}},
//             {{271, 327, 271, 198}, {278, 303, 234, 179}, {195, 258, 169, 120}, {164, 190, 138, 98}, {154, 179, 105, 70}, {123, 145, 81, 31}, {88, 120, 65, 33}, {59, 89, 45, -1}}
//     }; 
//     int bonus2[5][8][4] = {
//         {{-96, -65, -49, -21}, {-67, -54, -18, 8}, {-40, -27, -8, 29}, {-35, -2, 13, 28}, {-45, -16, 9, 39}, {-51, -44, -16, 17}, {-69, -50, -51, 12}, {-100, -88, -56, -17}},
//             {{-57, -30, -37, -12}, {-37, -13, -17, 1}, {-16, -1, -2, 10}, {-20, -6, 0, 17}, {-17, -1, -14, 15}, {-30, 6, 4, 6}, {-31, -20, -1, 1}, {-46, -42, -37, -24}},
//             {{-9, -13, -10, -9}, {-12, -9, -1, -2}, {6, -8, -2, -6}, {-6, 1, -9, 7}, {-5, 8, 7, -6}, {6, 1, -7, 10}, {4, 5, 20, -5}, {18, 0, 19, 13}},
//             {{-69, -57, -47, -26}, {-55, -31, -22, -4}, {-39, -18, -9, 3}, {-23, -3, 13, 24}, {-29, -6, 9, 21}, {-38, -18, -12, 1}, {-50, -27, -24, -8}, {-75, -52, -43, -36}},
//             {{1, 45, 85, 76}, {53, 100, 133, 135}, {88, 130, 169, 175}, {103, 156, 172, 172}, {96, 166, 199, 199}, {92, 172, 184, 191}, {47, 121, 116, 131}, {11, 59, 73, 78}}
//     };
//     int pbonus[8][8] = { {0, 0, 0, 0, 0, 0, 0, 0}, {3, 3, 10, 19, 16, 19, 7, -5}, {-9, -15, 11, 15, 32, 22, 5, -22}, {-4, -23, 6, 20, 40, 17, 4, -8}, {13, 0, -13, 1, 11, -2, -13, 5},
//         {5, -12, -7, 22, -8, -5, -15, -8}, {-7, 7, -3, -13, 5, -16, 10, -8}, {0, 0, 0, 0, 0, 0, 0, 0} };
//     int pbonus2[8][8] = {{0, 0, 0, 0, 0, 0, 0, 0}, {-10, -6, 10, 0, 14, 7, -5, -19}, {-10, -10, -10, 4, 4, 3, -6, -4}, {6, -2, -8, -4, -13, -12, -10, -9}, {10, 5, 4, -5, -5, -5, 14, 9},
//         {28, 20, 21, 28, 30, 7, 6, 13}, {0, -11, 12, 21, 25, 19, 4, 7}, {0, 0, 0, 0, 0, 0, 0, 0}};
//     int idx = piece_to_idx(square);
//     if (idx < 0) return 0;
//     if (idx == 1) return mg ? pbonus[7 - rank][file] : pbonus2[7 - rank][file];
//     else return mg ? bonus[idx - 1][7 - rank][std::min(file, 7 - file)] : bonus2[idx - 1][7 - rank][std::min(file, 7 - file)];

// }

// int imbalance(Piece& square, Piece* pieces) {
//     int qo[6][6] = {{0, 0, 0, 0, 0, 0}, {40, 38, 0, 0, 0, 0}, {32, 255, -62, 0, 0, 0}, {0, 104, 4, 0, 0, 0}, {-26, -2, 47, 105, -208, 0}, {-189, 24, 117, 133, -134, -6}};
//     int qt[6][6] = {{0, 0, 0, 0, 0, 0}, {36, 0, 0, 0, 0, 0}, {9, 63, 0, 0, 0, 0}, {59, 65, 42, 0, 0, 0}, {46, 39, 24, -24, 0, 0}, {97, 100, -42, 137, 268, 0}};
//     int idx = piece_to_idx(square);
//     if (idx < 0) return 0;
//     int bishop[2] = { 0, 0 }; int eval;
//     for (uint16_t i = 0; i < 64; i++) {
//         int idxL = piece_to_idx(pieces[i]);
//         if (idxL < 0) continue;
//         if (pieces[i] == BLACK_BISHOP) bishop[0]++;
//         if (pieces[i] == WHITE_BISHOP) bishop[1]++;
//         if (idxL > idx) continue;
//         if (piece_color(pieces[i]) == BLACK) eval += qt[idx][idxL];
//         else eval += qo[idx][idxL];
//     }
//     if (bishop[0] > 1) eval += qt[idx][0];
//     if (bishop[1] > 1) eval += qo[idx][0];

//     if (piece_color(square) == BLACK) return -eval;

//     return eval;
// }

// int imbalance_t(Piece& square, Piece* pieces) {
//     int eval = 0;
//     eval += imbalance(square, pieces)/16;
//     return eval;
// }

// int middle_game_eval(Position& b) {
//     int eval = 0;
//     Piece* pieces = b.getPieces();
//     for (int i = 0; i < NSQUARES; i++) {
//         if (piece_color(pieces[i]) == 0) {
//             eval += piece_value(true, pieces[i]);
//             eval += psqt_value(true, pieces[i], i);
//             eval += imbalance_t(pieces[i], pieces);
//         }
//     }
//     return eval;
// }

// // bool move_sorting_criterion(Move& a, Move& b) {
// //     return a.flags() > b.flags();
// // }

// int newEvaluate(Position& eB) {
//     return 0;
// }


