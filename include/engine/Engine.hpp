
#ifndef ENGINE_H
#define ENGINE_H

#include <engine/movegen/Tables.hpp>
#include <engine/movegen/Types.hpp>
#include <engine/movegen/Position.hpp>
#include <engine/search/Search.hpp>
#include <engine/search/Thread.hpp>
#include <engine/movegen/Move.hpp>
#include <engine/Log.hpp>
#include <engine/Misc.hpp>

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq"
#define DEBUG_POSITION "rnb2b2/2p1k1p1/1p1q4/3p4/P7/8/P4PPP/RNB1KBNR w KQ - 0 9"

#define MAX_DEPTH 20

using namespace std::chrono_literals;

namespace Engine {
	// static const char* COMMAND_STR_B[] = {"NONE", "SET", "MOVE", "UNDO", "MOVEREQ", "GENMOVES", "CHECK", "CHECKMATE", "FEN", "COLOR"};

	// enum CommandType : uint16_t {
	// 	NONE = 0, SET, MOVE, UNDO, MOVEREQ, GENMOVES, CHECK, CHECKMATE, FEN, COLOR
	// };

	// Structure of the command

	//       HEADER            ------- Payload ([PAYLEN] BYTES)
	//  ----------------      |
	// |                |     v
	// 0x00 | 0x00 0x00 | 0x00 ...
	//   ^       ^
	//   |       | 
	//   |        --------------- Payload length (2 BYTES)
	//    -------- Command identifier (1 BYTE)

	// struct CommandHeader {
	// 	CommandType type;
	// 	u16 payload_size;
	// };

	// struct Command {
	// 	CommandHeader header;
	// 	u8 payload[256];
	// };

	// struct Response {
	// 	u16 payload_size;
	// 	u8 payload[512];
	// };

	// #define OK_RESPONSE 	Engine::Response{2, {0xFF, 0xFF}}
	// #define BAD_RESPONSE 	Engine::Response{2, {0xDE, 0xAD}}
	// #define WRITE_MOVELIST(move_list, resp) \
	// {\
	// 	resp.payload_size =  move_list.size() * sizeof(Move);\
	// 	std::memcpy(resp.payload, (void*)move_list.begin(), resp.payload_size);\
	// };

	struct UCIOptions {
		size_t hash_table_size_mb;
		uint8_t threads;
	};

	void Optimize(SearchInfo& info, int time, int inc);

	class Engine {
		private:
			Position m_Position;
			std::vector<Move> m_MoveList;
			
			bool m_Debug;
			bool m_ShouldClose;
			SearchWorker m_SearchWorker;
			std::shared_ptr<SearchContext> m_SearchContext;
			UCIOptions m_Options;

			#define TT_MAX_ENTRIES 0x4000000

			std::shared_ptr<TranspositionTable> m_Table;

		private:
			void NewGame();
			void UCIParseCommand(std::string cmd);

		public:
			Engine(int argc, char** argv, bool debug) : 
                m_Debug(debug),  
                m_ShouldClose(false), 
                m_SearchWorker(),
                m_Table(new TranspositionTable(0x1000000)),
                m_SearchContext(new SearchContext())
			{ 
				initialise_all_databases();
    			zobrist::initialise_zobrist_keys();

				// Initialize search context
				m_SearchContext->table = m_Table;

				// Parse command line arguments
				for (int i = 1; i < argc; ++i) {
					std::string arg = argv[i];

					if (arg.find("--uci=") != std::string::npos) {
						std::string raw_commands = arg.substr(arg.find("=") + 1);
						std::vector<std::string> commands = tokenize(raw_commands, ';');

						for (auto& cmd : commands) {
							UCIParseCommand(cmd);
						}
					}
				}
			};
			~Engine() { m_SearchWorker.Stop(); }
			

			void UCICommandLoop();
	};

} // namespace Engine

#endif // ENGINE_H
