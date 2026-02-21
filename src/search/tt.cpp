
#include "tt.hpp"
#include "../log.hpp"
#include "../misc.hpp"

void TTable::resize(uint64_t new_capacity)
{
    m_map.resize(new_capacity);
    m_capacity = new_capacity;
}

void TTable::clear()
{
    m_map.clear();
    m_map.reserve(m_capacity);
}

void TTable::push(uint64_t hash, const Transposition& t)
{
    Transposition& entry = m_map[mul_hi64(hash, m_capacity)];
    entry = t;
    m_stored++;
}

std::tuple<bool, Transposition> TTable::probe(uint64_t hash) 
{
    Transposition entry = m_map[mul_hi64(hash, m_capacity)];

    if (entry.hash == hash) {
        m_hits++;
        return {true, entry};
    }

    return {false, NO_HASH_ENTRY};
}

// void TranspositionTable::push_position(Transposition t)
// {
//     Transposition& entry = m_DataArray[t.hash % m_Capacity];

//     if (entry.flags == FLAG_EMPTY) {
//         // If the entry is empty then set entry equal to t
//         entry = t;
//         m_Size++;
//     }
//     else {
//         entry = t;
//         // If the entry is not empty first check if its the same position
//         if (entry.hash == t.hash) {

//             // If the depth of the transposition (t) is higher
//             // then replace entry with t
//             if (t.depth > entry.depth) {
//                 entry = t;
//             }
//         }
//         else {
//             if (t.depth > entry.depth) {
//                 entry = t;
//             }
//         }
//     }
// }

// Transposition TranspositionTable::probe_hash(uint64_t hash) 
// {
//     Transposition position = m_DataArray[hash % m_Capacity];

//     if (position.hash == hash) {
//         hits++;
//         return position;
//     }

//     return NO_HASH_ENTRY;
// }

// void TranspositionTable::clear() 
// {
//     if (m_DataArray != NULL) {
//         std::memset(m_DataArray, 0, m_Capacity * sizeof(Transposition));
//     }
// }

// // WARNING: Calling this function will delete all the transpositions stored inside of the table!
// void TranspositionTable::realloc(size_t numberOfTranspositions) 
// {
//     delete m_DataArray;

//     m_DataArray = new Transposition[numberOfTranspositions];
//     m_Capacity = numberOfTranspositions;

//     std::memset(m_DataArray, 0, sizeof(Transposition) * m_Capacity);
// }
