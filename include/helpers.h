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
std::ostream& operator<<(std::ostream& stream, const FeatureVector& feats);

}

#endif
