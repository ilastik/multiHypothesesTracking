#include "settings.h"

namespace helpers
{

Settings::Settings(const Json::Value& entry)
{
	if(entry.isMember(JsonTypeNames[JsonTypes::StatesShareWeights]))
		statesShareWeights_ = entry[JsonTypeNames[JsonTypes::StatesShareWeights]].asBool();
	else 
		statesShareWeights_ = false;

	if(entry.isMember(JsonTypeNames[JsonTypes::AllowPartialMergerAppearance]))
		allowPartialMergerAppearance_ = entry[JsonTypeNames[JsonTypes::AllowPartialMergerAppearance]].asBool();
	else 
		allowPartialMergerAppearance_ = true;

	if(entry.isMember(JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]))
		requireSeparateChildrenOfDivision_ = entry[JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]].asBool();
	else 
		requireSeparateChildrenOfDivision_ = false;

	if(entry.isMember(JsonTypeNames[JsonTypes::OptimizerEpGap]))
		optimizerEpGap_ = entry[JsonTypeNames[JsonTypes::OptimizerEpGap]].asDouble();
	else 
		optimizerEpGap_ = 0.01;

	if(entry.isMember(JsonTypeNames[JsonTypes::OptimizerVerbose]))
		optimizerVerbose_ = entry[JsonTypeNames[JsonTypes::OptimizerVerbose]].asBool();
	else 
		optimizerVerbose_ = true;

	if(entry.isMember(JsonTypeNames[JsonTypes::OptimizerNumThreads]))
		optimizerNumThreads_ = entry[JsonTypeNames[JsonTypes::OptimizerNumThreads]].asUInt();
	else 
		optimizerNumThreads_ = 1;
}

void Settings::saveToJson(Json::Value& entry)
{
	entry[JsonTypeNames[JsonTypes::StatesShareWeights]] = Json::Value(statesShareWeights_);
	entry[JsonTypeNames[JsonTypes::AllowPartialMergerAppearance]] = Json::Value(allowPartialMergerAppearance_);
	entry[JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]] = Json::Value(requireSeparateChildrenOfDivision_);
	entry[JsonTypeNames[JsonTypes::OptimizerEpGap]] = Json::Value(optimizerEpGap_);
	entry[JsonTypeNames[JsonTypes::OptimizerVerbose]] = Json::Value(optimizerVerbose_);
	entry[JsonTypeNames[JsonTypes::OptimizerNumThreads]] = Json::Value((int)optimizerNumThreads_);
}

void Settings::print()
{
	std::cout << "************************\n"
		<< "Settings are:"
		<< "\n\tStatesShareWeights: " << (statesShareWeights_ ? "true" : "false")
		<< "\n\tAllowPartialMergerAppearance: " << (allowPartialMergerAppearance_ ? "true" : "false")
		<< "\n\tRequireSeparateChildrenOfDivision: " << (requireSeparateChildrenOfDivision_ ? "true" : "false")
		<< "\n\tOptimizerEpGap: " << optimizerEpGap_
		<< "\n\tOptimizerVerbose: " << (optimizerVerbose_ ? "true" : "false")
		<< "\n\tOptimizerNumThreads: " << optimizerNumThreads_
		<< "\n************************"
		<< std::endl;
}
	
} // end namespace helpers
