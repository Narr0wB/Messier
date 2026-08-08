
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <tuple>

#include <messier/movegen/move.hpp>
#include <messier/log.hpp>
#include <messier/misc.hpp>

#define FLAG_EMPTY 0
#define FLAG_EXACT 1
#define FLAG_ALPHA 2
#define FLAG_BETA 3

#define MATE_SCORE (INT16_MAX / 2)
#define INFTY      (MATE_SCORE + 10) 
#define NO_SCORE   (INFTY + 1)

#define GENERATION_MASK 0b111111

struct Transposition {
    uint32_t hash;
    int16_t score;
    int16_t eval;
    Move move;
    int8_t depth;
    uint8_t flags : 2;
    uint8_t generation : 6;

    Transposition() = default;

    Transposition(uint8_t f, uint64_t h, int8_t d, int sc, int e, Move m, uint8_t gen) : 
    flags(f), hash(h & 0xFFFFFFFFU), depth(d), score(sc), move(m), eval(e), generation(gen) {};
};

using Cluster = std::array<Transposition, 3>;

#define NO_HASH_ENTRY { FLAG_EMPTY, 0, 0, NO_SCORE, NO_SCORE, Move::none(), 0 }
#define DEFAULT_CAPACITY (1ULL << 20)
#define MAX_CAPACITY (1ULL << 25)

class TTable {
    private:

        std::vector<Cluster> m_map;
        size_t m_capacity;
        size_t m_stored;

    public:
        TTable(size_t capacity) :
            m_capacity(capacity),
            m_stored(0)
        {
            if (capacity > MAX_CAPACITY)
                m_capacity = MAX_CAPACITY;

            m_map.resize(m_capacity);
        }

        TTable() : m_capacity(DEFAULT_CAPACITY), m_stored(0) { m_map.resize(m_capacity); };

        inline size_t stored() const { return m_stored; }
        inline size_t capacity() const { return m_capacity; }

        inline void resize(size_t new_capacity) { m_map.resize(new_capacity); m_capacity = new_capacity; }
        inline void clear();

        inline void push(uint64_t hash, const Transposition& t);
        inline std::tuple<bool, Transposition> probe(uint64_t hash) const;
};

inline void TTable::push(uint64_t hash, const Transposition& t)
{
    Cluster& c = m_map[mul_hi64(hash, m_map.size())];

    int candidate = -1;
    int age       = -1;
    int depth     = INT32_MAX;

    for (int i = 0; i < 3; ++i) {
        if (c[i].flags == FLAG_EMPTY) {
            candidate = i;
            m_stored++;
            break;
        }

        if (c[i].hash == t.hash) {
            if (t.depth >= c[i].depth)
                c[i] = t;
            else
                c[i].generation = t.generation;

            return;
        }

        const uint8_t entry_age = (t.generation + 64U - c[i].generation) & GENERATION_MASK;

        if (entry_age > age || (entry_age == age && c[i].depth < depth)) {
            candidate = i;
            age       = entry_age;
            depth     = c[i].depth;
        }
    }

    if (candidate >= 0)
        c[candidate] = t;
}

inline std::tuple<bool, Transposition> TTable::probe(uint64_t hash) const
{
    const Cluster& c = m_map[mul_hi64(hash, m_map.size())];

    for (int i = 0; i < 3; ++i)
        if (c[i].hash == (hash & 0xFFFFFFFFU))
            return {true, c[i]};

    return {false, NO_HASH_ENTRY};
}

inline void TTable::clear()
{
    m_map.clear();
    m_map.resize(m_capacity);
    m_stored = 0;
}

#endif
