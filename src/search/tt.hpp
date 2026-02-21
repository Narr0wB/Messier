
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <tuple>

#include "../movegen/move.hpp"
#include "log.hpp"

#define FLAG_EMPTY 0
#define FLAG_EXACT 1
#define FLAG_ALPHA 2
#define FLAG_BETA 3

#define NO_SCORE 0 
#define NO_EVAL 0 

struct Transposition {
    uint64_t hash;
    int32_t score;
    int32_t eval;
    Move move;
    uint8_t flags;
    uint8_t depth;

    Transposition() = default;

    Transposition(uint8_t f, uint64_t h, int8_t d, int sc, int e, Move m) : 
    flags(f), hash(h), depth(d), score(sc), move(m), eval(e) {};
};

#define NO_HASH_ENTRY Transposition { FLAG_EMPTY, 0, 0, NO_EVAL, NO_SCORE, NO_MOVE }
#define DEFAULT_CAPACITY (1 << 20)
#define MAX_CAPACITY (1 << 22)

class TTable {
    private:
        std::vector<Transposition> m_map;
        size_t m_capacity;
        size_t m_stored;
        uint64_t m_hits;
    
    public:
        TTable(size_t capacity) :
            m_capacity(capacity),
            m_stored(0),
            m_hits(0)
        {
            if (capacity < MAX_CAPACITY)
                m_map.resize(capacity);
            else
                m_map.resize(MAX_CAPACITY);
        }

        TTable() :
            m_capacity(DEFAULT_CAPACITY),
            m_stored(0),
            m_hits(0)
        {}

        inline size_t stored() { return m_stored; }
        inline size_t capacity() { return m_capacity; }
        inline uint64_t hits() { return m_hits; }

        void resize(size_t new_capacity);
        void clear();

        void push(uint64_t hash, const Transposition& t);
        std::tuple<bool, Transposition> probe(uint64_t hash);
};

// class TranspositionTable {
//     private:
//         Transposition* m_DataArray;

//         // Maximum number of transpositions that can be stored in the table
//         uint32_t m_Capacity;

//         // Number of transpositions currently stored in the table
//         size_t m_Size;

//     public:
//         // Transposition table hits
//         int hits;

//     public:
//         TranspositionTable(uint32_t capacity) : m_Capacity(capacity), m_Size(0), hits(0) {
//             m_DataArray = new Transposition[capacity];
//             std::memset(m_DataArray, 0, sizeof(Transposition) * m_Capacity);
//         }

//         TranspositionTable() {};

//         // Move constructor
//         TranspositionTable(TranspositionTable&& tt) : 
//         m_DataArray(tt.m_DataArray),
//         m_Size(tt.m_Size),
//         hits(tt.hits) {}

//         // Move assignment operator
//         inline TranspositionTable& operator=(TranspositionTable&& tt) 
//         {
//             m_DataArray = (tt.m_DataArray);
//             m_Size = (tt.m_Size);
//             hits = (tt.hits);

//             return *this;
//         }

//         ~TranspositionTable() {
//             delete m_DataArray;
//         }

//         inline uint32_t size() { return m_Size; };
//         void realloc(size_t numberOfTranspositions);
//         void clear();

//         void push_position(Transposition t);
//         Transposition probe_hash(uint64_t hash);
// };

#endif
