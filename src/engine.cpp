
#include "engine.hpp"
#include "log.hpp"
#include "misc.hpp"

namespace Engine {
	Engine::Engine(int argc, char** argv, bool debug) : 
        m_debug(debug),  
        m_should_close(false), 
		m_options(),
		m_table(DEFAULT_CAPACITY),
        m_worker(m_table)
	{ 
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
		m_table.clear();
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
			m_board.reset();

			if (tokens[1] == "startpos") { Position::set(START_POSITION, m_board); } 
			else if (tokens[1] == "fen") { Position::set(command.substr(command.find("fen") + 4, std::string::npos), m_board); }

			if (command.find("moves") != std::string::npos) {
                LOG_INFO("ECHOING MOVES: {}", command);
				int string_start = command.find("moves") + 6;

				if (string_start > command.length()) return;

                std::string moves_substr = command.substr(string_start, std::string::npos);
                std::vector<std::string> moves = tokenize(moves_substr, ' ');

				for (const std::string& move : moves) {
					Move m = Move::from_string(move);

					auto match_lambda = [&m](Move _m) {
						if (m.is_promotion()) {
							// If the move is a promotion, Move::from_string() only loaded the promotion type (since captures are inferred by the current position).
							// Then, we just look for the move with the same (to, from) and promotion type
							return (_m.to_from() == m.to_from()) && ((_m.flags() & ~MoveFlags::CAPTURE) == m.flags());
						}

						return (_m.to_from() == m.to_from());
					};

					if (m_board.turn() == Color::WHITE) {
						MoveList<LEGAL, WHITE> move_list(m_board);
						Move *match = std::find_if(move_list.begin(), move_list.end(), match_lambda);
						if (match != move_list.end()) m_board.play<WHITE>(*match);
					}

					if (m_board.turn() == Color::BLACK) {
						MoveList<LEGAL, BLACK> move_list(m_board);
						Move *match = std::find_if(move_list.begin(), move_list.end(), match_lambda);
						if (match != move_list.end()) m_board.play<BLACK>(*match);
					}
				}
                
                // for (size_t i = 0; i < moves.size(); ++i) {
                //     uint8_t promotion = 0;

                //     if (moves[i].length() == 5) {
                //         auto p = moves[i][4];
                //         switch (p) {
                //             case 'n': promotion = MoveFlags::PR_KNIGHT; break;
                //             case 'b': promotion = MoveFlags::PR_BISHOP; break;
                //             case 'r': promotion = MoveFlags::PR_ROOK;   break;
                //             case 'q': promotion = MoveFlags::PR_QUEEN;  break;
                //         }
                //     }

                //     if (m_board.turn() == WHITE) {
                //         MoveList<WHITE> move_list(m_board);
                //         Move move = move_list.find(Move(moves[i]).to_from(), promotion);

                //         if (move != NO_MOVE) {
                //             m_board.play<WHITE>(move);
                //         }
                //     }
                //     else {
                //         MoveList<BLACK> move_list(m_board);
                //         Move move = move_list.find(Move(moves[i]).to_from(), promotion);
                        
                //         if (move != NO_MOVE) {
                //             m_board.play<BLACK>(move);
                //         }
                //     }
                // }
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
				m_table.clear();
				m_table.resize((table_size * 1024 * 1024) / sizeof(Transposition));
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

			// Check if current position is set
			if (!m_board) {
				Position::set(START_POSITION, m_board);
			}

			int depth = -1, time = 0, inc = 0;
			Search::SearchConfig cfg = {0};

			for (size_t i = 0; i < tokens.size(); ++i) {
				if (tokens.at(i) == "binc" and m_board.turn() == BLACK) {
					inc = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "winc" and m_board.turn() == WHITE) {
					inc = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "btime" and m_board.turn() == BLACK) {
					time = std::stoi(tokens.at(i + 1));
					cfg.timeset = true;
				}

				else if (tokens.at(i) == "wtime" and m_board.turn() == WHITE) {
					time = std::stoi(tokens.at(i + 1));
					cfg.timeset = true;
				}

				else if (tokens.at(i) == "movestogo") {
					cfg.movestogo = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "movetime") {
					time = std::stoi(tokens.at(i + 1));
					cfg.movetimeset = true;
					cfg.timeset     = true;
				}

				else if (tokens.at(i) == "depth") {
					depth = std::stoi(tokens.at(i + 1));
				}

				else if (tokens.at(i) == "nodes") {
					cfg.nodeset = true;
					cfg.nodeslimit = std::stoi(tokens.at(i + 1));
				}
			}

			if (depth == -1) {
				depth = MAX_DEPTH;
			}

			cfg.search_start_time = time_ms();
			cfg.max_depth = depth;
			cfg.quiescence_depth = 3;

			// Optimize time available for this search
			optimize(cfg, time, inc);

            LOG_INFO("command: {}", command); 
            LOG_INFO("time: {}", time); 
            LOG_INFO("start: {}", cfg.search_start_time); 
            LOG_INFO("stop: {}", cfg.search_end_time); 
            LOG_INFO("movetime: {}", cfg.search_end_time - cfg.search_start_time); 
            LOG_INFO("depth: {}", cfg.max_depth); 
            LOG_INFO("timeset: {}", cfg.timeset); 
            LOG_INFO("nodeset: {}", cfg.nodeset); 

			m_worker.run(m_board, cfg);
		}
	}
} // namespace Engine
