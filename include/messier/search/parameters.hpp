
#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#define MAX_DEPTH 25
#define MAX_PLY   30 

/* Search */
constexpr int time_check_nodes      = 2048;
constexpr int qsearch_see_threshold = 0;

/* Futility pruning */
constexpr int fp_depth  = 3;
constexpr int fp_margin = 500;

/* Reverse futility pruning */
constexpr int rfp_base_margin = 65;
constexpr int rfp_depth       = 3;

/* Razoring */
constexpr int razoring_base  = 60;
constexpr int razoring_depth = 2;

/* Null move pruning */
constexpr int nmp_depth          = 2;
constexpr int nmp_npawn_material = 4;

/* Internal Iterative Reductions */
constexpr int iir_depth = 7;

/* Delta margin */
constexpr int delta_margin = 100;

/* Movepicker */
constexpr int good_quiet_threshold   = -1;
constexpr int good_capture_threshold = 0;
constexpr int moves_before_sorting   = 128;

#endif // PARAMETERS_HPP