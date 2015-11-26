#ifndef MULTIHYPOTHESIS_SEGMENTATION_HYPOTHESIS_H
#define MULTIHYPOTHESIS_SEGMENTATION_HYPOTHESIS_H

#include <iostream>

#include <json/json.h>
#include "../helpers.h"

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
public: // nested classes
	/**
	 * @brief A segmentation hypothesis comprises of several variables: detection, division, appearance and disappearance.
	 * 		  This class wraps their most important functionality
	 */
	class Variable{
	public:
		/**
		 * @brief Construct with the given feature vector
		 */
		Variable(const helpers::FeatureVector& features = {}):
			features_(features),
			opengmVariableId_(-1)
		{}

		/**
		 * @brief Add this variable with given unary features and corresponding weights to opengm
		 * 
		 * @param model OpenGM Model
		 * @param weights opengm dataset weight object
		 * @param weightIds ids into the weight vector that correspond to features
		 * @return the new opengm variable id
		 */
		void addToOpenGM(
			helpers::GraphicalModelType& model, 
			helpers::WeightsType& weights, 
			const std::vector<size_t>& weightIds);

		/**
		 * @return number of features
		 */
		const size_t getNumFeatures() const { return features_.size(); }

		/**
		 * @return the opengm variable id of this variable
		 */
		int getOpenGMVariableId() const { return opengmVariableId_; }

	private:
		helpers::FeatureVector features_;
		int opengmVariableId_;
	};

public: // API
	SegmentationHypothesis();

	/**
	 * @brief Construct this hypothesis manually - mainly needed for testing
	 */
	SegmentationHypothesis(
		int id, 
		const helpers::FeatureVector& detectionFeatures, 
		const helpers::FeatureVector& divisionFeatures,
		const helpers::FeatureVector& appearanceFeatures = {},
		const helpers::FeatureVector& disappearanceFeatures = {});

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
	 * @param detectionWeightIds indices of the weights that are meant to be used together with the detection features (size must match 2*numFeatures)
	 * @param divisionWeightIds indices of the weights that are meant to be used together with the division features (size must match 2*numFeatures)
	 */
	void addToOpenGMModel(
		helpers::GraphicalModelType& model, 
		helpers::WeightsType& weights,
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
	void addDivisionConstraintToOpenGM(helpers::GraphicalModelType& model);

	/**
	 * @brief Add constraint that ensures that division and disappearance are not active at once
	 */
	void addDivisionDisappearanceConstraintToOpenGM(helpers::GraphicalModelType& model);

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

#endif // MULTIHYPOTHESIS_SEGMENTATION_HYPOTHESIS_H
