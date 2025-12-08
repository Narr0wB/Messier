
#ifndef ENGINE_H
#define ENGINE_H

#include "./movegen/position.hpp"
#include "./search/search.hpp"

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq"
#define DEBUG_POSITION "rnb2b2/2p1k1p1/1p1q4/3p4/P7/8/P4PPP/RNB1KBNR w KQ - 0 9"

namespace Engine {
	struct UCIOptions {
		size_t hash_table_size_mb;
		uint8_t threads;
	};

	class Engine {
		private:
			bool m_debug;
			bool m_should_close;
			UCIOptions m_options;

			Position m_board;
			Search::TranspositionTable m_ttable;
			Search::Worker m_worker;

		private:
			void new_game();
			void UCI_parse_command(const std::string& cmd);
			void optimize(Search::SearchConfig& config, int time, int inc);

		public:
			Engine(int argc, char** argv, bool debug);
			
			void UCI_command_loop();
	};
} // namespace Engine

#endif // ENGINE_H