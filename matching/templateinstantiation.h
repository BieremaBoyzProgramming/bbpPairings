/*
 * This file is part of BBP Pairings, a Swiss-system chess tournament engine
 * Copyright (C) 2016  Jeremy Bierema
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3.0, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MATCHINGTEMPLATEINSTANTIATION_H
#define MATCHINGTEMPLATEINSTANTIATION_H

#include <swisssystems/burstein.h>

// These macros are called in the cpp files of the matching code to instantiate
// the templates needed for the Swiss systems.

#define MATCHINGEDGEWEIGHTPARAMETERS1(a, b) \
  a swisssystems::burstein::matching_computer::edge_weight b
#define MATCHINGEDGEWEIGHTPARAMETERS2(a, b, c) \
  a swisssystems::burstein::matching_computer::edge_weight \
  b swisssystems::burstein::matching_computer::edge_weight c
#define MATCHINGEDGEWEIGHTPARAMETERS3(a, b, c, d) \
  a swisssystems::burstein::matching_computer::edge_weight \
  b swisssystems::burstein::matching_computer::edge_weight \
  c swisssystems::burstein::matching_computer::edge_weight d

#endif
