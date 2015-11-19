#ifndef MODEL_H
#define MODEL_H

#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include "helpers.h"

namespace mht
{

/**
 * @brief The model holds all detections, their links, and the mutual exclusion constraints
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
	 * @brief Initialize the OpenGM model by adding variables, factors and constraints
	 * 
	 * @param weights a reference to the weights object that will be used in all 
	 */
	void initializeOpenGMModel(WeightsType& weights);
	
	/**
	 * @brief Find the minimal-energy configuration using an ILP
	 * @return the vector of per-variable labels, can be used with the detection/linking hypotheses to query their state
	 */
	Solution infer();

	void learn();

private:
	std::map<int, SegmentationHypothesis> segmentationHypotheses_;
	std::vector<LinkingHypothesis> linkingHypotheses_;
	// std::vector<ExclusionConstraints> exclusionConstraints_;

	// OpenGM stuff
	GraphicalModelType model_;
};

} // end namespace mht

#endif // MODEL_H
