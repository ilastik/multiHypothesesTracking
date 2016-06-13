#include "exclusionconstraint.h"
#include <algorithm>

using namespace helpers;

namespace mht
{

ExclusionConstraint::ExclusionConstraint(const std::vector<helpers::IdLabelType>& ids):
	ids_(ids)
{}

void ExclusionConstraint::addToOpenGMModel(GraphicalModelType& model, std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses)
{
	LinearConstraintFunctionType::LinearConstraintType exclusionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;
    
	// sort because OpenGM likes to have variable ids in order
	std::sort(ids_.begin(), ids_.end(), [&](const helpers::IdLabelType& a, const helpers::IdLabelType& b){
		return segmentationHypotheses[b].getDetectionVariable().getOpenGMVariableId() > segmentationHypotheses[a].getDetectionVariable().getOpenGMVariableId();
	});

    // sum of all participating indicator variables for states > 0 must not exceed 1
    for(size_t i = 0; i < ids_.size(); ++i)
    {
    	// indicator variable references the i'th argument of the constraint function, and its states > 0
    	for(size_t state = 1; state < model.numberOfLabels(segmentationHypotheses[ids_[i]].getDetectionVariable().getOpenGMVariableId()); ++state)
    	{
	    	addOpenGMVariableToConstraint(exclusionConstraint, segmentationHypotheses[ids_[i]].getDetectionVariable().getOpenGMVariableId(),
				state, 1.0, constraintShape, factorVariables, model);
	    }
    }

    exclusionConstraint.setBound( 1 );
    exclusionConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::LessEqual);

    addConstraintToOpenGMModel(exclusionConstraint, constraintShape, factorVariables, model);
}

bool ExclusionConstraint::verifySolution(const Solution& sol, const std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses) const
{
	size_t sum = 0;

	for(size_t i = 0; i < ids_.size(); ++i)
    {
        sum += (sol[segmentationHypotheses.at(ids_[i]).getDetectionVariable().getOpenGMVariableId()] > 0 ? 1 : 0);
    }

    if(sum > 1)
    	std::cout << "Violating exclusion constraint between ids: " << ids_ << std::endl;

    return sum < 2;
}

void ExclusionConstraint::toDot(std::ostream& stream) const
{
	for(size_t i = 0; i < ids_.size(); ++i)
	{
		for(size_t j = i + 1; j < ids_.size(); ++j)
		{
			stream << "\t" << ids_[i] << " -> " << ids_[j] << "[ color=\"red\" fontcolor=\"red\" ]" << "; \n" << std::flush;	
		}
	}
}

} // end namespace mht
