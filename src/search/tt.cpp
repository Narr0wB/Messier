
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
}

void TTable::push(uint64_t hash, const Transposition& t)
{
    Transposition& e = m_map[mul_hi64(hash, m_map.size())];
    if (e.hash == hash || t.depth >= e.depth) 
        e = t;
}

std::tuple<bool, Transposition> TTable::probe(uint64_t hash) 
{
    Transposition entry = m_map[mul_hi64(hash, m_map.size())];
    if (entry.hash == hash && entry.flags != FLAG_EMPTY) return {true, entry};

    return {false, NO_HASH_ENTRY};
}