
#include <messier/engine.hpp>
#include <messier/log.hpp>

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();

	initialise_all_databases();
    zobrist::initialise_zobrist_keys();

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCI_command_loop();

	return 0;
}
