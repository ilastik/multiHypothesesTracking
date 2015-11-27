#include <iostream>

#include <boost/program_options.hpp>

#include "model.h"
#include "helpers.h"

using namespace mht;
using namespace helpers;

int main(int argc, char** argv) {
	namespace po = boost::program_options;

	std::string modelFilename;
	std::string solutionFilename;

	// Declare the supported options.
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
	    ("model,m", po::value<std::string>(&modelFilename), "filename of model stored as Json file")
	    ("solution,s", po::value<std::string>(&solutionFilename), "filename where the tracking solution (as links) is stored as Json file")
	;

	po::variables_map variableMap;
	po::store(po::parse_command_line(argc, argv, description), variableMap);
	po::notify(variableMap);

	if (variableMap.count("help")) {
	    std::cout << description << std::endl;
	    return 1;
	}

	if (!variableMap.count("model") || !variableMap.count("solution")) {
	    std::cout << "Model and Solution filenames have to be specified!" << std::endl;
	    std::cout << description << std::endl;
	} else {
	    Model model;
		model.readFromJson(modelFilename);
		WeightsType weights(model.computeNumWeights());
		model.initializeOpenGMModel(weights);
		Solution solution = model.readGTfromJson(solutionFilename);
		bool valid = model.verifySolution(solution);
		std::cout << "Is solution valid? " << (valid? "yes" : "no") << std::endl;
	}
}
