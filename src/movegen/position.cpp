#include "movegen/position.hpp" 
#include "movegen/tables.hpp" 
#include <sstream>

//Zobrist keys for each piece and each square
//Used to incrementally update the hash key of a position
uint64_t zobrist::zobrist_table[NPIECES][NSQUARES];
uint64_t zobrist::side_to_move[NCOLORS];

//Initializes the zobrist table with random 64-bit numbers
void zobrist::initialise_zobrist_keys() {
	PRNG rng(70026072);
	for (int i = 0; i < NPIECES; i++)
		for (int j = 0; j < NSQUARES; j++)
			zobrist::zobrist_table[i][j] = rng.rand<uint64_t>();
		
	zobrist::side_to_move[0] = 0;
	zobrist::side_to_move[1] = rng.rand<uint64_t>();
}

//Pretty-prints the position (including FEN and hash key)
std::ostream& operator<< (std::ostream& os, const Position& p) {
	const char* s = "   +---+---+---+---+---+---+---+---+\n";
	const char* t = "     A   B   C   D   E   F   G   H\n";
	os << t;
	for (int i = 56; i >= 0; i -= 8) {
		os << s << " " << i / 8 + 1 << " ";
		for (int j = 0; j < 8; j++)
			os << "| " << PIECE_STR[p.board[i + j]] << " ";
		os << "| " << i / 8 + 1 << "\n";
	}
	os << s;
	os << t << "\n";

	os << "FEN: " << p.fen() << "\n";
	os << "Hash: 0x" << std::hex << p.hash << std::dec << "\n";

	return os;
}

//Returns the FEN (Forsyth-Edwards Notation) representation of the position
std::string Position::fen() const {
	std::ostringstream fen;
	int empty;

	for (int i = 56; i >= 0; i -= 8) {
		empty = 0;
		for (int j = 0; j < 8; j++) {
			Piece p = board[i + j];
			if (p == NO_PIECE) empty++;
			else {
				fen << (empty == 0 ? "" : std::to_string(empty))
					<< PIECE_STR[p];
				empty = 0;
			}
		}

		if (empty != 0) fen << empty;
		if (i > 0) fen << '/';
	}

	fen << (side_to_play == WHITE ? " w " : " b ")
		<< (history[game_ply].entry & WHITE_OO_MASK ? "" : "K")
		<< (history[game_ply].entry & WHITE_OOO_MASK ? "" : "Q")
		<< (history[game_ply].entry & BLACK_OO_MASK ? "" : "k")
		<< (history[game_ply].entry & BLACK_OOO_MASK ? "" : "q")
		<< (history[game_ply].entry & ALL_CASTLING_MASK ? "" : "")
		<< " " << (history[game_ply].epsq == NO_SQUARE ? "-" : SQSTR[history[game_ply].epsq]);

	return fen.str();
}

//Updates a position according to an FEN string
void Position::set(const std::string& fen, Position& p) {
	int square = a8;
	for (char ch : fen.substr(0, fen.find(' '))) {
		if (isdigit(ch))
			square += (ch - '0') * EAST;
		else if (ch == '/')
			square += 2 * SOUTH;
		else
			p.put_piece(Piece(PIECE_STR.find(ch)), Square(square++));
	}

	std::istringstream ss(fen.substr(fen.find(' ')));
	unsigned char token;

	ss >> token;
	p.side_to_play = token == 'w' ? WHITE : BLACK;
	p.hash ^= zobrist::side_to_move[p.side_to_play];

	p.history[p.game_ply].entry = ALL_CASTLING_MASK;
	while (ss >> token && !isspace(token)) {
		switch (token) {
		case 'K':
			p.history[p.game_ply].entry &= ~WHITE_OO_MASK;
			break;
		case 'Q':
			p.history[p.game_ply].entry &= ~WHITE_OOO_MASK;
			break;
		case 'k':
			p.history[p.game_ply].entry &= ~BLACK_OO_MASK;
			break;
		case 'q':
			p.history[p.game_ply].entry &= ~BLACK_OOO_MASK;
			break;
		}
	}
}
	

