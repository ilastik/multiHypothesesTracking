#include "variable.h"
#include "helpers.h"

#include <opengm/datastructures/marray/marray.hxx>

using namespace helpers;

namespace mht
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
	assert((int)weightIds.size() == getNumWeights(statesShareWeights));

	if(statesShareWeights)
	{
		// if we want to use the weights more than once, the construction is a bit more involved than in the else-branch
		size_t numFeatures = features_[0].size();
		std::vector<marray::Marray<double>> features; // for each feature, there will be its own Marray (which is a column for a unary)
		std::vector<size_t> coords(1, 0); // coordinate into a feature column

		for(size_t i = 0; i < numFeatures; ++i)
		{
			std::vector<size_t> shape(1, numStates);
	        marray::Marray<double> featureColumn(shape.begin(), shape.end(), 0);

	        for(size_t state = 0; state < numStates; ++state)
	        {
	        	coords[0] = state;
	        	featureColumn(coords.begin()) = features_[state][i];
	        }

	        features.push_back(featureColumn);
	    }

	    std::vector<size_t> functionShape(1, numStates);
	    LearnableWeightedSumOfFuncType unary(functionShape, weights, weightIds, features);
		GraphicalModelType::FunctionIdentifier fid = model.addFunction(unary);
		model.addFactor(fid, &openGMVariableId_, &openGMVariableId_+1);
	}
	else
	{
		// add unary factor to model
		std::vector<FeaturesAndIndicesType> featuresAndWeightsPerLabel;

		// if weights are not shared over states, we need to keep track how many weights have been used before
		size_t weightIdx = 0;

		for(size_t state = 0; state < numStates; ++state)
		{
			FeaturesAndIndicesType featureAndIndex;

			featureAndIndex.features = features_[state];
			for(size_t i = 0; i < features_[state].size(); ++i)
			{
				featureAndIndex.weightIds.push_back(weightIds[weightIdx++]);
			}

			featuresAndWeightsPerLabel.push_back(featureAndIndex);
		}

		LearnableUnaryFuncType unary(weights, featuresAndWeightsPerLabel);
		GraphicalModelType::FunctionIdentifier fid = model.addFunction(unary);
		model.addFactor(fid, &openGMVariableId_, &openGMVariableId_+1);
	}
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
				if((int)features_.at(i).size() != numWeights)
					throw std::runtime_error("Number of features must be equal for all states!");
		}
		else
		{
			numWeights = 0;
			for(size_t i = 0; i < features_.size(); ++i)
				numWeights += features_.at(i).size();
		}
	}

	return numWeights;
}

} // end namespace conservation
