#ifndef SEGMENTATION_HYPOTHESIS_H
#define SEGMENTATION_HYPOTHESIS_H

#include <iostream>

#include <json/json.h>
#include "helpers.h"
#include "variable.h"

// settings forward declaration
namespace helpers
{
	class Settings;
}

namespace mht
{

// forward declaration
class LinkingHypothesis;

/**
 * @brief A segmentation hypothesis is a detection of a target in a frame.
 * @details It can be read from Json, be added to an opengm model (with unary composed of several features that are learnable).
 */
class SegmentationHypothesis
{
public: // API
	SegmentationHypothesis();

	/**
	 * @brief Construct this hypothesis manually - mainly needed for testing
	 */
	SegmentationHypothesis(
		int id, 
		const helpers::StateFeatureVector& detectionFeatures, 
		const helpers::StateFeatureVector& divisionFeatures,
		const helpers::StateFeatureVector& appearanceFeatures = {},
		const helpers::StateFeatureVector& disappearanceFeatures = {});

	/**
	 * @brief read segmentation hypothesis from Json
	 * @details expects the json value to contain attributes "id"(int) and "features"(list of double),
	 * 			as well as "divisionFeatures", "appearanceFeatures" and "disappearanceFeatures", where
	 * 			the presence of the latter two toggles the presence of an appearance or disappearance node.
	 * 			Hypotheses which do not have these, are not allowed to appear/disappear!
	 * 
	 * @param entry json object for this hypothesis
	 * @return the found id of this hypothesis
	 */
	const int readFromJson(const Json::Value& entry);

	/**
	 * @return detection variable
	 */
	const Variable& getDetectionVariable() const { return detection_; }

	/**
	 * @return division variable
	 */
	const Variable& getDivisionVariable() const { return division_; }

	/**
	 * @return appearance variable
	 */
	const Variable& getAppearanceVariable() const { return appearance_; }

	/**
	 * @return disappearance variable
	 */
	const Variable& getDisappearanceVariable() const { return disappearance_; }


	/**
	 * @brief Add this hypothesis to the OpenGM model
	 * @details also adds the unary factor
	 * 
	 * @param model OpenGM model
	 * @param weights OpenGM weight object (if you are running learning this must be a reference to the weight object of the dataset)
	 * @param statesShareWeights whether there is one weight per feature for all states, or a separate weight for each feature and state
	 * @param detectionWeightIds indices of the weights that are meant to be used together with the detection features
	 * @param divisionWeightIds indices of the weights that are meant to be used together with the division features
	 * @param appearanceWeightIds indices of the weights that are meant to be used together with the division features
	 * @param disappearanceWeightIds indices of the weights that are meant to be used together with the division features
	 */
	void addToOpenGMModel(
		helpers::GraphicalModelType& model, 
		helpers::WeightsType& weights,
		std::shared_ptr<helpers::Settings> settings,
		const std::vector<size_t>& detectionWeightIds,
		const std::vector<size_t>& divisionWeightIds = {},
		const std::vector<size_t>& appearanceWeightIds = {},
		const std::vector<size_t>& disappearanceWeightIds = {});

	/**
	 * @brief Add an incoming link to this node as hypothesis. Will be considered in conservation constraints
	 * @details Links must be added before calling addToOpenGMModel for this segmentation hypothesis!
	 * 
	 * @param link the linking hypothesis
	 */
	void addIncomingLink(std::shared_ptr<LinkingHypothesis> link);

	/**
	 * @brief Add an outgoing link to this node as hypothesis. Will be considered in conservation constraints
	 * @details Links must be added before calling addToOpenGMModel for this segmentation hypothesis!
	 * @param link the linking hypothesis
	 */
	void addOutgoingLink(std::shared_ptr<LinkingHypothesis> link);

	/**
	 * @brief Save this node to an open ostream in the graphviz dot format
	 */
	void toDot(std::ostream& stream, const helpers::Solution* sol) const;

	/**
	 * @brief Create json value containing the state of this division, linked to this detection's id
	 */
	const Json::Value divisionToJson(size_t value) const;

	/**
	 * @brief Create json value containing the state of this detection
	 */
	const Json::Value detectionToJson(size_t value) const;

	/**
	 * @brief Check that the given solution vector obeys all flow conservation constraints + divisions
	 * 
	 * @param sol the opengm solution vector
	 */
	bool verifySolution(const helpers::Solution& sol) const;

	/**
	 * @return the number of incoming links of this detection which are active in the given solution
	 */
	size_t getNumActiveIncomingLinks(const helpers::Solution& sol) const;

	/**
	 * @return the number of outoing links of this detection which are active in the given solution
	 */
	size_t getNumActiveOutgoingLinks(const helpers::Solution& sol) const;

private:
	/**
	 * @brief Add incoming constraints to OpenGM
	 */
	void addIncomingConstraintToOpenGM(helpers::GraphicalModelType& model);

	/**
	 * @brief Add outgoing constraints to OpenGM
	 */
	void addOutgoingConstraintToOpenGM(helpers::GraphicalModelType& model);

	/**
	 * @brief Add division constraints to OpenGM
	 */
	void addDivisionConstraintToOpenGM(helpers::GraphicalModelType& model, bool requireSeparateChildren);

	/**
	 * @brief Add constraint that ensures that at most one of the two given opengm variables takes a state > 0
	 */
	void addExclusionConstraintToOpenGM(
		helpers::GraphicalModelType& model, 
		int openGmVarA, 
		int openGmVarB);

	/**
	 * Add a constraint between two variables and constraints with given bound and operator
	 */
	void addConstraintToOpenGM(
		helpers::GraphicalModelType& model, 
		int openGMVarA, 
		int openGMVarB, 
		size_t stateA, 
		size_t stateB, 
		size_t bound, 
		opengm::LinearConstraintTraits::LinearConstraintOperator::ValueType op);

	/**
	 * @brief Sort the linking hypotheses by their opengm variable ids
	 */
	void sortByOpenGMVariableId(std::vector< std::shared_ptr<LinkingHypothesis> >& links);

private:
	int id_;
	
	Variable detection_;
	Variable division_;
	Variable appearance_;
	Variable disappearance_;

	std::vector< std::shared_ptr<LinkingHypothesis> > incomingLinks_;
	std::vector< std::shared_ptr<LinkingHypothesis> > outgoingLinks_;
};

} // end namespace mht

#endif // SEGMENTATION_HYPOTHESIS_H
