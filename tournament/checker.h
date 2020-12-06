#ifndef CHECKER_H
#define CHECKER_H

#include <ostream>
#include <string>

#include <swisssystems/common.h>

#include "tournament.h"

#ifndef OMIT_CHECKER
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

#endif
