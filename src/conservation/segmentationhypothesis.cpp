#include "conservation/segmentationhypothesis.h"
#include "conservation/linkinghypothesis.h"

#include <stdexcept>

using namespace helpers;

namespace constracking
{

SegmentationHypothesis::SegmentationHypothesis():
	id_(-1)
{}

SegmentationHypothesis::SegmentationHypothesis(
	int id, 
	const helpers::FeatureVector& detectionFeatures, 
	const helpers::FeatureVector& divisionFeatures,
	const helpers::FeatureVector& appearanceFeatures,
	const helpers::FeatureVector& disappearanceFeatures):
	id_(id),
	detection_(detectionFeatures),
	division_(divisionFeatures),
	appearance_(appearanceFeatures),
	disappearance_(disappearanceFeatures)
{}

const int SegmentationHypothesis::readFromJson(const Json::Value& entry)
{
	if(!entry.isObject())
		throw std::runtime_error("Cannot extract SegmentationHypothesis from non-object JSON entry");
	if(!entry.isMember(JsonTypeNames[JsonTypes::Id]) || !entry[JsonTypeNames[JsonTypes::Id]].isInt() 
		|| !entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
		throw std::runtime_error("JSON entry for SegmentationHytpohesis is invalid");

	id_ = entry[JsonTypeNames[JsonTypes::Id]].asInt();

	// define helper lambda function used below
	auto extractFeatures = [&](JsonTypes type)
	{
		helpers::FeatureVector featVec;
		if(!entry.isMember(JsonTypeNames[type]))
			throw std::runtime_error("Could not find Json tags for " + JsonTypeNames[type]);

		const Json::Value features = entry[JsonTypeNames[type]];

		if(!features.size() > 0)
			throw std::runtime_error("Features may not be empty for " + JsonTypeNames[type]);

		for(int i = 0; i < (int)features.size(); i++)
		{
			featVec.push_back(features[i].asDouble());
		}
		return featVec;
	};

	detection_ = Variable(extractFeatures(JsonTypes::Features));
	if(entry.isMember(JsonTypeNames[JsonTypes::DivisionFeatures]))
		division_ = Variable(extractFeatures(JsonTypes::DivisionFeatures));

	// read appearance and disappearance if present
	if(entry.isMember(JsonTypeNames[JsonTypes::AppearanceFeatures]))
		appearance_ = Variable(extractFeatures(JsonTypes::AppearanceFeatures));
	if(entry.isMember(JsonTypeNames[JsonTypes::DisappearanceFeatures]))
		disappearance_ = Variable(extractFeatures(JsonTypes::DisappearanceFeatures));

	// std::cout << "Found segmentation hypothesis with id " << id_ << std::endl;

	return id_;
}

void SegmentationHypothesis::toDot(std::ostream& stream, const Solution* sol) const
{
	stream << "\t" << id_ << " [ label=\"" << id_ << ", div=";

	if(sol != nullptr && division_.getOpenGMVariableId() > 0 && sol->at(division_.getOpenGMVariableId()) > 0)
		stream << "yes";
	else
		stream << "no";

	stream << "\" ";

	// highlight active nodes in blue
	if(sol != nullptr && detection_.getOpenGMVariableId() > 0 && sol->at(detection_.getOpenGMVariableId()) > 0)
		stream << "color=\"blue\" fontcolor=\"blue\" ";

	stream <<  "]; \n" << std::flush;
}

void SegmentationHypothesis::Variable::addToOpenGM(
	GraphicalModelType& model, 
	WeightsType& weights, 
	const std::vector<size_t>& weightIds)
{
	// only add variable if there are any features
	if(features_.size() == 0)
		return;

	// Add variable to model. All Variables are binary!
	model.addVariable(2);
	size_t variableId = model.numberOfVariables() - 1;

	// add unary factor to model
	std::vector<FeaturesAndIndicesType> featuresAndWeightsPerLabel;
	size_t numFeatures = features_.size();
	assert(weightIds.size() == numFeatures * 2);

	for(size_t l = 0; l < 2; l++)
	{
		FeaturesAndIndicesType featureAndIndex;

		featureAndIndex.features = features_;
		for(size_t i = 0; i < numFeatures; ++i)
			featureAndIndex.weightIds.push_back(weightIds[l * numFeatures + i]);

		featuresAndWeightsPerLabel.push_back(featureAndIndex);
	}

	LearnableUnaryFuncType unary(weights, featuresAndWeightsPerLabel);
	GraphicalModelType::FunctionIdentifier fid = model.addFunction(unary);
	model.addFactor(fid, &variableId, &variableId+1);

	opengmVariableId_ = variableId;
}

void SegmentationHypothesis::addIncomingConstraintToOpenGM(GraphicalModelType& model)
{
	if(incomingLinks_.size() == 0 && appearance_.getOpenGMVariableId() < 0)
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
    	addOpenGMVariableStateToConstraint(incomingConsistencyConstraint, incomingLinks_[i]->getOpenGMVariableId(),
    		1.0, constraintShape, factorVariables, model);
    }

    // add this variable's state with negative coefficient
	addOpenGMVariableStateToConstraint(incomingConsistencyConstraint, detection_.getOpenGMVariableId(),
		-1.0, constraintShape, factorVariables, model);

    // add appearance with positive coefficient, if any
    if(appearance_.getOpenGMVariableId() >= 0)
    {
    	addOpenGMVariableStateToConstraint(incomingConsistencyConstraint, appearance_.getOpenGMVariableId(),
    		1.0, constraintShape, factorVariables, model);
    }

    incomingConsistencyConstraint.setBound( 0 );
    incomingConsistencyConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::Equal);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &incomingConsistencyConstraint, &incomingConsistencyConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addOutgoingConstraintToOpenGM(GraphicalModelType& model)
{
	if(outgoingLinks_.size() == 0 && disappearance_.getOpenGMVariableId() < 0)
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
        addOpenGMVariableStateToConstraint(outgoingConsistencyConstraint, outgoingLinks_[i]->getOpenGMVariableId(),
    		1.0, constraintShape, factorVariables, model);
    }

