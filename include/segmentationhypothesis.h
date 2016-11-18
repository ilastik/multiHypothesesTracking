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
class DivisionHypothesis;

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
		helpers::IdLabelType id, 
		const helpers::StateFeatureVector& detectionFeatures, 
		const helpers::StateFeatureVector& divisionFeatures = {},
		const helpers::StateFeatureVector& appearanceFeatures = {},
		const helpers::StateFeatureVector& disappearanceFeatures = {});

	const helpers::IdLabelType getId() const { return id_; }

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
	 * @brief Add an incoming division, which will be handled the same as incoming links
	 * @details must be added before calling addToOpenGMModel
	 * 
	 * @param division the division hypothesis
	 */
	void addIncomingDivision(std::shared_ptr<DivisionHypothesis> division);

	/**
	 * @brief Add an outgoing division - of which always only one may be active
	 * @details must be added before calling addToOpenGMModel
	 * 
	 * @param division the division hypothesis
	 */
	void addOutgoingDivision(std::shared_ptr<DivisionHypothesis> division);

	/**
	 * @brief Save this node to an open ostream in the graphviz dot format
	 */
	void toDot(std::ostream& stream, const helpers::Solution* sol) const;

	/**
	 * @brief Check that the given solution vector obeys all flow conservation constraints + divisions
	 * 
	 * @param sol the opengm solution vector
	 */
	bool verifySolution(const helpers::Solution& sol, const std::shared_ptr<helpers::Settings>& settings) const;

	/**
	 * @return the number of incoming links and external divisions of this detection which are active in the given solution
	 * 
	 */
	size_t getNumActiveIncomingLinks(const helpers::Solution& sol) const;

	/**
	 * @return the number of outoing links and external divisions of this detection which are active in the given solution
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
	 * @brief Add constraints of external division nodes (division hypotheses) to OpenGM
	 */
	void addExternalDivisionConstraintaToOpenGM(helpers::GraphicalModelType& model);

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
	template<class T>
	void sortByOpenGMVariableId(std::vector< std::shared_ptr<T> >& links);

private:
	helpers::IdLabelType id_;
	
	Variable detection_;
	Variable division_;
	Variable appearance_;
	Variable disappearance_;

	std::vector< std::shared_ptr<LinkingHypothesis> > incomingLinks_;
	std::vector< std::shared_ptr<LinkingHypothesis> > outgoingLinks_;
	std::vector< std::shared_ptr<DivisionHypothesis> > incomingDivisions_;
	std::vector< std::shared_ptr<DivisionHypothesis> > outgoingDivisions_;
};

template<class T>
void SegmentationHypothesis::sortByOpenGMVariableId(std::vector< std::shared_ptr<T> >& links)
{
	std::sort(links.begin(), links.end(), [](const std::shared_ptr<T>& a, const std::shared_ptr<T>& b){
		return b->getVariable().getOpenGMVariableId() > a->getVariable().getOpenGMVariableId();
	});
}

} // end namespace mht

#endif // SEGMENTATION_HYPOTHESIS_H
