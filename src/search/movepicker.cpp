
#include "search/movepicker.hpp"

template <Color Us>
int SEE(Position pos, Square to) 
{
    int value = 0;
    Bitboard occupied = pos.all_pieces<WHITE>() | pos.all_pieces<BLACK>();

    int pt;
    Square from = NO_SQUARE;
    Bitboard attackers;
    for (pt = PAWN; pt < KING; pt++) {
        if ( (attackers = pos.attacker<Us>((PieceType)pt, to, occupied)) ) {
            from = pop_lsb(&attackers);
            break;
        }
    }

    if (from != NO_SQUARE) {
        int captured = pos.at(to) == NO_PIECE ? 6 : type_of(pos.at(to));
        Move see_move(from, to, captured == 6 ? MoveFlags::QUIET : MoveFlags::CAPTURE);

        pos.play<Us>(see_move);

        value = piece_value[captured] - SEE<~Us>(pos, to);    

        pos.undo<Us>(see_move);
    }
        
    return value;
}

/* Standard search constructor */
MovePicker::MovePicker(const Position& pos, const Search::SearchContext& ctx, int ply, int depth, Move tt_move) :
    m_pos(pos),
    m_ctx(ctx),
    m_ply(ply),
    m_depth(depth),
    m_ttmove(tt_move)
{
    if (pos.checkers)
        m_stage = Stage::EVASION_TT;
    else
        m_stage = (depth > 0) ? Stage::MAIN_TT : Stage::QUIESCENCE_TT;
}

// MovePicker::MovePicker(const Position& pos, const Search::SearchContext& ctx, int ply, Move tt_move) :
//     m_pos(pos),
//     m_ctx(ctx),
//     m_ply(ply),
//     m_ttmove(tt_move)
// {

// }

template int SEE<WHITE>(Position pos, Square to);
template int SEE<BLACK>(Position pos, Square to);