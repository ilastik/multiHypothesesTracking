#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>
#include <map>

#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include "exclusionconstraint.h"
#include "divisionhypothesis.h"
#include "helpers.h"
#include "settings.h"

namespace mht
{

/**
 * @brief The model holds all detections and their links, as well as exclusion constraints between detections
 * @detail WARNING: at the moment you can only run either learn or infer once on the model. 
 * 		   Build a new one if you need it multiple times
 */
class Model
{
public:	
	/**
	 * @return the number of weights which is estimated by checking how many features are given for detections, links and divisions
	 */
	size_t computeNumWeights();
	
	/**
	 * @brief Find the minimal-energy configuration using an ILP
	 * @param weights a vector of weights to use
	 * @return the vector of per-variable labels, can be used with the detection/linking hypotheses to query their state
	 */
	helpers::Solution infer(const std::vector<helpers::ValueType>& weights);

	/**
	 * @brief Run learning using a given ground truth file and initial weights
	 * @details Loads the ground truth using getGroundTruth() and learns the best weights using Structured Bundled Risk Minimization
	 * @param weights 
	 * @return the vector of learned weights
	 */
	std::vector<helpers::ValueType> learn(const std::vector<helpers::ValueType>& weights);

	/**
	 * @brief Run learning using a given ground truth file
	 * @details Loads the ground truth using getGroundTruth() and learns the best weights using Structured Bundled Risk Minimization
	 * 
	 * @return the vector of learned weights
	 */
	std::vector<helpers::ValueType> learn();

	/**
	 * @brief check that the solution does not violate any constraints
	 * @detail WARNING: may only be used after calling initializeOpenGMModel(), learn() or infer() because it needs an initialized opengm model!
	 * 
	 * @param sol solution vector
	 * @return boolean value describing whether this solution is valid
	 */
	bool verifySolution(const helpers::Solution& sol) const;

	/**
	 * @brief Return the energy of the given solution vector
	 * @detail WARNING: may only be used after calling initializeOpenGMModel(), learn() or infer() because it needs an initialized opengm model!
	 * 
	 * @param sol solution vector
	 * @return energy of the system in this solution
	 */
	double evaluateSolution(const helpers::Solution& sol) const;

	/**
	 * @brief Create a graphviz dot output of the full graph, showing used nodes/links in blue and exclusion constraints in red
	 * 
	 * @param filename output filename
	 * @param sol pointer to solution vector, if nullptr it will be ignored
	 */
	void toDot(const std::string& filename, const helpers::Solution* sol = nullptr) const;

	/**
	 * @brief Initialize the OpenGM model by adding variables, factors and constraints.
	 * @detail This is called by learn() or infer()
	 * 
	 * @param weights a reference to the weights object that will be used in all 
	 */
	void initializeOpenGMModel(helpers::WeightsType& weights);

	/**
	 * @return a vector of strings describing each entry in the weight vector
	 */
	std::vector<std::string> getWeightDescriptions();

	/**
	 * @brief get the ground truth for learning, needs to be implemented by subclasses
	 * @return the solution vector that fits the initialized OpenGM model
	 */
	virtual helpers::Solution getGroundTruth() = 0;

protected:
	/**
	 * @brief deduce states of appearance and disappearance variables and update the solution vector
	 */
	void deduceAppearanceDisappearanceStates(helpers::Solution& solution);

protected:
	// segmentation hypotheses
	std::map<helpers::IdLabelType, SegmentationHypothesis> segmentationHypotheses_;
	// linking hypotheses are stored as shared pointer so it is easier to pass them around
	std::map<std::pair<helpers::IdLabelType, helpers::IdLabelType>, std::shared_ptr<LinkingHypothesis> > linkingHypotheses_;
	// division hypotheses as shared pointers
	std::map<DivisionHypothesis::IdType, std::shared_ptr<DivisionHypothesis> > divisionHypotheses_;
	// exclusion constraints
	std::vector<ExclusionConstraint> exclusionConstraints_;

	// OpenGM stuff
	helpers::GraphicalModelType model_;

	// model settings
	std::shared_ptr<helpers::Settings> settings_;

	// numbers of weights
	size_t numDetWeights_ = 0;
	size_t numDivWeights_ = 0;
	size_t numAppWeights_ = 0;
	size_t numDisWeights_ = 0;
	size_t numExternalDivWeights_ = 0;
	size_t numLinkWeights_ = 0;
};

} // end namespace mht

#endif // MODEL_H
