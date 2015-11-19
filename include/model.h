#ifndef MODEL_H
#define MODEL_H

#include <memory>
#include <vector>
#include <map>

#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include "exclusionconstraint.h"
#include "helpers.h"

namespace mht
{

/**
 * @brief The model holds all detections, their links, and the mutual exclusion constraints
 * @detail WARNING: at the moment you can only run either learn or infer once on the model. 
 * 		   Build a new one if you need it multiple times
 */
class Model
{
public:
	/**
	 * @brief Enumerate the strings for attributes used in the Json file
	 */
	enum class JsonTypes {Segmentations, Links, Exclusions};
	std::map<JsonTypes, std::string> JsonTypeNames = {
		{JsonTypes::Segmentations, "segmentation-hypotheses"}, 
		{JsonTypes::Links, "linking-hypotheses"}, 
		{JsonTypes::Exclusions, "exclusions"}
	};

public:	
	/**
	 * @brief Read a model from a json file
	 * @param filename
	 */
	void readFromJson(const std::string& filename);

	/**
	 * @return the number of weights which is estimated by checking how many features are given for detections, links and divisions
	 */
	size_t computeNumWeights() const;
	
	/**
	 * @brief Find the minimal-energy configuration using an ILP
	 * @param weights a vector of weights to use
	 * @return the vector of per-variable labels, can be used with the detection/linking hypotheses to query their state
	 */
	Solution infer(const std::vector<ValueType>& weights);

	/**
	 * @brief Run learning using a given ground truth file
	 * @details Loads the ground truth from another JSON file
	 * 
	 * @param gt_filename JSON file containing a mapping of "src"(int), "dest"(int) -> "value"(bool)
	 * @return the vector of learned weights
	 */
	std::vector<ValueType> learn(const std::string gt_filename);

private:
	/**
	 * @brief Initialize the OpenGM model by adding variables, factors and constraints.
	 * @detail This is called by learn() or infer()
	 * 
	 * @param weights a reference to the weights object that will be used in all 
	 */
	void initializeOpenGMModel(WeightsType& weights);

private:
	std::map<int, SegmentationHypothesis> segmentationHypotheses_;
	// linking hypotheses are stored as shared pointer so it is easier to pass them around
	std::vector< std::shared_ptr<LinkingHypothesis> > linkingHypotheses_;
	std::vector<ExclusionConstraint> exclusionConstraints_;

	// OpenGM stuff
	GraphicalModelType model_;
};

} // end namespace mht

#endif // MODEL_H
