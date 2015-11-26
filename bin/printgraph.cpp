#include <iostream>

#include <boost/program_options.hpp>

#include "multihypothesis/model.h"
#include "helpers.h"

using namespace mht;
using namespace helpers;

int main(int argc, char** argv) {
	namespace po = boost::program_options;

	std::string modelFilename;
	std::string solutionFilename;
	std::string outputFilename("graph.dot");

	// Declare the supported options.
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
	    ("model,m", po::value<std::string>(&modelFilename), "filename of model stored as Json file")
	    ("solution,s", po::value<std::string>(&solutionFilename), "(optional) filename where the tracking solution (as links) is stored as Json file")
	    ("output,o", po::value<std::string>(&outputFilename), "filename where the graphviz DOT print of the graph should go")
	;

	po::variables_map variableMap;
	po::store(po::parse_command_line(argc, argv, description), variableMap);
	po::notify(variableMap);

	if (variableMap.count("help")) {
	    std::cout << description << std::endl;
	    return 1;
	}

	if (!variableMap.count("model") || !variableMap.count("output")) {
	    std::cout << "Model and Output filenames have to be specified!" << std::endl;
	    std::cout << description << std::endl;
	} else {
	    Model model;
		model.readFromJson(modelFilename);
		WeightsType weights(model.computeNumWeights());
		model.initializeOpenGMModel(weights);

		// print with given solution if any
		if(solutionFilename.size() > 0)
		{
			Solution solution = model.readGTfromJson(solutionFilename);
			model.toDot(outputFilename, &solution);
		}
		else
		{
			model.toDot(outputFilename);
		}
	}
}
