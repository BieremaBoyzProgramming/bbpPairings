#ifndef MATCHINGTEMPLATEINSTANTIATION_H
#define MATCHINGTEMPLATEINSTANTIATION_H

#include <swisssystems/burstein.h>
#include <swisssystems/dubov.h>
#include <swisssystems/dutch.h>
#include <swisssystems/lim.h>

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
a(swisssystems::dutch::matching_computer::edge_weight)
#endif

// The Dubov system uses the same edge weight type as the Dutch system, so it
// only needs its own instantiation when the Dutch system is omitted.
#if defined(OMIT_DUBOV) || !defined(OMIT_DUTCH)
#define MATCHING_EDGE_WEIGHTS_DUBOV(a)
#else
#define MATCHING_EDGE_WEIGHTS_DUBOV(a) \
a(swisssystems::dubov::matching_computer::edge_weight)
#endif

// The Lim system uses the same edge weight type as the Dutch and Dubov
// systems, so it only needs its own instantiation when both are omitted.
#if defined(OMIT_LIM) || !defined(OMIT_DUTCH) || !defined(OMIT_DUBOV)
#define MATCHING_EDGE_WEIGHTS_LIM(a)
#else
#define MATCHING_EDGE_WEIGHTS_LIM(a) \
a(swisssystems::lim::matching_computer::edge_weight)
#endif

// This macro is called in the cpp files of the matching code to instantiate the
// templates needed for the Swiss systems.

#define INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(a) \
MATCHING_EDGE_WEIGHT_BURSTEIN(a) MATCHING_EDGE_WEIGHTS_DUTCH(a) \
MATCHING_EDGE_WEIGHTS_DUBOV(a) MATCHING_EDGE_WEIGHTS_LIM(a)

#endif
