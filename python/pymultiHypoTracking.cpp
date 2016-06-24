#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python.hpp>

#include "pythonmodel.h"
#include "helpers.h"

using namespace mht;
using namespace boost::python;
using namespace helpers;

object track(object& graphDict, object& weightsDict)
{
	dict pyGraph = extract<dict>(graphDict);
	dict pyWeights = extract<dict>(weightsDict);

	PythonModel model;
	model.readFromPython(pyGraph);
	FeatureVector weights = readWeightsFromPython(pyWeights);
	Solution solution = model.infer(weights);
	object result = model.saveResultToPython(solution);
    
	return result;
}

/**
 * @brief Python interface of 'mht' module
 */
BOOST_PYTHON_MODULE( multiHypoTracking )
{
	def("track", track, args("graph", "weights"),
		"Use an ILP solver on a graph specified as a dictionary,"
		"in the same structure as the supported JSON format. Similarly, the weights are also given as dict.\n\n"
		"Returns a python dictionary similar to the result.json file");
}
