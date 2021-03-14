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
