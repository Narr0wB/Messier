
#include "engine.hpp" 
#include "log.hpp" 

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();
	initialise_all_databases();
    zobrist::initialise_zobrist_keys();

	// Position pos;
	// pos.set(START_POSITION, pos);
	// pos.play<WHITE>(Move::from_string("h2h4"));
	// pos.play<BLACK>(Move::from_string("b8a6"));
	// pos.play<WHITE>(Move::from_string("g2g4"));
	// pos.play<BLACK>(Move::from_string("a6b4"));
	// pos.play<WHITE>(Move::from_string("f2f4"));

	// LOG_INFO("POS {}", pos.fen());

	// return 0;

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCI_command_loop();

	return 0;
}
