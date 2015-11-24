#include <iostream>

#include <boost/program_options.hpp>

#include "model.h"

using namespace mht;

int main(int argc, char** argv) {
	namespace po = boost::program_options;

	std::string modelFilename;
	std::string outputFilename;
	std::string weightsFilename;

	// Declare the supported options.
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
	    ("model,m", po::value<std::string>(&modelFilename), "filename of model stored as Json file")
	    ("weights,w", po::value<std::string>(&weightsFilename), "filename of the weights stored as Json file")
	    ("output,o", po::value<std::string>(&outputFilename), "filename where the resulting tracking (as links) will be stored as Json file")
	;

	po::variables_map variableMap;
	po::store(po::parse_command_line(argc, argv, description), variableMap);
	po::notify(variableMap);

	if (variableMap.count("help")) {
	    std::cout << description << std::endl;
	    return 1;
	}

	if (!variableMap.count("model") || !variableMap.count("output") || !variableMap.count("weights")) {
	    std::cout << "Model, Weights and Output filenames have to be specified!" << std::endl;
	    std::cout << description << std::endl;
	} else {
	    Model model;
		model.readFromJson(modelFilename);
		std::vector<double> weights = readWeightsFromJson(weightsFilename);
		Solution solution = model.infer(weights);
		model.saveResultToJson(outputFilename, solution);
	}
}
