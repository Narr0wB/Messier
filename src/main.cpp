
#include "engine.hpp" 
#include "log.hpp" 

#include <memory>

int main(int argc, char** argv) 
{
    Log::Init();

	std::unique_ptr<Engine::Engine> app = std::make_unique<Engine::Engine>(argc, argv, true);
	app->UCICommandLoop();

	return 0;
}
