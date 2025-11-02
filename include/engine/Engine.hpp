
#ifndef ENGINE_H
#define ENGINE_H

#include <engine/movegen/Position.hpp>
#include <engine/search/Search.hpp>
#include <engine/search/Worker.hpp>

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq"
#define DEBUG_POSITION "rnb2b2/2p1k1p1/1p1q4/3p4/P7/8/P4PPP/RNB1KBNR w KQ - 0 9"

#define MAX_DEPTH 20
#define TT_MAX_ENTRIES 0x4000000

using namespace std::chrono_literals;

namespace Engine {
	struct UCIOptions {
		size_t hash_table_size_mb;
		uint8_t threads;
	};

	class Engine {
		private:
			bool m_Debug;
			bool m_ShouldClose;
			UCIOptions m_Options;

			Position m_Position;
			SearchWorker m_SearchWorker;

		private:
			void NewGame();
			void UCIParseCommand(std::string cmd);
			void Optimize(SearchInfo& info, int time, int inc);

		public:
			Engine(int argc, char** argv, bool debug);
			~Engine() { m_SearchWorker.Stop(); }
			
			void UCICommandLoop();
	};

} // namespace Engine

#endif // ENGINE_H
