#ifndef MATCHINGTEMPLATEINSTANTIATION_H
#define MATCHINGTEMPLATEINSTANTIATION_H

#include <swisssystems/burstein.h>
#include <swisssystems/dutch.h>

#ifdef OMIT_BURSTEIN
#define MATCHING_EDGE_WEIGHT_BURSTEIN(a)
#else
#define MATCHING_EDGE_WEIGHT_BURSTEIN(a) \
a(swisssystems::burstein::matching_computer::edge_weight)
#endif

#ifdef OMIT_DUTCH
#define MATCHING_EDGE_WEIGHTS_DUTCH(a)
#else
#define MATCHING_EDGE_WEIGHTS_DUTCH(a) \
a(swisssystems::dutch::validity_matching_computer::edge_weight) \
a(swisssystems::dutch::optimality_matching_computer::edge_weight)
#endif

// This macro is called in the cpp files of the matching code to instantiate the
// templates needed for the Swiss systems.

#define INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(a) \
MATCHING_EDGE_WEIGHT_BURSTEIN(a) MATCHING_EDGE_WEIGHTS_DUTCH(a)

#endif
