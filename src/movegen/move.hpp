
#ifndef MOVE_HPP
#define MOVE_HPP

#include "movegen/types.hpp" 
#include "movegen/position.hpp" 

#define MAX_MOVES 218

template <GenType type, Color Us>
class MoveList {
	private:
		Move list[MAX_MOVES];
		Move *last;

	public:
		explicit MoveList(const Position& p) : last(p.generate<type, Us>(list)) {}

		inline Move& operator[](size_t idx) { return list[idx]; }

		Move* begin() { return list; }
		Move* end() { return last; }
		size_t size() const { return last - list; }
};

// template <GenType type, Color Us>
// Move MoveList<type, Us>::find(uint16_t to_from, uint8_t promotion)
// {
// 	for (int i = 0; i < size(); ++i) {
// 		if (promotion) {
// 			if (list[i].to_from() == to_from && ((list[i].flags() & (promotion)) == promotion)) {
// 				return list[i];
// 			}
// 		}
// 		else {
// 			if (list[i].to_from() == to_from) {
// 				return list[i];
// 			}
// 		}
// 	}

// 	return Move(0);
// }

#endif // MOVE_HPP