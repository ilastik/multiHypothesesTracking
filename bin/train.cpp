#include <iostream>

#include <boost/program_options.hpp>

#include "jsonmodel.h"
#include "helpers.h"

using namespace mht;
using namespace helpers;

int main(int argc, char** argv) {
	namespace po = boost::program_options;

	std::string modelFilename;
	std::string groundtruthFilename;
	std::string weightsFilename("weights.json");

	// Declare the supported options.
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
	    ("model,m", po::value<std::string>(&modelFilename), "filename of model stored as Json file")
	    ("groundtruth,g", po::value<std::string>(&groundtruthFilename), "filename of ground truth stored as Json file")
	    ("weights,w", po::value<std::string>(&weightsFilename), "filename where the resulting weights will be stored as Json file")
	;

	po::variables_map variableMap;
	po::store(po::parse_command_line(argc, argv, description), variableMap);
	po::notify(variableMap);

	if (variableMap.count("help")) 
	{
	    std::cout << description << std::endl;
	    return 1;
	}

	if (!variableMap.count("model") || !variableMap.count("groundtruth")) 
	{
	    std::cout << "Model and Groundtruth filenames have to be specified!" << std::endl;
	    std::cout << description << std::endl;
	} 
	else 
	{
	    JsonModel model;
		model.readFromJson(modelFilename);
		model.setJsonGtFile(groundtruthFilename);
		std::vector<double> weights = model.learn();
		std::vector<std::string> weightDescriptions = model.getWeightDescriptions();
		saveWeightsToJson(weights, weightsFilename, weightDescriptions);
	}
}
