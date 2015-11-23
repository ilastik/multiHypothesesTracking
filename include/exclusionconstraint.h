#ifndef EXCLUSION_CONSTRAINT_H
#define EXCLUSION_CONSTRAINT_H

#include <json/json.h>
#include "segmentationhypothesis.h"

namespace mht
{

/**
 * @brief An exclusion constraint models that of a set of segmentation hypotheses only one can be active at once.
 */
class ExclusionConstraint
{
public:
	ExclusionConstraint(){}

	/**
	 * @brief Manually create an exclusion constraint disallowing the two hypotheses to be active at the same time
	 */
	ExclusionConstraint(const std::vector<int>& ids);
	
	/**
	 * @brief read exclusion constraint from Json
	 * @details expects the json array to be a list of ints representing ids
	 * 
	 * @param entry json object for this hypothesis
	 */
	void readFromJson(const Json::Value& entry);

	/**
	 * @brief Add this constraint to the OpenGM model
	 * 
	 * @param model OpenGM model
	 * @param segmentationHypotheses the map of all segmentation hypotheses by id
	 */
	void addToOpenGMModel(GraphicalModelType& model, std::map<int, SegmentationHypothesis>& segmentationHypotheses);

	/**
	 * @brief Check that the given solution vector obeys this exclusion constraint
	 * 
	 * @param sol the opengm solution vector
	 * @param segmentationHypotheses the map or all segmentation hypotheses by id
	 */
	bool verifySolution(const Solution& sol, const std::map<int, SegmentationHypothesis>& segmentationHypotheses) const;

private:
	std::vector<int> ids_;
};

} // end namespace mht

#endif // EXCLUSION_CONSTRAINT_H
