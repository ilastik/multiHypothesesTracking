#include <fstream>
#include <json/json.h>
#include "helpers.h"

namespace helpers
{

std::map<JsonTypes, std::string> JsonTypeNames = {
	{JsonTypes::Segmentations, "segmentationHypotheses"}, 
	{JsonTypes::Links, "linkingHypotheses"}, 
	{JsonTypes::Exclusions, "exclusions"},
	{JsonTypes::LinkResults, "linkingResults"},
	{JsonTypes::DivisionResults, "divisionResults"},
	{JsonTypes::DetectionResults, "detectionResults"},
	{JsonTypes::SrcId, "src"}, 
	{JsonTypes::DestId, "dest"}, 
	{JsonTypes::Value, "value"},
	{JsonTypes::Id, "id"}, 
	{JsonTypes::Features, "features"},
	{JsonTypes::DivisionFeatures, "divisionFeatures"},
	{JsonTypes::AppearanceFeatures, "appearanceFeatures"},
	{JsonTypes::DisappearanceFeatures, "disappearanceFeatures"},
	{JsonTypes::Weights, "weights"},
	{JsonTypes::StatesShareWeights, "statesShareWeights"}
};

void saveWeightsToJson(
	const std::vector<ValueType>& weights, 
	const std::string& filename, 
	const std::vector<std::string>& weightDescriptions)
{
	if(weightDescriptions.size() > 0 && weightDescriptions.size() != weights.size())
		throw std::runtime_error("Length of weight descriptions must match length of weights if given");

	std::ofstream output(filename.c_str());
	if(!output.good())
		throw std::runtime_error("Could not open JSON weight file for saving: " + filename);

	Json::Value root;
	Json::Value& weightsJson = root[JsonTypeNames[JsonTypes::Weights]];

	for(size_t i = 0; i < weights.size(); i++)
	{
		Json::Value v(weights[i]);
		if(weightDescriptions.size() > 0)
			v.setComment("// " + weightDescriptions[i], Json::commentAfterOnSameLine);
		weightsJson.append(v);
	}

	if(!weightsJson.isArray())
		throw std::runtime_error("Cannot save Weights to non-array JSON entry");

	output << root << std::endl;
}

std::vector<ValueType> readWeightsFromJson(const std::string& filename)
{
	std::ifstream input(filename.c_str());
	if(!input.good())
		throw std::runtime_error("Could not open JSON weight file for reading: " + filename);

	Json::Value root;
	input >> root;

	if(!root.isMember(JsonTypeNames[JsonTypes::Weights]))
		throw std::runtime_error("Could not find 'Weights' group in JSON file");
	
	const Json::Value entry = root[JsonTypeNames[JsonTypes::Weights]];
	if(!entry.isArray())
		throw std::runtime_error("Cannot extract Weights from non-array JSON entry");

	std::vector<ValueType> weights;
	for(int i = 0; i < (int)entry.size(); i++)
	{
		weights.push_back(entry[i].asDouble());
	}
	return weights;
}

StateFeatureVector extractFeatures(const Json::Value& entry, JsonTypes type)
{
	StateFeatureVector stateFeatVec;
	if(!entry.isMember(JsonTypeNames[type]))
		throw std::runtime_error("Could not find Json tags for " + JsonTypeNames[type]);

	const Json::Value featuresPerState = entry[JsonTypeNames[type]];

	if(!featuresPerState.isArray())
		throw std::runtime_error(JsonTypeNames[type] + " must be an array");

	if(!featuresPerState.size() > 0)
		throw std::runtime_error("Features may not be empty for " + JsonTypeNames[type]);

	// std::cout << "\tReading features for: " << JsonTypeNames[type] << std::endl;

	// get the features per state
	for(int i = 0; i < (int)featuresPerState.size(); i++)
	{
		// get features for the specific state
		FeatureVector featVec;
		const Json::Value& featuresForState = featuresPerState[i];

		if(!featuresForState.isArray())
			throw std::runtime_error("Expected to find a list of features for each state");

		if(!featuresForState.size() > 0)
		throw std::runtime_error("Features for state may not be empty for " + JsonTypeNames[type]);

		for(int j = 0; j < (int)featuresForState.size(); j++)
		{
			featVec.push_back(featuresForState[j].asDouble());
		}

		// std::cout << "\t\tfound " << featVec.size() << " features for state " << i << std::endl;

		stateFeatVec.push_back(featVec);
	}

	return stateFeatVec;
}

void addOpenGMVariableToConstraint(
	LinearConstraintFunctionType::LinearConstraintType& constraint, 
	size_t opengmVariableId,
	size_t state, 
	double coefficient,
	std::vector<LabelType>& constraintShape,
	std::vector<LabelType>& factorVariables,
	GraphicalModelType& model)
{
	IndicatorVariableType indicatorVariable(constraintShape.size(), LabelType(state));
    constraint.add(indicatorVariable, coefficient);

    factorVariables.push_back(opengmVariableId);
    constraintShape.push_back(model.numberOfLabels(opengmVariableId));
}

void addOpenGMVariableStateToConstraint(
	LinearConstraintFunctionType::LinearConstraintType& constraint, 
	size_t opengmVariableId,
	double coefficient,
	std::vector<LabelType>& constraintShape,
	std::vector<LabelType>& factorVariables,
	GraphicalModelType& model)
{
	size_t numStates = model.numberOfLabels(opengmVariableId);

	for(size_t i = 1; i < numStates; i++)
	{
		IndicatorVariableType indicatorVariable(constraintShape.size(), LabelType(i));
	    constraint.add(indicatorVariable, coefficient * i);
	}

    factorVariables.push_back(opengmVariableId);
    constraintShape.push_back(numStates);
}

} // end namespace mht