//Moves a piece to a (possibly empty) square on the board and updates the hash
void Position::move_piece(Square from, Square to) {
	hash ^= zobrist::zobrist_table[board[from]][from] ^ zobrist::zobrist_table[board[from]][to]
		^ zobrist::zobrist_table[board[to]][to];
	Bitboard mask = SQUARE_BB[from] | SQUARE_BB[to];
	piece_bb[board[from]] ^= mask;
	piece_bb[board[to]] &= ~mask;
	board[to] = board[from];
	board[from] = NO_PIECE;
}

//Moves a piece to an empty square. Note that it is an error if the <to> square contains a piece
void Position::move_piece_quiet(Square from, Square to) {
	hash ^= zobrist::zobrist_table[board[from]][from] ^ zobrist::zobrist_table[board[from]][to];
	piece_bb[board[from]] ^= (SQUARE_BB[from] | SQUARE_BB[to]);
	board[to] = board[from];
	board[from] = NO_PIECE;
}

void Position::reset()
{
	game_ply = 0;
	hash = 0;
	side_to_play = WHITE;

	for (int i = 0; i < NPIECES; ++i) {
		piece_bb[i] = 0;
	}

	for (int i = 0; i < NSQUARES; ++i) {
		board[i] = NO_PIECE;
	}

	for (int i = 0; i < 256; ++i) {
		history[i] = UndoInfo();
	}
}

bool Position::is_pseudo_legal(Move m) 
{
	Color to_play = side_to_play;

	Bitboard us = to_play == WHITE ? all_pieces<WHITE>() : all_pieces<BLACK>();
	Square from = m.from();
	Square to   = m.to();
	MoveFlags f = m.flags();
	Piece  p1   = at(from);
	Piece  p2   = at(to);

	if (p1 == NO_PIECE || color_of(p1) != to_play) return false;
	if (m.is_capture() && f != MoveFlags::EN_PASSANT && (p2 == NO_PIECE || bitboard_at(to) & us)) return false;
	if (m.is_quiet()   && (p2 != NO_PIECE)) return false;

	Bitboard occ = all_pieces<WHITE>() | all_pieces<BLACK>();

	switch (type_of(p1)) {
		case PAWN: {
			Direction up = Direction(to_play == WHITE ? NORTH : -NORTH);

			if (f == MoveFlags::QUIET && to != from + up) return false;
			if (f == MoveFlags::DOUBLE_PUSH && 
				(to != from + up + up || rank_of(from) != (to_play == WHITE ? Rank::RANK2 : Rank::RANK7) || at(from + up) != NO_PIECE)) return false;
			if (f == MoveFlags::EN_PASSANT && to != history[game_ply].epsq) return false;
			if (m.is_capture() && 
				!((to_play == WHITE ? pawn_attacks<WHITE>(from) : pawn_attacks<BLACK>(from)) & bitboard_at(to))) return false;

			return true;
		}
		case KNIGHT: {
			return attacks<KNIGHT>(from, occ) & bitboard_at(to);
		}
		case BISHOP:  {
			return attacks<BISHOP>(from, occ) & bitboard_at(to);
		}
		case ROOK: {
			return attacks<ROOK>(from, occ) & bitboard_at(to);
		}
		case QUEEN: {
			return attacks<QUEEN>(from, occ) & bitboard_at(to);
		}
		case KING: {
			if (f == MoveFlags::OO) {
				return (history[game_ply].entry & (to_play == WHITE ? WHITE_OO_MASK : BLACK_OO_MASK)) && 
					   !(to_play == WHITE ? in_check<WHITE>() : in_check<BLACK>()) && 
					   !(occ & (to_play == WHITE ? oo_blockers_mask<WHITE>() : oo_blockers_mask<BLACK>()));
			}

			if (f == MoveFlags::OOO) {
				return (history[game_ply].entry & (to_play == WHITE ? WHITE_OOO_MASK : BLACK_OOO_MASK)) && 
					   !(to_play == WHITE ? in_check<WHITE>() : in_check<BLACK>()) &&
					   !(occ & (to_play == WHITE ? ooo_blockers_mask<WHITE>() : ooo_blockers_mask<BLACK>()));
			}

			return attacks<KING>(from, occ) & bitboard_at(to);
		}
	}

	return true;
}


