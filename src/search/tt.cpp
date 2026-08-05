
#include "tt.hpp"
#include "log.hpp"
#include "misc.hpp"

void TTable::resize(size_t new_capacity)
{
    m_map.resize(new_capacity);
    m_capacity = new_capacity;
}

void TTable::clear()
{
    m_map.clear();
    m_map.resize(m_capacity);
    m_stored = 0;
}

void TTable::push(uint64_t hash, const Transposition& t)
{
    Cluster& c = m_map[mul_hi64(hash, m_map.size())];

    int candidate = -1;
    int age = INT32_MAX;
    int depth = INT32_MAX;

    for (int i = 0; i < 3; ++i) {
        if (c[i].flags == FLAG_EMPTY) {
            candidate = i;
            m_stored++;
            break;
        }

        if (c[i].hash == t.hash
            && (t.generation > c[i].generation || t.depth >= c[i].depth)) 
        {
            candidate = i;
            break;
        }

        if (c[i].generation < age || (c[i].generation == age && c[i].depth < depth)) {
            candidate = i;
            age = c[i].generation;
            depth = c[i].depth;
        }
    }

    if (candidate >= 0)
        c[candidate] = t;
}

std::tuple<bool, Transposition> TTable::probe(uint64_t hash) const
{
    const Cluster& c = m_map[mul_hi64(hash, m_map.size())];

    for (int i = 0; i < 3; ++i)
        if (c[i].hash == (hash & 0xFFFFFFFFU)) 
            return {true, c[i]};

    return {false, NO_HASH_ENTRY};
}