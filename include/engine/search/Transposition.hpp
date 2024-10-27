
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <cstring>

#include <engine/movegen/Types.hpp>

#define FLAG_EMPTY 0
#define FLAG_EXACT 1
#define FLAG_ALPHA 2
#define FLAG_BETA 3

#define NO_SCORE 0 

typedef uint8_t TranspositionFlags;

struct Transposition {
    Transposition(TranspositionFlags f, uint64_t h, int8_t d, int sc, Move b) : 
    flags(f), hash(h), depth(d), score(sc), best(b) {};
    
    Transposition() {};

    TranspositionFlags flags;
    uint64_t hash;
    int8_t depth;
    int score;
    Move best;
};

#define NO_HASH_ENTRY Transposition { FLAG_EMPTY, 0, 0, NO_SCORE, NO_MOVE }

class TranspositionTable {
    private:
        Transposition* m_DataArray;

        // Maximum number of transpositions that can be stored in the table
        uint32_t m_Capacity;

        // Number of transpositions currently stored in the table
        size_t m_Size;

    public:
        // Transposition table hits
        int hits;

    public:
        TranspositionTable(uint32_t capacity) : m_Capacity(capacity), m_Size(0), hits(0) {
            m_DataArray = new Transposition[capacity];
            std::memset(m_DataArray, 0, sizeof(Transposition) * m_Capacity);
        }

        TranspositionTable() {};

        // Move constructor
        TranspositionTable(TranspositionTable&& tt) : 
        m_DataArray(tt.m_DataArray),
        m_Size(tt.m_Size),
        hits(tt.hits) {}

        // Move assignment operator
        inline TranspositionTable& operator=(TranspositionTable&& tt) {
            m_DataArray = (tt.m_DataArray);
            m_Size = (tt.m_Size);
            hits = (tt.hits);

            return *this;
        }

        ~TranspositionTable() {
            delete m_DataArray;
        }

        inline uint32_t size() { return m_Size; };
        void realloc(size_t numberOfTranspositions);
        void clear();

        void push_position(Transposition t);
        Transposition probe_hash(uint64_t hash, int alpha, int beta, int depth);
};

#endif