    // add this variable's state with negative coefficient
    addOpenGMVariableStateToConstraint(outgoingConsistencyConstraint, detection_.getOpenGMVariableId(),
		-1.0, constraintShape, factorVariables, model);

	// also the division node, if any
    if(division_.getOpenGMVariableId() >= 0)
    {
    	addOpenGMVariableStateToConstraint(outgoingConsistencyConstraint, division_.getOpenGMVariableId(),
    		-1.0, constraintShape, factorVariables, model);
    }

    // add appearance with positive coefficient, if any
    if(disappearance_.getOpenGMVariableId() >= 0)
    {
    	addOpenGMVariableStateToConstraint(outgoingConsistencyConstraint, disappearance_.getOpenGMVariableId(),
    		1.0, constraintShape, factorVariables, model);
    }

    outgoingConsistencyConstraint.setBound( 0 );
    outgoingConsistencyConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::Equal);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &outgoingConsistencyConstraint, &outgoingConsistencyConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addDivisionConstraintToOpenGM(GraphicalModelType& model)
{
	if(division_.getOpenGMVariableId() < 0)
		return;

	// add constraint for sum of ougoing = this label + division
	LinearConstraintFunctionType::LinearConstraintType divisionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;

	// add this variable's state with negative coefficient
	addOpenGMVariableToConstraint(divisionConstraint, detection_.getOpenGMVariableId(),
		1, -1.0, constraintShape, factorVariables, model);

	addOpenGMVariableToConstraint(divisionConstraint, division_.getOpenGMVariableId(),
		1, 1.0, constraintShape, factorVariables, model);

    divisionConstraint.setBound( 0 );
    divisionConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::LessEqual);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &divisionConstraint, &divisionConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addExclusionConstraintToOpenGM(GraphicalModelType& model, int openGMVarA, int openGMVarB)
{
	if(openGMVarA < 0 || openGMVarB < 0)
		return;

	// make sure they are ordered properly
	if(openGMVarA > openGMVarB)
		std::swap(openGMVarA, openGMVarB);

	// add constraint: at least one of the two variables must take state 0 (A(0) + B(0) >= 1)
	LinearConstraintFunctionType::LinearConstraintType exclusionConstraint;
	std::vector<LabelType> factorVariables;
	std::vector<LabelType> constraintShape;

	addOpenGMVariableToConstraint(exclusionConstraint, openGMVarA,
		0, 1.0, constraintShape, factorVariables, model);

	addOpenGMVariableToConstraint(exclusionConstraint, openGMVarB,
		0, 1.0, constraintShape, factorVariables, model);

    exclusionConstraint.setBound( 1 );
    exclusionConstraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::GreaterEqual);

    LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &exclusionConstraint, &exclusionConstraint + 1);
    GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = model.addFunction(linearConstraintFunction);
    model.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
}

