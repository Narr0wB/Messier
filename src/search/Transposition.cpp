
#include <engine/search/Transposition.hpp>
#include <engine/Log.hpp>

void TranspositionTable::push_position(Transposition t)
{
    Transposition& entry = m_DataArray[t.hash % m_Capacity];

    if (entry.flags == FLAG_EMPTY) {
        // If the entry is empty then set entry equal to t
        entry = t;
        m_Size++;
    }
    else {
        entry = t;
        // If the entry is not empty first check if its the same position
        if (entry.hash == t.hash) {

            // If the depth of the transposition (t) is higher
            // then replace entry with t
            if (t.depth > entry.depth) {
                entry = t;
            }
        }
        else {
            if (t.depth > entry.depth) {
                entry = t;
            }
        }
    }
}

Transposition TranspositionTable::probe_hash(uint64_t hash) {
    Transposition position = m_DataArray[hash % m_Capacity];
    hits++;

    // // TODO: Since implememting fail soft search, must investigate the effect of fail-hard TT lookup (current)
    // if (position.hash == hash) {
    //     if (position.depth >= depth) {
    //         hits++;
    //         
    //         // if (position.flags != FLAG_EMPTY)
    //         //     return position;
    //
    //         if (position.flags == FLAG_EXACT) {
    //             return position;
    //         }
    //
    //         if (position.flags == FLAG_ALPHA && position.score <= alpha) {
    //             position.score = alpha;
    //             return position;
    //         }
    //
    //         if (position.flags == FLAG_BETA && position.score >= beta) {
    //             position.score = beta;
    //             return position;
    //         }
    //     }
    // }

    return position;
}

void TranspositionTable::clear() {
    if (m_DataArray != NULL) {
        std::memset(m_DataArray, 0, m_Capacity * sizeof(Transposition));
    }
}

// WARNING: Calling this function will delete all the transpositions stored inside of the table!
void TranspositionTable::realloc(size_t numberOfTranspositions) {
    delete m_DataArray;

    m_DataArray = new Transposition[numberOfTranspositions];
    m_Capacity = numberOfTranspositions;

    std::memset(m_DataArray, 0, sizeof(Transposition) * m_Capacity);
}
