#include "exclusionconstraint.h"
#include <algorithm>

using namespace helpers;

namespace mht
{

ExclusionConstraint::ExclusionConstraint(const std::vector<int>& ids):
	ids_(ids)
{}

void ExclusionConstraint::readFromJson(const Json::Value& entry)
{
	if(!entry.isArray())
		throw std::runtime_error("Cannot extract Constraint from non-array JSON entry");

	ids_.clear();
	for(int i = 0; i < (int)entry.size(); i++)
	{
		ids_.push_back(entry[i].asInt());
	}

	// sort because OpenGM likes to have variable ids in order
	std::sort(ids_.begin(), ids_.end());
}

void ExclusionConstraint::addToOpenGMModel(GraphicalModelType& model, std::map<int, SegmentationHypothesis>& segmentationHypotheses)
{
	LinearConstraintFunctionType::LinearConstraintType exclusionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;
    
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

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &exclusionConstraint, &exclusionConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

bool ExclusionConstraint::verifySolution(const Solution& sol, const std::map<int, SegmentationHypothesis>& segmentationHypotheses) const
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
