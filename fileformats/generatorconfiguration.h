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


#ifndef GENERATORCONFIGURATION_H
#define GENERATORCONFIGURATION_H

#include <istream>

#include <tournament/generator.h>

#ifndef OMIT_GENERATOR
namespace fileformats
{
  namespace generatorconfiguration
  {
    void readFile(
      tournament::generator::Configuration &,
      std::istream &);
  }
}
#endif

#endif
