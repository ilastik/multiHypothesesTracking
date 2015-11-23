#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include <stdexcept>

namespace mht
{

SegmentationHypothesis::SegmentationHypothesis():
	id_(-1),
	opengmVariableId_(-1),
	opengmDivisionVariableId_(-1)
{}

SegmentationHypothesis::SegmentationHypothesis(int id, const FeatureVector& features, const FeatureVector& divisionFeatures):
	id_(id),
	features_(features),
	opengmVariableId_(-1),
	opengmDivisionVariableId_(-1)
{}

const int SegmentationHypothesis::readFromJson(const Json::Value& entry)
{
	if(!entry.isObject())
		throw std::runtime_error("Cannot extract SegmentationHypothesis from non-object JSON entry");
	if(!entry.isMember(JsonTypeNames[JsonTypes::Id]) || !entry[JsonTypeNames[JsonTypes::Id]].isInt() || !entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
		throw std::runtime_error("JSON entry for SegmentationHytpohesis is invalid");

	id_ = entry[JsonTypeNames[JsonTypes::Id]].asInt();

	features_.clear();
	const Json::Value features = entry[JsonTypeNames[JsonTypes::Features]];
	for(int i = 0; i < (int)features.size(); i++)
	{
		features_.push_back(features[i].asDouble());
	}

	divisionFeatures_.clear();
	const Json::Value divisionFeatures = entry[JsonTypeNames[JsonTypes::DivisionFeatures]];
	for(int i = 0; i < (int)divisionFeatures.size(); i++)
	{
		divisionFeatures_.push_back(divisionFeatures[i].asDouble());
	}

	// std::cout << "Found segmentation hypothesis with id " << id_ << std::endl;

	return id_;
}

void SegmentationHypothesis::toDot(std::ostream& stream, const Solution* sol) const
{
	stream << "\t" << id_ << " [ label=\"" << id_ << ", div=";

	if(sol != nullptr && opengmDivisionVariableId_ > 0 && sol->at(opengmDivisionVariableId_) > 0)
		stream << "yes";
	else
		stream << "no";

	stream << "\" ";

	// highlight active nodes in blue
	if(sol != nullptr && opengmVariableId_ > 0 && sol->at(opengmVariableId_) > 0)
		stream << "color=\"blue\" fontcolor=\"blue\" ";

	stream <<  "]; \n" << std::flush;
}

size_t SegmentationHypothesis::addVariableToOpenGM(
	GraphicalModelType& model, 
	WeightsType& weights, 
	FeatureVector& features,
	const std::vector<size_t>& weightIds)
{
	// Add variable to model. All Variables are binary!
	model.addVariable(2);
	size_t variableId = model.numberOfVariables() - 1;

	// add unary factor to model
	std::vector<FeaturesAndIndicesType> featuresAndWeightsPerLabel;
	size_t numFeatures = features.size();
	assert(weightIds.size() == numFeatures * 2);

	for(size_t l = 0; l < 2; l++)
	{
		FeaturesAndIndicesType featureAndIndex;

		featureAndIndex.features = features;
		for(size_t i = 0; i < numFeatures; ++i)
			featureAndIndex.weightIds.push_back(weightIds[l * numFeatures + i]);

		featuresAndWeightsPerLabel.push_back(featureAndIndex);
	}

	LearnableUnaryFuncType unary(weights, featuresAndWeightsPerLabel);
	GraphicalModelType::FunctionIdentifier fid = model.addFunction(unary);
	model.addFactor(fid, &variableId, &variableId+1);
	return variableId;
}

void SegmentationHypothesis::addIncomingConstraintToOpenGM(GraphicalModelType& model)
{
	if(incomingLinks_.size() == 0)
		return;

	// add constraint for sum of incoming = this label
	LinearConstraintFunctionType::LinearConstraintType incomingConsistencyConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;
    
    // TODO: make sure that links are sorted by their opengm variable ids!
    // add all incoming transition variables with positive coefficient
    for(size_t i = 0; i < incomingLinks_.size(); ++i)
    {
    	// indicator variable references the i+1'th argument of the constraint function, and its state 1
        const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(i, 1);
        incomingConsistencyConstraint.add(indicatorVariable, 1.0);
        
        // directly save which variables we are interested in
        factorVariables.push_back(incomingLinks_[i]->getOpenGMVariableId());
        constraintShape.push_back(2);
    }

    // add this variable's state with negative coefficient
    const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(incomingLinks_.size(), LabelType(1));
    incomingConsistencyConstraint.add(indicatorVariable, -1.0);

    factorVariables.push_back(opengmVariableId_);
    constraintShape.push_back(2);

    incomingConsistencyConstraint.setBound( 0 );
    incomingConsistencyConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::Equal);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &incomingConsistencyConstraint, &incomingConsistencyConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addOutgoingConstraintToOpenGM(GraphicalModelType& model)
{
	if(outgoingLinks_.size() == 0)
		return;

	// add constraint for sum of ougoing = this label + division
	LinearConstraintFunctionType::LinearConstraintType outgoingConsistencyConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;
    
    // TODO: make sure that links are sorted by their opengm variable ids!
    // add all incoming transition variables with positive coefficient
    for(size_t i = 0; i < outgoingLinks_.size(); ++i)
    {
    	// indicator variable references the i+2'nd argument of the constraint function, and its state 1
        const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(i, 1);
        outgoingConsistencyConstraint.add(indicatorVariable, 1.0);
        
        // directly save which variables we are interested in
        factorVariables.push_back(outgoingLinks_[i]->getOpenGMVariableId());
        constraintShape.push_back(2);
    }

    // add this variable's state with negative coefficient
    const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(outgoingLinks_.size(), LabelType(1));
    outgoingConsistencyConstraint.add(indicatorVariable, -1.0);

    factorVariables.push_back(opengmVariableId_);
    constraintShape.push_back(2);

    const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType divisionIndicatorVariable(outgoingLinks_.size()+1, LabelType(1));
    outgoingConsistencyConstraint.add(divisionIndicatorVariable, -1.0);

    factorVariables.push_back(opengmDivisionVariableId_);
    constraintShape.push_back(2);

    outgoingConsistencyConstraint.setBound( 0 );
    outgoingConsistencyConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::Equal);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &outgoingConsistencyConstraint, &outgoingConsistencyConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addDivisionConstraintToOpenGM(GraphicalModelType& model)
{
	// add constraint for sum of ougoing = this label + division
	LinearConstraintFunctionType::LinearConstraintType divisionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;

	// add this variable's state with negative coefficient
    const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(0, LabelType(1));
    divisionConstraint.add(indicatorVariable, -1.0);

    factorVariables.push_back(opengmVariableId_);
    constraintShape.push_back(2);

    const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType divisionIndicatorVariable(1, LabelType(1));
    divisionConstraint.add(divisionIndicatorVariable, 1.0);

    factorVariables.push_back(opengmDivisionVariableId_);
    constraintShape.push_back(2);

    divisionConstraint.setBound( 0 );
    divisionConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::LessEqual);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &divisionConstraint, &divisionConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addToOpenGMModel(
	GraphicalModelType& model, 
	WeightsType& weights, 
	const std::vector<size_t>& detectionWeightIds,
	const std::vector<size_t>& divisionWeightIds)
{
	// std::cout << "Adding segmentation hypothesis " << id_ << " to opengm" << std::endl;
	opengmVariableId_ = addVariableToOpenGM(model, weights, features_, detectionWeightIds);
	opengmDivisionVariableId_ = addVariableToOpenGM(model, weights, divisionFeatures_, divisionWeightIds);
	addIncomingConstraintToOpenGM(model);
	addOutgoingConstraintToOpenGM(model);
	addDivisionConstraintToOpenGM(model);
}

void SegmentationHypothesis::addIncomingLink(std::shared_ptr<LinkingHypothesis> link)
{
	if(opengmVariableId_ >= 0)
		throw std::runtime_error("Links must be added before the segmentation hypothesis is added to the OpenGM model");
	if(link)
		incomingLinks_.push_back(link);
}

void SegmentationHypothesis::addOutgoingLink(std::shared_ptr<LinkingHypothesis> link)
{
	if(opengmVariableId_ >= 0)
		throw std::runtime_error("Links must be added before the segmentation hypothesis is added to the OpenGM model");
	if(link)
		outgoingLinks_.push_back(link);
}

bool SegmentationHypothesis::verifySolution(const Solution& sol) const
{
	size_t ownValue = sol[opengmVariableId_];
	size_t divisionValue = sol[opengmDivisionVariableId_];
	
	//--------------------------------
	// check incoming
	size_t sumIncoming = 0;
	for(auto link : incomingLinks_)
	{
		sumIncoming += sol[link->getOpenGMVariableId()];
	}
	// TODO: how are we dealing with appearance / disappearance?
	if(incomingLinks_.size() > 0 && sumIncoming != ownValue)
	{
		std::cout << "At node " << id_ << ": incoming=" << sumIncoming << " is NOT EQUAL to " << ownValue << std::endl;
		return false;
	}

	//--------------------------------
	// check outgoing
	size_t sumOutgoing = 0;
	for(auto link : outgoingLinks_)
	{
		sumOutgoing += sol[link->getOpenGMVariableId()];
	}

	// TODO: how are we dealing with appearance / disappearance?
	if(outgoingLinks_.size() > 0 && sumOutgoing != ownValue + divisionValue)
	{
		std::cout << "At node " << id_ << ": outgoing=" << sumOutgoing << " is NOT EQUAL to " << ownValue << " + " << divisionValue << " (own+div)" << std::endl;
		return false;
	}

	//--------------------------------
	// check divisions
	if(divisionValue > ownValue)
		std::cout << "At node " << id_ << ": division > value: " << divisionValue << " > " << ownValue << std::endl;

	return divisionValue <= ownValue;
}

} // end namespace mht
