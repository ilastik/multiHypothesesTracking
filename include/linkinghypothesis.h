#ifndef LINKING_HYPOTHESIS_H
#define LINKING_HYPOTHESIS_H

#include <iostream>
#include <memory>

#include <json/json.h>
#include "helpers.h"
#include "segmentationhypothesis.h"
#include "variable.h"

namespace mht
{

/**
 * @brief A linking hypothesis is a possible connection between segmentation hypotheses (in consecutive? frames)
 * @details It can be read from Json, be added to an opengm model 
 * (with unary composed of several features that are learnable).
 */
class LinkingHypothesis : public std::enable_shared_from_this<LinkingHypothesis>
{
public:
	LinkingHypothesis();

	/**
	 * @brief Construct this hypothesis manually - mainly needed for testing
	 */
	LinkingHypothesis(helpers::IdLabelType srcId, helpers::IdLabelType destId, const helpers::StateFeatureVector& features);

	/**
	 * @brief read linking hypothesis from Json
	 * @details expects the json value to contain attributes "src"(helpers::IdLabelType), "dest"(helpers::IdLabelType), and "features"(list of double)
	 * 
	 * @param entry json object for this hypothesis
	 * @returns a pair of (src, dest) segmentation hypotheses ids
	 */
	const std::pair<helpers::IdLabelType, helpers::IdLabelType> readFromJson(const Json::Value& entry);

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
	 * @brief notify the two connected segmentation hypotheses about their new incoming/outgoing link
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
	helpers::IdLabelType srcId_;
	helpers::IdLabelType destId_;
	
	Variable variable_;
};

} // end namespace mht

#endif // LINKING_HYPOTHESIS_H
