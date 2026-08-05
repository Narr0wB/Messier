
#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

/* Search */
const int time_check_nodes      = 2048;
const int qsearch_see_threshold = 0;

/* Futility pruning */
const int fp_depth  = 3;
const int fp_margin = 500;

/* Reverse futility pruning */
const int rfp_base_margin = 65;
const int rfp_depth       = 8;

/* Null move pruning */
const int nmp_depth          = 2;
const int nmp_npawn_material = 4;

/* Internal Iterative Reductions */
const int iir_depth = 7;

/* Delta margin */
const int delta_margin = 100;

/* Movepicker */
const int good_quiet_threshold   = -1;
const int good_capture_threshold = 0;

#endif // PARAMETERS_HPP