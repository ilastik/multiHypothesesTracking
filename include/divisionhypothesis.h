#ifndef DIVISION_HYPOTHESIS_H
#define DIVISION_HYPOTHESIS_H

#include <iostream>
#include <memory>
#include <tuple>

#include <json/json.h>
#include "helpers.h"
#include "segmentationhypothesis.h"
#include "variable.h"

namespace mht
{

/**
 * @brief A division hypothesis is a possible connection between a parent segmentation hypothesis and two children hypotheses (in consecutive frames!)
 * @details It can be read from Json, be added to an opengm model 
 * (with unary composed of several features that are learnable).
 */
class DivisionHypothesis : public std::enable_shared_from_this<DivisionHypothesis>
{
public:
	DivisionHypothesis();

	/**
	 * @brief Construct this hypothesis manually - mainly needed for testing
	 */
	DivisionHypothesis(helpers::IdLabelType parent, const std::vector<helpers::IdLabelType>& children, const helpers::StateFeatureVector& features);

	/**
	 * @brief read linking hypothesis from Json
	 * @details expects the json value to contain attributes "src"(helpers::IdLabelType), "dest"(helpers::IdLabelType), and "features"(list of double)
	 * 
	 * @param entry json object for this hypothesis
	 * @returns a tuple of (parent, child1, child2) segmentation hypotheses ids
	 */
	const std::tuple<helpers::IdLabelType, helpers::IdLabelType, helpers::IdLabelType> readFromJson(const Json::Value& entry);

	/**
	 * @brief Create a json string describing this link with its value (for result saving)
	 * 
	 * @param state the state that this link has (will be saved as "value" in JSON)
	 * @return the Json value to put in an array into the result file
	 */
	const Json::Value toJson(size_t state) const;

	/**
	 * @brief Add this hypothesis to the OpenGM model
	 * @details also adds the unary factor
	 * 
	 * @param model OpenGM model
	 * @param weights OpenGM weight object (if you are running learning this must be a reference to the weight object of the dataset)
	 * @param statesShareWeights whether there is one weight per feature for all states, or a separate weight for each feature and state
	 * @param weightIds indices of the weights that are meant to be used together with the features (size must match 2*numFeatures)
	 */
	void addToOpenGMModel(
		helpers::GraphicalModelType& model, 
		helpers::WeightsType& weights, 
		bool statesShareWeights,
		const std::vector<size_t>& weightIds);

	/**
	 * @brief notify the three connected segmentation hypotheses about their new incoming/outgoing division link
	 * 
	 * @param segmentationHypotheses the map of all segmentation hypotheses
	 */
	void registerWithSegmentations(std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses);

	/**
	 * @brief Save this node to an open ostream in the graphviz dot format
	 */
	void toDot(std::ostream& stream, const helpers::Solution* sol) const;

	/**
	 * @return opengm variable
	 */
	const Variable& getVariable() const { return variable_; }

private:
	helpers::IdLabelType parentId_;
	std::vector<helpers::IdLabelType> childrenIds_;
	
	Variable variable_;
};

} // end namespace mht

#endif // DIVISION_HYPOTHESIS_H
