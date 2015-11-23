#include "exclusionconstraint.h"
#include <algorithm>

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
	// add constraint for sum of ougoing = this label + division
	LinearConstraintFunctionType::LinearConstraintType exclusionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;
    
    // sum of all participating variables must not exceed 1
    for(size_t i = 0; i < ids_.size(); ++i)
    {
    	// indicator variable references the i'th argument of the constraint function, and its state 1
        const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(i, 1);
        exclusionConstraint.add(indicatorVariable, 1.0);
        
        // directly save which variables we are interested in
        factorVariables.push_back(segmentationHypotheses[ids_[i]].getOpenGMVariableId());
        constraintShape.push_back(2);
    }

    exclusionConstraint.setBound( 1 );
    exclusionConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::LessEqual);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &exclusionConstraint, &exclusionConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

bool ExclusionConstraint::verifySolution(const Solution& sol, std::map<int, SegmentationHypothesis>& segmentationHypotheses)
{
	size_t sum = 0;

	for(size_t i = 0; i < ids_.size(); ++i)
    {
        sum += sol[segmentationHypotheses[ids_[i]].getOpenGMVariableId()];
    }

    return sum < 2;
}

} // end namespace mht
