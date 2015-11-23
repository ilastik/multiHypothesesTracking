#ifndef HELPERS_H
#define HELPERS_H

#include <iostream>
#include <vector>

// opengm
#include <opengm/opengm.hxx>
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/functions/constraint_functions/linear_constraint_function.hxx>
#include <opengm/functions/learnable/lunary.hxx>
#include <opengm/functions/unary_loss_function.hxx>
#include <opengm/functions/explicit_function.hxx>
#include <opengm/learning/dataset/editabledataset.hxx>
#include <opengm/learning/loss/hammingloss.hxx>


namespace mht
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
typedef opengm::ExplicitFunction<ValueType, IndexType, LabelType> ExplicitFunctionType;
typedef opengm::UnaryLossFunction<ValueType, IndexType, LabelType> UnaryLossFunctionType;
typedef opengm::meta::TypeListGenerator< LearnableUnaryFuncType, LinearConstraintFunctionType, UnaryLossFunctionType, ExplicitFunctionType >::type FunctionTypeList;
typedef opengm::GraphicalModel<ValueType, opengm::Adder, FunctionTypeList> GraphicalModelType;
typedef opengm::learning::HammingLoss	LossType;
typedef opengm::datasets::EditableDataset<GraphicalModelType, LossType> DatasetType;
typedef std::vector<LabelType> Solution;
typedef opengm::learning::Weights<ValueType> WeightsType;

// other stuff
typedef std::vector<ValueType> FeatureVector;

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
	SrcId, 
	DestId, 
	Value, 
	Id, 
	Features, 
	DivisionFeatures, 
	Weights
};

/// mapping from JsonTypes to strings which are used in the Json files
extern std::map<JsonTypes, std::string> JsonTypeNames;

/**
 * @brief save weights to Json
 * 
 * @param weights a vector of weights (not the OpenGM Weight object)
 * @param filename file to save the weights to
 */
void saveWeightsToJson(const std::vector<ValueType>& weights, const std::string& filename);

/**
 * @brief read weights from Json
 * 
 * @param filename
 * @return a vector of weights
 */
std::vector<ValueType> readWeightsFromJson(const std::string& filename);

}

#endif
