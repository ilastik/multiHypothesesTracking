#include "conservation/variable.h"
#include "helpers.h"

using namespace helpers;

namespace constracking
{

void Variable::addToOpenGM(
	GraphicalModelType& model, 
	bool statesShareWeights,
	WeightsType& weights, 
	const std::vector<size_t>& weightIds)
{
	// only add variable if there are any features
	if(features_.size() == 0 || features_[0].size() == 0)
		return;

	// Add variable to model. All Variables are binary!
	size_t numStates = getNumStates();
	model.addVariable(numStates);
	openGMVariableId_ = model.numberOfVariables() - 1;

	// add unary factor to model
	std::vector<FeaturesAndIndicesType> featuresAndWeightsPerLabel;
	assert(weightIds.size() == getNumWeights(statesShareWeights));

	// if weights are not shared over states, we need to keep track how many weights have been used before
	size_t weightIdx = 0;

	for(size_t state = 0; state < numStates; state++)
	{
		FeaturesAndIndicesType featureAndIndex;

		featureAndIndex.features = features_[state];
		for(size_t i = 0; i < features_[state].size(); ++i)
		{
			if(statesShareWeights)
				featureAndIndex.weightIds.push_back(weightIds[i]);
			else
				featureAndIndex.weightIds.push_back(weightIds[weightIdx++]);
		}

		featuresAndWeightsPerLabel.push_back(featureAndIndex);
	}

	LearnableUnaryFuncType unary(weights, featuresAndWeightsPerLabel);
	GraphicalModelType::FunctionIdentifier fid = model.addFunction(unary);
	model.addFactor(fid, &openGMVariableId_, &openGMVariableId_+1);
}

const int Variable::getNumWeights(bool statesShareWeights) const
{
	int numWeights = -1;

	if(features_.size() > 0 && features_.at(0).size() > 0)
	{
		if(statesShareWeights)
		{
			numWeights = features_.at(0).size();

			// sanity check
			for(size_t i = 1; i < features_.size(); ++i)
				if(features_.at(i).size() != numWeights)
					throw std::runtime_error("Number of features must be equal for all states!");
		}
		else
		{
			for(size_t i = 0; i < features_.size(); ++i)
				numWeights += features_.at(i).size();
		}
	}

	return numWeights;
}

} // end namespace conservation
