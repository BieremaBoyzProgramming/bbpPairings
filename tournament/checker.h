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


#ifndef CHECKER_H
#define CHECKER_H

#include <ostream>
#include <string>

#include <swisssystems/common.h>

#include "tournament.h"

namespace tournament
{
  namespace checker
  {
    void check(
      const tournament::Tournament &,
      swisssystems::SwissSystem,
      std::ostream *,
      const std::string &);
  }
}

#endif
