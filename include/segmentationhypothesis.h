#ifndef SEGMENTATION_HYPOTHESIS_H
#define SEGMENTATION_HYPOTHESIS_H

#include <iostream>

#include <json/json.h>
#include "helpers.h"

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
public:
	SegmentationHypothesis();

	/**
	 * @brief Construct this hypothesis manually - mainly needed for testing
	 */
	SegmentationHypothesis(int id, const FeatureVector& features, const FeatureVector& divisionFeatures);

	/**
	 * @brief read segmentation hypothesis from Json
	 * @details expects the json value to contain attributes "id"(int) and "features"(list of double)
	 * 
	 * @param entry json object for this hypothesis
	 * @return the found id of this hypothesis
	 */
	const int readFromJson(const Json::Value& entry);

	/**
	 * @return number of features, this should be equal for all hypotheses!
	 */
	const size_t getNumFeatures() const { return features_.size(); }

	/**
	 * @return number of division features, this should be equal for all hypotheses!
	 */
	const size_t getNumDivisionFeatures() const { return divisionFeatures_.size(); }

	/**
	 * @brief Add this hypothesis to the OpenGM model
	 * @details also adds the unary factor
	 * 
	 * @param model OpenGM model
	 * @param weights OpenGM weight object (if you are running learning this must be a reference to the weight object of the dataset)
	 * @param detectionWeightIds indices of the weights that are meant to be used together with the detection features (size must match 2*numFeatures)
	 * @param divisionWeightIds indices of the weights that are meant to be used together with the division features (size must match 2*numFeatures)
	 */
	void addToOpenGMModel(
		GraphicalModelType& model, 
		WeightsType& weights,
		const std::vector<size_t>& detectionWeightIds,
		const std::vector<size_t>& divisionWeightIds);

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
	void toDot(std::ostream& stream) const;

	/**
	 * @return the opengm variable id of the segmentation variable
	 */
	int getOpenGMVariableId() const { return opengmVariableId_; }

	/**
	 * @return the opengm variable id of the division variable
	 */
	int getOpenGMDivisionVariableId() const { return opengmDivisionVariableId_; }

	/**
	 * @brief Check that the given solution vector obeys all flow conservation constraints + divisions
	 * 
	 * @param sol the opengm solution vector
	 */
	bool verifySolution(const Solution& sol);

private:
	size_t addVariableToOpenGM(
		GraphicalModelType& model, 
		WeightsType& weights, 
		FeatureVector& features,
		const std::vector<size_t>& weightIds);

	void addIncomingConstraintToOpenGM(GraphicalModelType& model);
	void addOutgoingConstraintToOpenGM(GraphicalModelType& model);
	void addDivisionConstraintToOpenGM(GraphicalModelType& model);

private:
	int id_;
	FeatureVector features_;
	FeatureVector divisionFeatures_;
	int opengmVariableId_;
	int opengmDivisionVariableId_;
	std::vector< std::shared_ptr<LinkingHypothesis> > incomingLinks_;
	std::vector< std::shared_ptr<LinkingHypothesis> > outgoingLinks_;
};

} // end namespace mht

#endif // SEGMENTATION_HYPOTHESIS_H
