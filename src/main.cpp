
#include "engine.hpp" 
#include "log.hpp" 

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();
	initialise_all_databases();
    zobrist::initialise_zobrist_keys();

	// Check this -> Pos: rn1q3k/ppp3pp/2n5/3B4/4P3/P3r3/1PP4P/RN2K2R w KQ - 0 16 Move: f2e3
	// Position pos;
	// pos.set("2R1k2r/7p/p4pp1/3Q4/8/4PN2/PP3PPP/6K1 b - - 0 24", pos);
	// MoveList<EVASIONS, BLACK> list(pos);
	// for (auto m : list) LOG_INFO("{} {}", list.size(), m.to_string());

	// return 0;

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCI_command_loop();

	return 0;
}
