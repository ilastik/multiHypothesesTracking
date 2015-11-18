#ifndef COVER_TREE_INFERENCE_MODEL_H
#define COVER_TREE_INFERENCE_MODEL_H

#include "covertree.h"
#include <utility>
#include <opengm/opengm.hxx>
#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/functions/constraint_functions/linear_constraint_function.hxx>
#include <opengm/functions/learnable/lunary.hxx>
#include <opengm/functions/unary_loss_function.hxx>
#include <opengm/functions/explicit_function.hxx>
#include <opengm/learning/dataset/editabledataset.hxx>
#include <opengm/learning/loss/hammingloss.hxx>

namespace covertree
{

class CoverTreeInferenceModel
{
private:
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
	typedef std::map<CoverTreeNodePtr, std::pair<size_t, size_t> > VariableMap;
	typedef opengm::learning::Weights<ValueType> WeightsType;

public: // API
	CoverTreeInferenceModel() = delete;
	CoverTreeInferenceModel(CoverTree& tree, size_t max_label, size_t max_add);

	// use labels in CoverTreeNodes as ground truth and learn weights
	void learn();

	const WeightsType& getWeights() const { return weights_; }
	const size_t getNumWeights() const { return tree_.getRoot()->getCoverFeatures().size() * max_label_ + tree_.getRoot()->getAddFeatures().size() * max_add_; }

	// use given weights
	void infer(std::vector<ValueType>& weights);

private: // methods
	void buildModel(WeightsType& weights);

	void storeSolutionInTree(Solution& solution);

private: // members
	CoverTree& tree_;
	size_t max_label_;
	size_t max_add_;

	/// opengm model of this cover graph
	GraphicalModelType graphical_model_;

	/// weights object
	DatasetType::Weights weights_;

	/// mapping so that we can enter inference results into the tree
	VariableMap variable_mapping_;
};

} // end namespace covertree

#endif
