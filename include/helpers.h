#ifndef HELPERS_H
#define HELPERS_H

#include <iostream>
#include <vector>

// opengm
#include <opengm/opengm.hxx>
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/functions/constraint_functions/linear_constraint_function.hxx>
#include <opengm/functions/learnable/lunary.hxx>
#include <opengm/functions/learnable/lweightedsum_of_functions.hxx>
#include <opengm/functions/unary_loss_function.hxx>
#include <opengm/functions/explicit_function.hxx>
#include <opengm/learning/dataset/editabledataset.hxx>
#include <opengm/learning/loss/hammingloss.hxx>

// json
#include <json/json.h>

namespace helpers
{

// --------------------------------------------------------------
// typedefs
// --------------------------------------------------------------

// typedefs for opengm model
typedef double ValueType;
typedef size_t IndexType;
typedef size_t LabelType;
typedef opengm::LinearConstraintFunction<ValueType, IndexType, LabelType> LinearConstraintFunctionType;
typedef opengm::functions::learnable::LUnary<ValueType, IndexType, LabelType> LearnableUnaryFuncType;
typedef opengm::functions::learnable::FeaturesAndIndices<ValueType, IndexType> FeaturesAndIndicesType;
typedef opengm::functions::learnable::LWeightedSumOfFunctions<ValueType, IndexType, LabelType> LearnableWeightedSumOfFuncType;
typedef opengm::ExplicitFunction<ValueType, IndexType, LabelType> ExplicitFunctionType;
typedef opengm::UnaryLossFunction<ValueType, IndexType, LabelType> UnaryLossFunctionType;
typedef opengm::meta::TypeListGenerator< LearnableUnaryFuncType, LearnableWeightedSumOfFuncType, LinearConstraintFunctionType, UnaryLossFunctionType, ExplicitFunctionType >::type FunctionTypeList;
typedef opengm::GraphicalModel<ValueType, opengm::Adder, FunctionTypeList> GraphicalModelType;
typedef opengm::learning::HammingLoss	LossType;
typedef opengm::datasets::EditableDataset<GraphicalModelType, LossType> DatasetType;
typedef std::vector<LabelType> Solution;
typedef opengm::learning::Weights<ValueType> WeightsType;
typedef LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType IndicatorVariableType;

// other stuff
typedef std::vector<ValueType> FeatureVector;
typedef std::vector<FeatureVector> StateFeatureVector;

// --------------------------------------------------------------
// functions
// --------------------------------------------------------------
/**
 * @brief Output std vectors of stuff
 * 
 * @param stream output stream
 * @param feats the vector of stuff
 */
template<class T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& feats)
{
	stream << "(";
	for(auto f_it = feats.begin(); f_it != feats.end(); ++f_it)
	{
		if(f_it != feats.begin())
			stream << ", ";
		stream << *f_it;
	}
	stream << ")";
	return stream;
}

/**
 * @brief create an indicator variable and add it to the constraint. 
 * 		  Assumes the function argument index to be the next number
 * 
 * @param constraint the constraint function to add to
 * @param opengmVariableId the opengm variable in question
 * @param state which state of the variable are we interested in
 * @param coefficient by what coefficient is the indicator variable to be multiplied
 * @param constraintShape a vector containing the number of labels of all previous variables of the constraint
 * @param factorVariables list of opengm variables that this constraint should reason about
 * @param model the opengm model
 */
void addOpenGMVariableToConstraint(
	LinearConstraintFunctionType::LinearConstraintType& constraint, 
	size_t opengmVariableId,
	size_t state, 
	double coefficient,
	std::vector<LabelType>& constraintShape,
	std::vector<LabelType>& factorVariables,
	GraphicalModelType& model);

/**
 * @brief add the variable's value to the constraint, not just an indicator variable
 * 
 * @param constraint the constraint function to add to
 * @param opengmVariableId the opengm variable in question
 * @param coefficient by what coefficient is the indicator variable to be multiplied
 * @param constraintShape a vector containing the number of labels of all previous variables of the constraint
 * @param factorVariables list of opengm variables that this constraint should reason about
 * @param model the opengm model
 */
void addOpenGMVariableStateToConstraint(
	LinearConstraintFunctionType::LinearConstraintType& constraint, 
	size_t opengmVariableId,
	double coefficient,
	std::vector<LabelType>& constraintShape,
	std::vector<LabelType>& factorVariables,
	GraphicalModelType& model);

// --------------------------------------------------------------
// json type definitions
// --------------------------------------------------------------

/**
 * @brief Enumerate the strings for attributes used in the Json files
 */
enum class JsonTypes {Segmentations, 
	Links, 
	Exclusions, 
	LinkResults, 
	DivisionResults,
	DetectionResults,
	SrcId, 
	DestId, 
	Value, 
	Id, 
	Features, 
	DivisionFeatures,
	AppearanceFeatures,
	DisappearanceFeatures,
	Weights,
	// settings-related
	Settings,
	StatesShareWeights,
	OptimizerEpGap,
	OptimizerVerbose,
	OptimizerNumThreads,
	AllowPartialMergerAppearance,
	RequireSeparateChildrenOfDivision
};

/// mapping from JsonTypes to strings which are used in the Json files
extern std::map<JsonTypes, std::string> JsonTypeNames;

/**
 * @brief save weights to Json
 * 
 * @param weights a vector of weights (not the OpenGM Weight object)
 * @param filename file to save the weights to
 * @param weightDescriptions an optional vector that contains a description for each weight
 */
void saveWeightsToJson(
	const std::vector<ValueType>& weights, 
	const std::string& filename,
	const std::vector<std::string>& weightDescriptions = {});

/**
 * @brief read weights from Json
 * 
 * @param filename
 * @return a vector of weights
 */
std::vector<ValueType> readWeightsFromJson(const std::string& filename);

/**
 * @brief Extract a list of detection/division/disapperance/appearance features for each state from a given entry
 * 
 * @param entry the json root to extract the features from
 * @param type the type of feature to extract (checks for the respectively named member!)
 * 
 * @return a vector of FeatureVectors, one for each state the variable can take
 */
StateFeatureVector extractFeatures(const Json::Value& entry, JsonTypes type);

}

#endif
