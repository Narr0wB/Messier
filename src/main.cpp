
#include "engine.hpp" 
#include "log.hpp" 

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();

	// Position pos;
	// pos.set("rnbqkbnr/pppppppp/8/8/8/N7/PPPPPPPP/R1BQKBNR b KQkq-", pos);
	// pos.play<BLACK>(Move::from_string("d7d5"));
	// LOG_INFO("{}", pos.fen());

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCI_command_loop();

	return 0;
}
