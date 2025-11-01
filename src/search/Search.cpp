
#include <engine/search/Search.hpp>



namespace Search {

    
   
    int mate_in(int ply) {
        return MATE_SCORE - ply;
    }

    int mated_in(int ply) {
        return -MATE_SCORE + ply;
    }

} // namespace Search
