#include <iostream>

#include <boost/program_options.hpp>

#include "jsonmodel.h"
#include "helpers.h"

using namespace mht;
using namespace helpers;

int main(int argc, char** argv) {
	namespace po = boost::program_options;

	std::string modelFilename;
	std::string solutionFilename;
	std::string weightsFilename;

	// Declare the supported options.
	po::options_description description("Allowed options");
	description.add_options()
	    ("help", "produce help message")
	    ("model,m", po::value<std::string>(&modelFilename), "filename of model stored as Json file")
	    ("solution,s", po::value<std::string>(&solutionFilename), "filename where the tracking solution (as links) is stored as Json file")
	    ("weights,w", po::value<std::string>(&weightsFilename), "filename of the weights stored as Json file")
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
	    JsonModel model;
		model.readFromJson(modelFilename);
		WeightsType weights(model.computeNumWeights());
		
		if(variableMap.count("weights") > 0)
		{
			std::vector<double> weightVec = readWeightsFromJson(weightsFilename);
			for(size_t i = 0; i < weightVec.size(); i++)
				weights.setWeight(i, weightVec[i]);
		}
		
		model.initializeOpenGMModel(weights);
		model.setJsonGtFile(solutionFilename);
		bool valid = false;
		Solution solution;

		try
		{
			solution = model.getGroundTruth();
			valid = model.verifySolution(solution);
		}
		catch(...)
		{
			std::cout << "Could not read ground truth." << std::endl;
		}

		std::cout << "Is solution valid? " << (valid? "yes" : "no") << std::endl;

		if(valid && variableMap.count("weights") > 0)
		{
			std::cout << "Solution has energy: " << model.evaluateSolution(solution) << std::endl;
			Solution zeros(solution.size(), 0);
			std::cout << "(state zero has energy: " << model.evaluateSolution(zeros) << ")" << std::endl;
		}
	}
	return 0;
}