void SegmentationHypothesis::addToOpenGMModel(
	GraphicalModelType& model, 
	WeightsType& weights, 
	const std::vector<size_t>& detectionWeightIds,
	const std::vector<size_t>& divisionWeightIds,
	const std::vector<size_t>& appearanceWeightIds,
	const std::vector<size_t>& disappearanceWeightIds)
{
	detection_.addToOpenGM(model, weights, detectionWeightIds);
	if(detection_.getOpenGMVariableId() < 0)
		throw std::runtime_error("Detection variable must have some features!");

	division_.addToOpenGM(model, weights, divisionWeightIds);
	appearance_.addToOpenGM(model, weights, appearanceWeightIds);
	disappearance_.addToOpenGM(model, weights, disappearanceWeightIds);
	addIncomingConstraintToOpenGM(model);
	addOutgoingConstraintToOpenGM(model);
	addDivisionConstraintToOpenGM(model);

	// add exclusion constraints:
	if(appearance_.getOpenGMVariableId() >= 0)
	{
		for(auto link : incomingLinks_)
			addExclusionConstraintToOpenGM(model, appearance_.getOpenGMVariableId(), link->getOpenGMVariableId());
	}

	if(disappearance_.getOpenGMVariableId() >= 0)
	{
		for(auto link : outgoingLinks_)
			addExclusionConstraintToOpenGM(model, disappearance_.getOpenGMVariableId(), link->getOpenGMVariableId());

		if(division_.getOpenGMVariableId() >= 0)
			addExclusionConstraintToOpenGM(model, disappearance_.getOpenGMVariableId(), division_.getOpenGMVariableId());
	}
}

void SegmentationHypothesis::addIncomingLink(std::shared_ptr<LinkingHypothesis> link)
{
	if(detection_.getOpenGMVariableId() >= 0)
		throw std::runtime_error("Links must be added before the segmentation hypothesis is added to the OpenGM model");
	if(link)
		incomingLinks_.push_back(link);
}

void SegmentationHypothesis::addOutgoingLink(std::shared_ptr<LinkingHypothesis> link)
{
	if(detection_.getOpenGMVariableId() >= 0)
		throw std::runtime_error("Links must be added before the segmentation hypothesis is added to the OpenGM model");
	if(link)
		outgoingLinks_.push_back(link);
}

size_t SegmentationHypothesis::getNumActiveIncomingLinks(const Solution& sol) const
{
	size_t sum = 0;
	for(auto link : incomingLinks_)
	{
		if(link->getOpenGMVariableId() < 0)
			throw std::runtime_error("Cannot compute sum of active links if they have not been added to opengm");
		sum += sol[link->getOpenGMVariableId()];
	}
	return sum;
}

size_t SegmentationHypothesis::getNumActiveOutgoingLinks(const Solution& sol) const
{
	size_t sum = 0;
	for(auto link : outgoingLinks_)
	{
		if(link->getOpenGMVariableId() < 0)
			throw std::runtime_error("Cannot compute sum of active links if they have not been added to opengm");
		sum += sol[link->getOpenGMVariableId()];
	}
	return sum;
}

bool SegmentationHypothesis::verifySolution(const Solution& sol) const
{
	size_t ownValue = sol[detection_.getOpenGMVariableId()];
	size_t divisionValue = sol[division_.getOpenGMVariableId()];
	
	//--------------------------------
	// check incoming
	size_t sumIncoming = getNumActiveIncomingLinks(sol);

	if(appearance_.getOpenGMVariableId() >= 0)
		sumIncoming += sol[appearance_.getOpenGMVariableId()];

	// TODO: how are we dealing with appearance / disappearance?
	if(incomingLinks_.size() > 0 && sumIncoming != ownValue)
	{
		std::cout << "At node " << id_ << ": incoming=" << sumIncoming << " is NOT EQUAL to " << ownValue << std::endl;
		return false;
	}

	//--------------------------------
	// check outgoing
	size_t sumOutgoing = getNumActiveOutgoingLinks(sol);

	if(disappearance_.getOpenGMVariableId() >= 0)
		sumOutgoing += sol[disappearance_.getOpenGMVariableId()];

	// TODO: how are we dealing with appearance / disappearance?
	if(outgoingLinks_.size() > 0 && sumOutgoing != ownValue + divisionValue)
	{
		std::cout << "At node " << id_ << ": outgoing=" << sumOutgoing << " is NOT EQUAL to " << ownValue << " + " << divisionValue << " (own+div)" << std::endl;
		return false;
	}

	//--------------------------------
	// check divisions
	if(divisionValue > ownValue)
	{
		std::cout << "At node " << id_ << ": division > value: " << divisionValue << " > " << ownValue << " -> INVALID!" << std::endl;
		return false;
	}


	//--------------------------------
	// check division vs disappearance
	if(disappearance_.getOpenGMVariableId() >= 0 && divisionValue + sol[disappearance_.getOpenGMVariableId()] > 1)
	{
		std::cout << "At node " << id_ << ": division and disappearance are BOTH active -> INVALID!" << std::endl;
		return false;
	}

	return true;
}

} // end namespace constracking
