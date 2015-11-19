#include "model.h"
#include <json/json.h>
#include <fstream>
#include <stdexcept>

#ifdef WITH_CPLEX
#include <opengm/inference/lpcplex2.hxx>
#else
#include <opengm/inference/lpgurobi2.hxx>
#endif

#include <opengm/learning/struct-max-margin.hxx>

namespace mht
{
	
void Model::readFromJson(const std::string& filename)
{
	std::ifstream input(filename.c_str());
	if(!input.good())
		throw std::runtime_error("Could not open JSON file " + filename);

	Json::Value root;
	input >> root;

	const Json::Value segmentationHypotheses = root[JsonTypeNames[JsonTypes::Segmentations]];
	std::cout << "\tcontains " << segmentationHypotheses.size() << " segmentation hypotheses" << std::endl;
	
	for(int i = 0; i < (int)segmentationHypotheses.size(); i++)
	{
		const Json::Value jsonHyp = segmentationHypotheses[i];
		SegmentationHypothesis hyp;
		int id = hyp.readFromJson(jsonHyp);
		segmentationHypotheses_[id] = hyp;
	}

	const Json::Value linkingHypotheses = root[JsonTypeNames[JsonTypes::Links]];
	std::cout << "\tcontains " << linkingHypotheses.size() << " linking hypotheses" << std::endl;
	for(int i = 0; i < (int)linkingHypotheses.size(); i++)
	{
		const Json::Value jsonHyp = linkingHypotheses[i];
		LinkingHypothesis hyp;
		hyp.readFromJson(jsonHyp);
		hyp.registerWithSegmentations(segmentationHypotheses_);
	}

	// const Json::Value exclusions = root[JsonTypeNames[JsonTypes::Exclusions]];
	// std::cout << "\tcontains " << exclusions.size() << " exclusions" << std::endl;
	// for(int i = 0; i < (int)exclusions.size(); i++)
	// {
	// 	const Json::Value hyp = exclusions[i];
	// }
}

size_t Model::computeNumWeights() const
{
	int numDetFeatures = -1;
	int numDivFeatures = -1;
	int numLinkFeatures = -1;

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		if(numDetFeatures < 0)
			numDetFeatures = iter->second.getNumFeatures();
		else
			if(iter->second.getNumFeatures() != numDetFeatures)
				throw std::runtime_error("Detections do not have the same number of features!");

		if(numDivFeatures < 0)
			numDivFeatures = iter->second.getNumDivisionFeatures();
		else
			if(iter->second.getNumDivisionFeatures() != numDivFeatures)
				throw std::runtime_error("Divisions do not have the same number of features!");
	}

	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		if(numLinkFeatures < 0)
			numLinkFeatures = iter->getNumFeatures();
		else
			if(iter->getNumFeatures() != numLinkFeatures)
				throw std::runtime_error("Detections do not have the same number of features!");
	}

	// we need two sets of weights for all features to represent state "on" and "off"
	return 2 * (numDetFeatures + numDivFeatures + numLinkFeatures);
}

void Model::initializeOpenGMModel(WeightsType& weights)
{
	// first add all link variables, because segmentations will use them when defining constraints
	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		iter->addToOpenGMModel(model_, weights, {});
	}

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		iter->second.addToOpenGMModel(model_, weights, {}, {});
	}
}

Solution Model::infer()
{
#ifdef WITH_CPLEX
	std::cout << "Using cplex optimizer" << std::endl;
	typedef opengm::LPCplex2<GraphicalModelType, opengm::Minimizer> OptimizerType;
#else
	std::cout << "Using gurobi optimizer" << std::endl;
	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
#endif
	OptimizerType::Parameter optimizerParam;
	optimizerParam.integerConstraintNodeVar_ = true;
	optimizerParam.relaxation_ = OptimizerType::Parameter::TightPolytope;
	optimizerParam.verbose_ = true;
	optimizerParam.useSoftConstraints_ = false;

	OptimizerType optimizer(model_, optimizerParam);

	Solution solution(model_.numberOfVariables());
	OptimizerType::VerboseVisitorType optimizerVisitor;
	optimizer.infer(optimizerVisitor);
	optimizer.arg(solution);
	std::cout << "solution has energy: " << optimizer.value() << std::endl;

	std::cout << " found solution: ";
	for(size_t s : solution)
		std::cout << s << " ";
	std::cout << std::endl;

	return solution;
}

void Model::learn()
{
// 	DatasetType dataset;
// 	WeightsType initialWeights(getNumWeights());
// 	dataset.setWeights(initialWeights);
// 	buildModel(dataset.getWeights());

// 	Solution gt;
// 	NodeVisitor node_to_GT_visitor([&](CoverTreeNodePtr node){
// 		gt.push_back(node->getCoverLabel());
// 		gt.push_back(node->getAddLabel());
// 	});
// 	tree_.getRoot()->accept(&node_to_GT_visitor);

// 	std::cout << "got ground truth: ";
// 	for(size_t s : gt)
// 		std::cout << s << " ";
// 	std::cout << std::endl;

// 	dataset.pushBackInstance(graphical_model_, gt);
	
// 	std::cout << "Done setting up dataset, creating learner" << std::endl;
// 	opengm::learning::StructMaxMargin<DatasetType>::Parameter learner_param;
// 	opengm::learning::StructMaxMargin<DatasetType> learner(dataset, learner_param);

// #ifdef WITH_CPLEX
// 	typedef opengm::LPCplex2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #else
// 	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #endif
	
// 	OptimizerType::Parameter optimizer_param;
// 	optimizer_param.integerConstraintNodeVar_ = true;
// 	optimizer_param.relaxation_ = OptimizerType::Parameter::TightPolytope;
// 	optimizer_param.verbose_ = true;
// 	optimizer_param.useSoftConstraints_ = false;

// 	std::cout << "Calling learn()..." << std::endl;
// 	learner.learn<OptimizerType>(optimizer_param); 
// 	std::cout << "extracting weights" << std::endl;
// 	weights_ = learner.getWeights();
}

} // end namespace mht