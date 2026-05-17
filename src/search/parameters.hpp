
#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

/* Futility pruning */
const int fp_depth = 3;
const int fp_margin = 500;

/* Reverse futility pruning */
const int rfp_base_margin = 65;
const int rfp_depth       = 8;

/* Null move pruning */
const int nmp_depth          = 2;
const int nmp_npawn_material = 0;

/* Internal Iterative Reductions */
const int iir_depth = 7;

#endif // PARAMETERS_HPP