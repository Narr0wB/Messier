
#ifndef MOVE_HPP
#define MOVE_HPP

#include <engine/movegen/Types.hpp>
#include <engine/search/Evaluate.hpp>
#include <engine/search/SearchTypes.hpp>

//A convenience class for interfacing with legal moves, rather than using the low-level
//generate_legals() function directly. It can be iterated over.
template<Color Us>
class MoveList {
	private:
		Move list[218];
		Move *last;

	public:
		explicit MoveList(Position& p) : last(p.generate_legals<Us>(list)) {}
		explicit MoveList(Position& p, Square sq) : last(p.generate_legals_for<Us>(sq, list)) {}
		explicit MoveList(Position& p, std::shared_ptr<SearchContext>& ctx, int ply, Move tt_move) : last(p.generate_legals<Us>(list)) { score_moves(ctx, ply, tt_move); }

		inline Move& operator[](size_t idx) { return list[idx]; }

		void score_moves(std::shared_ptr<SearchContext>& ctx, int ply, Move tt_move);
		Move pick(size_t idx);
		Move find(uint16_t to_from, uint8_t promotion = 0);

		Move* begin() { return list; }
		Move* end() { return last; }
		size_t size() const { return last - list; }
};

template <Color Us>
Move MoveList<Us>::find(uint16_t from_to, uint8_t promotion = 0)
{
	for (int i = 0; i < size(); ++i) {
		if (promotion) {
			if (list[i].to_from() == to_from && ((list[i].flags() & (promotion)) == promotion)) {
				return list[i];
			}
		}
		else {
			if (list[i].to_from() == to_from) {
				return list[i];
			}
		}
	}

	return Move(0);
}

template <Color Us>
void MoveList<Us>::score_moves(std::shared_ptr<SearchContext>& ctx, int ply, Move tt_move) 
{
	for (size_t i = 0; i < size(); i++) {
		list[i].set_score(score_move(list[i], ctx, ply, tt_move));
	}
}

template <Color Us>
Move MoveList<Us>::pick(size_t idx) 
{
    for (size_t i = idx + 1; i < size(); i++) {
        if (list[i].get_score() > list[idx].get_score()) {
            Move tmp = list[idx];
            list[idx] = list[i];
            list[i] = tmp;
        }
    }

	return list[idx];
}

#endif // MOVE_HPP