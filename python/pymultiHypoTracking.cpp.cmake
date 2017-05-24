#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python.hpp>

#include "pythonmodel.h"
#include "helpers.h"

using namespace mht;
using namespace boost::python;
using namespace helpers;

/**
 * @brief Helper class to release / lock the Python GIL
 */
class ScopedGILRelease {
public:
    inline ScopedGILRelease() { threadState_ = PyEval_SaveThread(); }
    inline ~ScopedGILRelease() 
    {
        PyEval_RestoreThread(threadState_);
        threadState_ = NULL;
    }
private:
    PyThreadState* threadState_;
};

object track(object& graphDict, object& weightsDict)
{
	dict pyGraph = extract<dict>(graphDict);
	dict pyWeights = extract<dict>(weightsDict);
	
	PythonModel model;
	model.readFromPython(pyGraph);
	FeatureVector weights = readWeightsFromPython(pyWeights);
	Solution solution;

	{
		ScopedGILRelease gilLock;
		solution = model.infer(weights);
	}
    
	object result = model.saveResultToPython(solution);
	return result;
}

object train(object& graphDict, object& gtDict)
{
	dict pyGraph = extract<dict>(graphDict);
	dict pyGt = extract<dict>(gtDict);
	PythonModel model;
	model.readFromPython(pyGraph);
	model.setPythonGt(pyGt);
	std::vector<double> weights;
	
	{
		// Not sure whether releasing the GIL here is safe,
		// because this actually calls model.getGroundTruth(), which reads from python objects...
		// ScopedGILRelease gilLock;
		weights = model.learn();
	}
		
	object result = model.saveWeightsToPython(weights);
    
	return result;
}

object trainWithWeightInitialization(object& graphDict, object& gtDict, object& weightsDict)
{
	dict pyGraph = extract<dict>(graphDict);
	dict pyGt = extract<dict>(gtDict);
	dict pyWeights = extract<dict>(weightsDict);
	PythonModel model;
	std::vector<double> weights;
	model.readFromPython(pyGraph);
	model.setPythonGt(pyGt);

	FeatureVector weightInitialization = readWeightsFromPython(pyWeights);

	{
		// Not sure whether releasing the GIL here is safe,
		// because this actually calls model.getGroundTruth(), which reads from python objects...
		// ScopedGILRelease gilLock;
		weights = model.learn(weightInitialization); 
	}
	
	object result = model.saveWeightsToPython(weights);
    
	return result;
}

bool validate(object& graphDict, object& gtDict)
{
	dict pyGraph = extract<dict>(graphDict);
	dict pyGt = extract<dict>(gtDict);
	PythonModel model;
	model.readFromPython(pyGraph);
	model.setPythonGt(pyGt);
	Solution solution = model.getGroundTruth();
	bool valid;

	{
		ScopedGILRelease gilLock;
		valid = model.verifySolution(solution);
	}
	
	return valid;
}

/**
 * @brief Python interface of 'mht' module
 */
BOOST_PYTHON_MODULE( multiHypoTracking@SUFFIX@ )
{
	def("track", track, args("graph", "weights"),
		"Use an ILP solver on a graph specified as a dictionary,"
		"in the same structure as the supported JSON format. Similarly, the weights are also given as dict.\n\n"
		"Returns a python dictionary similar to the result.json file");
	def("train", train, args("graph", "groundTruth"),
		"Run Structured Learning with an ILP solver on a graph specified as a dictionary,"
		"in the same structure as the supported JSON format." 
		"Similarly, the ground truth are also given as dict as in a result.json file .\n\n"
		"Returns a python dictionary containing a weights entry");
	def("trainWithWeightInitialization", trainWithWeightInitialization, args("graph", "groundTruth", "initialWeights"),
		"Run Structured Learning with an ILP solver on a graph specified as a dictionary,"
		"in the same structure as the supported JSON format. Similarly, the weights are also given as dict." 
		"Similarly, the ground truth are also given as dict as in a result.json file .\n\n"
		"Returns a python dictionary containing a weights entry");
	def("validate", train, args("graph", "solution"),
		"Validate a solution on a graph specified as a dictionary,"
		"in the same structure as the supported JSON format." 
		"Similarly, the solution is also given as dict as in a result.json file .\n\n"
		"Returns a boolean whether the solution is valid");
}
