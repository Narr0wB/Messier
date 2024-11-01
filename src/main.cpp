#include <engine/Engine.hpp>

/*
	Table of commands:

	SET <fen_string>							---> Set the board using a FEN string
	MOVE <color> <unint16_t move>				---> Play move <move> as <color>
	UNDO										---> Undo the last move
	MOVEREQ <AI_level> <AI_color>				---> Request a move from the AI at level <AI_level> as <AI_color>
	GENMOVES <piece_square>					    ---> Generate all legal moves for given piece at <piece_square>, if <piece_square> is equal to 100 + <color> then generate all legal moves for <color>
	CHECK <color>								---> Check if side <color> is in check
	CHECKMATE <color>							---> Check if side <color> is in checkmate
	COLOR <color>								---> Return the current side to play
	EXIT										---> Exit the program

*/

int main(int argc, char** argv) {
    Log::Init();

	Engine::Engine app(argc, argv, true);
	app.UCICommandLoop();

    // info depth 7 score cp -65 nodes 249714 tthits 64170 pv b1c3 c7c6 g1f3 f7f6 d2d4 b7b6 e2e4
    // info depth 7 score cp -65 nodes 180261 tthits 1878 pvlen 7 pv b1c3 c7c6 g1f3 f7f6 d2d4 g7g6 e2e4
    //
    // position startpos
    // info depth 10 score cp -70 nodes 8398043 tthits 0 pv d2d4 b7b6 b1c3 f7f6 g1f3 c7c5 d4d5 g7g6 e2e4 a7a5
    // info depth 10 score cp -60 nodes 4582238 tthits 115152 pv b1c3 c7c5 g1f3 b7b6
    // info depth 10 score cp -65 nodes 3931283 tthits 56044 pvlen 10 pv g1f3 f7f5 b1c3 c7c5 d2d4 c5d4 (capture) f3d4 (capture) g7g6 c1f4 b8a6
    // info depth 10 score cp -70 nodes 2954713 nps 1529364 tthits 55452 pvlen 10 pv b1c3 b7b6 g1f3 f7f6 d2d4 c7c5 d4d5 g7g6 e2e4 a7a5
    //
    // position startpos moves d2d4 c7c6 e2e4 d8a5 b1d2 g7g6 g1f3 f7f6 f1d3 b7b6 e1g1
    // info depth 10 score cp -45 nodes 42182693 tthits 0 pvlen 10 pv a5h5 e4e5 g6g5 e5f6 (capture) e7f6 (capture) d1e2 e8d8 d2e4 g5g4 f3d2
    // info depth 10 score cp -35 nodes 12371631 tthits 191554 pvlen 10 pv a5h5 d2b3 f8h6 d3c4 h6c1 (capture) a1c1 (capture) a7a5 c4g8 (capture) h8g8 (capture) d1d3
    // info depth 10 score cp -40 nodes 11973931 tthits 81955 pvlen 10 pv a5h5 d2b3 f8h6 c1h6 (capture) h5h6 (capture) d3c4 a7a5 c4g8 (capture) h8g8 (capture) d1d3 
    // info depth 10 score cp -40 nodes 11585913 tthits 80611 pvlen 10 pv a5h5 d2b3 f8h6 c1h6 (capture) h5h6 (capture) d3c4 a7a5 c4g8 (capture) h8g8 (capture) d1d3
   
	return 0;
}
