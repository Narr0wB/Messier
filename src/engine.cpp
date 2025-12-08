
#include "engine.hpp"
#include "log.hpp"
#include "misc.hpp"

namespace Engine {
	Engine::Engine(int argc, char** argv, bool debug) : 
        m_debug(debug),  
        m_should_close(false), 
		m_options(),
        m_worker()
	{ 
		initialise_all_databases();
    	zobrist::initialise_zobrist_keys();

		// Parse command line arguments
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];

			if (arg.find("--uci=") != std::string::npos) {
				std::string raw_commands = arg.substr(arg.find("=") + 1);
				std::vector<std::string> commands = tokenize(raw_commands, ';');

				for (auto& cmd : commands) {
					UCI_parse_command(cmd);
				}
			}
		}
	};

	/*
	 * Optimize the time for the next search based on given info 
	 */
	void Engine::optimize(Search::SearchConfig& config, int time, int inc) 
	{
		if (time < 0) time = 5000;

		// Consider the overhead lost during uci communication and subtract it to the avilable time
		const int overhead = std::min(230, time / 2);
		time -= overhead;

		// If given how long should a move take then we use that for our search
		if (config.movetimeset) {
			config.search_end_time = config.search_start_time + time;
		}

		// If we are given how much time a player has left and how many more moves should the player make
		else if (config.timeset && config.movestogo != 0) {
			const float time_per_move = (int)(time / config.movestogo);

			// Never use more than 80% of the available time for a move
			const float max_time_bound = 0.8f * time;
			const float max_time = std::min(0.8f * time_per_move, max_time_bound);

			config.search_end_time = config.search_start_time + (uint64_t) max_time;
		}
		
		// If we are given only how much time is left then we define an arbitrary move time
		else if (config.timeset) {
			const float time_per_move = 0.03 * time + inc * 0.75;
			// Never use more than 80% of the available time for a move
			const float max_time_bound = 0.8f * time;
			const float max_time = std::min(time_per_move, max_time_bound);

			config.search_end_time = config.search_start_time + (uint64_t) max_time;
		}
	}

	void Engine::new_game() 
	{
		m_board.reset();
		m_worker.clear();
	}

	void Engine::UCI_command_loop() 
	{
		std::string command;
		
		while (!m_should_close) {
			if (!std::getline(std::cin, command)) {
				break;
			}

			if (!command.length()) {
				continue;
			}

			// Parse the command
			UCI_parse_command(command);
		}
	}

	void Engine::UCI_parse_command(const std::string& command) 
	{
		std::vector<std::string> tokens = tokenize(command, ' ');

		if (tokens[0] == "position") {
            m_ttable.clear();
			m_board.reset();

			if (tokens[1] == "startpos") { Position::set(START_POSITION, m_board); } 
			else if (tokens[1] == "fen") { Position::set(command.substr(command.find("fen") + 4, std::string::npos), m_board); }

			if (command.find("moves") != std::string::npos) {
                LOG_INFO("ECHOING MOVES: {}", command);
				int string_start = command.find("moves") + 6;

				if (string_start > command.length()) return;

                std::string moves_substr = command.substr(string_start, std::string::npos);
                std::vector<std::string> moves = tokenize(moves_substr, ' ');
                
                for (size_t i = 0; i < moves.size(); ++i) {
                    uint8_t promotion = 0;

                    if (moves[i].length() == 5) {
                        auto p = moves[i][4];
                        switch (p) {
                            case 'n': promotion = MoveFlags::PR_KNIGHT; break;
                            case 'b': promotion = MoveFlags::PR_BISHOP; break;
                            case 'r': promotion = MoveFlags::PR_ROOK;   break;
                            case 'q': promotion = MoveFlags::PR_QUEEN;  break;
                        }
                    }

                    if (m_board.getCurrentColor() == WHITE) {
                        MoveList<WHITE> move_list(m_board);
                        Move move = move_list.find(Move(moves[i]).to_from(), promotion);

                        if (move != NO_MOVE) {
                            m_board.play<WHITE>(move);
                        }
                    }
                    else {
                        MoveList<BLACK> move_list(m_board);
                        Move move = move_list.find(Move(moves[i]).to_from(), promotion);
                        
                        if (move != NO_MOVE) {
                            m_board.play<BLACK>(move);
                        }
                    }
                }
			}
		}

		else if (tokens[0] == "printpos") {
			std::cout << m_board << std::endl;
		}

		else if (tokens[0] == "uci") {
			std::cout << "id name " << "Messier v1.0" << std::endl;
			std::cout << "id author Ilias \"narr0w\" El Fourati" << std::endl;
			std::cout << "option name Hash type spin default 16 min 1 max 2048" << std::endl;
			std::cout << "uciok" << std::endl;
		}

		else if (tokens[0] == "debug") {
			// Toggle debug
            m_debug ^= true;
			if (m_debug) LOG_INFO("Debug enabled");
			else LOG_INFO("Debug disabled");
		}

		else if (tokens[0] == "isready") {
			std::cout << "readyok" << std::endl;
		}	

		else if (tokens[0] == "setoption") {
			if (tokens.size() < 5) {
				std::cout << "Invalid setoption format" << std::endl;
				return;
			}

			if (tokens.at(2) == "Hash") {
				size_t table_size = std::stoi(tokens.at(4));
				std::cout << "Setting hash table size to " << table_size << "MB" << std::endl;

				m_options.hash_table_size_mb = table_size;
				ttable.realloc((table_size * 1024 * 1024) / sizeof(Transposition));
			}
			
			else if (tokens.at(2) == "Threads") {
				m_options.threads = std::stoi(tokens.at(4));

				// TODO
			}
        
			else {
				std::cout << "Unknown command: " << command << std::endl;
			}
		}

		else if (tokens[0] == "ucinewgame") {
			new_game();
		}

		else if (tokens[0] == "stop") {
			m_worker.stop();
		}

		else if (tokens[0] == "quit" or tokens[0] == "exit") {
			m_worker.stop();

			m_should_close = true;
		}

		else if (tokens[0] == "go") {
			// Interrupt any previous search
			m_worker.stop();

            // Clear any search info
            m_worker.clear();

			// Check if current position is set
			if (!m_board) {
				Position::set(START_POSITION, m_board);
			}

			int depth = -1, time = 0, inc = 0;

			for (size_t i = 0; i < tokens.size(); ++i) {
				if (tokens.at(i) == "binc" and m_board.getCurrentColor() == BLACK) {
					inc = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "winc" and m_board.getCurrentColor() == WHITE) {
					inc = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "btime" and m_board.getCurrentColor() == BLACK) {
					time = std::stoi(tokens.at(i + 1));
					m_SearchContext->info.timeset = true;
				}

				else if (tokens.at(i) == "wtime" and m_board.getCurrentColor() == WHITE) {
					time = std::stoi(tokens.at(i + 1));
					m_SearchContext->info.timeset = true;
				}

				else if (tokens.at(i) == "movestogo") {
					m_SearchContext->info.movestogo = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "movetime") {
					time = std::stoi(tokens.at(i + 1));
					m_SearchContext->info.movetimeset = true;
					m_SearchContext->info.timeset     = true;
				}

				else if (tokens.at(i) == "depth") {
					depth = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "nodes") {
					m_SearchContext->info.nodeset = true;
					m_SearchContext->info.nodeslimit = std::stoi(tokens.at(i + 1));
				}
			}

			if (depth == -1) {
				depth = MAX_DEPTH;
			}

			m_SearchContext->board = m_board;
			m_SearchContext->info.search_start_time = GetTimeMS();
			m_SearchContext->info.depth = depth;
			m_SearchContext->info.quiescence_depth = 3;

			// Optimize time available for this search
			Optimize(m_SearchContext->info, time, inc);

            LOG_INFO("command: {}", command); 
            LOG_INFO("time: {}", time); 
            LOG_INFO("start: {}", m_SearchContext->info.search_start_time); 
            LOG_INFO("stop: {}", m_SearchContext->info.search_end_time); 
            LOG_INFO("movetime: {}", m_SearchContext->info.search_end_time - m_SearchContext->info.search_start_time); 
            LOG_INFO("depth: {}", m_SearchContext->info.depth); 
            LOG_INFO("timeset: {}", m_SearchContext->info.timeset); 
            LOG_INFO("nodeset: {}", m_SearchContext->info.nodeset); 

			// Clear data before starting a new search
			m_SearchContext->data = { 0 };
				
			if (m_board.getCurrentColor() == WHITE) {
				m_SearchWorker.Run<WHITE>(m_SearchContext);
			}
			else {
				m_SearchWorker.Run<BLACK>(m_SearchContext);
			}
		}
	}
} // namespace Engine
