#include <fstream>
#include <json/json.h>
#include "helpers.h"

namespace mht
{

std::map<JsonTypes, std::string> JsonTypeNames = {
	{JsonTypes::Segmentations, "segmentation-hypotheses"}, 
	{JsonTypes::Links, "linking-hypotheses"}, 
	{JsonTypes::Exclusions, "exclusions"},
	{JsonTypes::LinkResults, "linking-results"},
	{JsonTypes::SrcId, "src"}, 
	{JsonTypes::DestId, "dest"}, 
	{JsonTypes::Value, "value"},
	{JsonTypes::Id, "id"}, 
	{JsonTypes::Features, "features"},
	{JsonTypes::DivisionFeatures, "divisionFeatures"},
	{JsonTypes::Weights, "weights"},
};

void saveWeightsToJson(const std::vector<ValueType>& weights, const std::string& filename)
{
	std::ofstream output(filename.c_str());
	if(!output.good())
		throw std::runtime_error("Could not open JSON weight file for saving: " + filename);

	Json::Value root;
	Json::Value& weightsJson = root[JsonTypeNames[JsonTypes::Weights]];

	for(auto w : weights)
	{
		weightsJson.append(Json::Value(w));
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


} // end namespace mht
