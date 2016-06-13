#ifndef EXCLUSION_CONSTRAINT_H
#define EXCLUSION_CONSTRAINT_H

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
	ExclusionConstraint(const std::vector<helpers::IdLabelType>& ids);
	
	/**
	 * @brief Add this constraint to the OpenGM model
	 * 
	 * @param model OpenGM model
	 * @param segmentationHypotheses the map of all segmentation hypotheses by id
	 */
	void addToOpenGMModel(helpers::GraphicalModelType& model, std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses);

	/**
	 * @brief Check that the given solution vector obeys this exclusion constraint
	 * 
	 * @param sol the opengm solution vector
	 * @param segmentationHypotheses the map or all segmentation hypotheses by id
	 */
	bool verifySolution(const helpers::Solution& sol, const std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses) const;

	/**
	 * @brief Save this constraint as red edges in a graphviz dot graph
	 */
	void toDot(std::ostream& stream) const;

private:
	std::vector<helpers::IdLabelType> ids_;
};

} // end namespace mht

#endif // EXCLUSION_CONSTRAINT_H
