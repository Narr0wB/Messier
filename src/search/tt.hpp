
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <tuple>

#include "movegen/move.hpp"
#include "log.hpp"

#define FLAG_EMPTY 0
#define FLAG_EXACT 1
#define FLAG_ALPHA 2
#define FLAG_BETA 3

#define MATE_SCORE (INT16_MAX / 2)
#define INFTY (MATE_SCORE + 10) 
#define NO_SCORE (INFTY + 1)
#define NO_EVAL 0 

struct Transposition {
    uint64_t hash;
    int16_t score;
    int16_t eval;
    Move move;
    int8_t depth;
    uint8_t flags : 2;
    uint8_t generation : 6;

    Transposition() = default;

    Transposition(uint8_t f, uint64_t h, int8_t d, int sc, int e, Move m, uint8_t gen) : 
    flags(f), hash(h), depth(d), score(sc), move(m), eval(e), generation(gen) {};
};

#define NO_HASH_ENTRY { FLAG_EMPTY, 0, 0, NO_SCORE, NO_EVAL, Move::none(), 0 }
#define DEFAULT_CAPACITY (1 << 25)
#define MAX_CAPACITY (1 << 27)

class TTable {
    private:
        std::vector<Transposition> m_map;
        size_t m_capacity;
        size_t m_stored;
    
    public:
        TTable(size_t capacity) :
            m_capacity(capacity),
            m_stored(0)
        {
            if (capacity < MAX_CAPACITY) {
                m_map.resize(capacity);
            }
            else {
                m_map.resize(MAX_CAPACITY);
                m_capacity = MAX_CAPACITY;
            }
        }

        TTable() : m_capacity(DEFAULT_CAPACITY), m_stored(0) { m_map.resize(m_capacity); };

        inline size_t stored() { return m_stored; }
        inline size_t capacity() { return m_capacity; }

        void resize(size_t new_capacity);
        void clear();

        void push(uint64_t hash, const Transposition& t);
        std::tuple<bool, Transposition> probe(uint64_t hash);
};

#endif
