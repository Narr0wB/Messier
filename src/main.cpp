
#include "engine.hpp" 
#include "log.hpp" 

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();

	initialise_all_databases();
    zobrist::initialise_zobrist_keys();

	// Check this -> Pos: rn1q3k/ppp3pp/2n5/3B4/4P3/P3r3/1PP4P/RN2K2R w KQ - 0 16 Move: f2e3
	// New problematic position: r2q1r1k/p1p3pp/3bQ3/8/8/2P1P3/P1P2PPP/R1B1K2R w KQ - 1 17, illegal move: d5e6

	// 0x4416848e53be08e9
	// 0x4416848e53be08e9

	// Position pos;
	// Position pos2;
	// Position::set("r1bqkbnr/ppppp1pp/n7/1N3p2/8/7N/PPPPPPPP/R1BQKB1R b KQkq -", pos);
	// Position::set("r1bqkbnr/ppppp1pp/n7/1N3p2/8/7N/PPPPPPPP/R1BQKB1R w KQkq -", pos2);
	// LOG_INFO("h1 {}", pos.get_hash());
	// LOG_INFO("h2 {}", pos2.get_hash());
	// LOG_INFO("cmp {}", pos2.get_hash() == pos.get_hash());
	// return 0;

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCI_command_loop();

	return 0;
}
