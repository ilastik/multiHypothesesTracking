#ifndef VARIABLE_H
#define VARIABLE_H 

#include "helpers.h"

namespace mht
{

/**
 * @brief Segmentation hypotheses and linking hypotheses comprise of several opengm variables: detection, division, appearance, disappearance and links.
 * 		  This class wraps their most important functionality
 */
class Variable{
public:
	/**
	 * @brief Construct with the given feature vector
	 */
	Variable(const helpers::StateFeatureVector& features = {}):
		features_(features),
		openGMVariableId_(-1)
	{}

	/**
	 * @brief Add this variable with given unary features and corresponding weights to opengm
	 * 
	 * @param model OpenGM Model
	 * @param statesShareWeights if this is true it means that the features of each state are multiplied by the same weight
	 * @param weights opengm dataset weight object
	 * @param weightIds ids into the weight vector that correspond to features
	 * @return the new opengm variable id
	 */
	void addToOpenGM(
		helpers::GraphicalModelType& model, 
		bool statesShareWeights,
		helpers::WeightsType& weights, 
		const std::vector<size_t>& weightIds);

	/**
	 * @brief Get the number of weights needed for this variable
	 * 
	 * @param statesShareWeights if this is true it means that the features of each state are multiplied by the same weight
	 * @return needed number of weights for this variable
	 */
	const int getNumWeights(bool statesShareWeights) const;

	/**
	 * @param state the state of which we want to know the number of features
	 * @return number of features 
	 */
	const size_t getNumFeatures(size_t state) const { return features_.at(state).size(); }

	/**
	 * @return number of features summed over all states 
	 */
	const size_t getNumFeatures() const
	{
		size_t sum = 0;
		for(size_t s = 0; s < getNumStates(); s++)
			sum += getNumFeatures(s);
		return sum;
	}

	/**
	 * @return number of states this variable can take (defined by the number of feature lists in JSON)
	 */
	const size_t getNumStates() const { return features_.size(); }

	/**
	 * @return the opengm variable id of this variable
	 */
	int getOpenGMVariableId() const { return openGMVariableId_; }

private:
	helpers::StateFeatureVector features_;
	int openGMVariableId_;
};

}

#endif // VARIABLE_H
