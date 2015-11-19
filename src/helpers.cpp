#include "helpers.h"

namespace mht
{

std::ostream& operator<<(std::ostream& stream, const FeatureVector& feats)
{
	stream << "(";
	for(FeatureVector::const_iterator f_it = feats.begin(); f_it != feats.end(); ++f_it)
	{
		if(f_it != feats.begin())
			stream << ", ";
		stream << *f_it;
	}
	stream << ")";
	return stream;
}

} // end namespace mht
