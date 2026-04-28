
#include "evaluate.hpp"

using namespace std::chrono_literals;

int mg_value[6] = { 82, 337, 365, 477, 1025,  0};
int eg_value[6] = { 94, 281, 297, 512,  936,  0};

int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

int* mg_tables[NPIECE_TYPES] = {
    mg_pawn_table,
    mg_knight_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
    mg_king_table
};

int* eg_tables[NPIECE_TYPES] = {
    eg_pawn_table,
    eg_knight_table,
    eg_bishop_table,
    eg_rook_table,
    eg_queen_table,
    eg_king_table
};

int evaluate(const Position& position) 
{
    // int score = position.material(WHITE) - position.material(BLACK);
    int mg_score = 0;
    int eg_score = 0;

    for (PieceType p = PAWN; p <= KING; ++p) {
        Bitboard white_piece_bb = position.bitboard_of(make_piece(WHITE, p));
        Bitboard black_piece_bb = position.bitboard_of(make_piece(BLACK, p));

        while (white_piece_bb) {   
            Square piece_sq = pop_lsb(&white_piece_bb);

            mg_score += mg_value[p];
            eg_score += eg_value[p];

            mg_score += mg_tables[p][piece_sq ^ 56];
            eg_score += eg_tables[p][piece_sq ^ 56];
        }

        while (black_piece_bb) {
            Square piece_sq = pop_lsb(&black_piece_bb);

            mg_score -= mg_value[p];
            eg_score -= eg_value[p];

            mg_score -= mg_tables[p][piece_sq];
            eg_score -= eg_tables[p][piece_sq];
        }
    }

    int phase = position.npm();
    if (phase > 24) phase = 24;
    return (phase * mg_score + (24 - phase) * eg_score) / 24;
}

// int score_move(const Move& m_, const Search::SearchContext& ctx, const Position& pos, int ply, Move ttmove) 
// {
//     // Score the move from the previous iterative search pv higher 
//     if (m_ == ctx.pv_table[0][ply]) {
//         return MAX_MOVE_SCORE;
//     }

//     else if (m_ == ttmove) {
//         return MAX_MOVE_SCORE - 1000; 
//     }

//     else if (m_.flags() == MoveFlags::CAPTURE) {
//         return mvv_lva(m_, pos) + 1000;
//     }

//     else if (m_ == ctx.killer_moves[ply][0]) {
//         return 900;
//     }

//     else if (m_ == ctx.killer_moves[ply][1]) {
//         return 800;
//     }

//     return ctx.history_moves[m_.from()][m_.to()];
// }

// static const int mvv_lva_lookup[NPIECE_TYPES][NPIECE_TYPES] = {
//     /*           PAWN  KNIGHT BISHOP ROOK QUEEN KING */
//     /* PAWN */   {105, 205,   225,   235, 805,  905},
//     /* KNIGHT */ {104, 204,   224,   234, 804,  904},
//     /* BISHOP */ {103, 203,   223,   233, 803,  903},
//     /* ROOK */   {102, 202,   222,   232, 802,  902},
//     /* QUEEN  */ {101, 201,   221,   231, 801,  901},
//     /* KING  */  {100, 200,   220,   230, 800,  900},
// };

// int mvv_lva(const Move &m_, const Position &p_) 
// {
//     if (m_.flags() != MoveFlags::CAPTURE) {
//         return 0;
//     }

//     PieceType attacker = type_of(p_.at(m_.from()));
//     PieceType victim = type_of(p_.at(m_.to()));

//     return mvv_lva_lookup[attacker][victim];
// }

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


