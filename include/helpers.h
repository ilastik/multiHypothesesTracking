#ifndef HELPERS_H
#define HELPERS_H

#include <iostream>
#include <vector>

namespace covertree
{

// --------------------------------------------------------------
// typedefs
// --------------------------------------------------------------
typedef std::vector<double> FeatureVector;


// --------------------------------------------------------------
// functions
// --------------------------------------------------------------
std::ostream& operator<<(std::ostream& stream, const FeatureVector& feats);

}

#endif
