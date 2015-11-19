#include "model.h"
#include <json/json.h>
#include <fstream>
#include <stdexcept>
#include <numeric>

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
		std::shared_ptr<LinkingHypothesis> hyp = std::make_shared<LinkingHypothesis>();
		hyp->readFromJson(jsonHyp);
		hyp->registerWithSegmentations(segmentationHypotheses_);
		linkingHypotheses_.push_back(hyp);
	}

	const Json::Value exclusions = root[JsonTypeNames[JsonTypes::Exclusions]];
	std::cout << "\tcontains " << exclusions.size() << " exclusions" << std::endl;
	for(int i = 0; i < (int)exclusions.size(); i++)
	{
		const Json::Value jsonExc = exclusions[i];
		exclusionConstraints_.push_back(ExclusionConstraint());
		ExclusionConstraint& exclusion = exclusionConstraints_.back();
		exclusion.readFromJson(jsonExc);
	}
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
			numLinkFeatures = (*iter)->getNumFeatures();
		else
			if((*iter)->getNumFeatures() != numLinkFeatures)
				throw std::runtime_error("Detections do not have the same number of features!");
	}

	// we need two sets of weights for all features to represent state "on" and "off"!
	return 2 * (numDetFeatures + numDivFeatures + numLinkFeatures);
}

void Model::initializeOpenGMModel(WeightsType& weights)
{
	std::cout << "Initializing opengm model..." << std::endl;
	// we need two sets of weights for all features to represent state "on" and "off"!
	size_t numLinkWeights = 2 * linkingHypotheses_.front()->getNumFeatures();
	std::vector<size_t> linkWeightIds(numLinkWeights);
	std::iota(linkWeightIds.begin(), linkWeightIds.end(), 0); // fill with increasing values starting at 0

	// first add all link variables, because segmentations will use them when defining constraints
	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		(*iter)->addToOpenGMModel(model_, weights, linkWeightIds);
	}

	size_t numDetWeights = 2 * segmentationHypotheses_.begin()->second.getNumFeatures();
	std::vector<size_t> detWeightIds(numDetWeights);
	std::iota(detWeightIds.begin(), detWeightIds.end(), numLinkWeights); // fill with increasing values starting at 0

	size_t numDivWeights = 2 * segmentationHypotheses_.begin()->second.getNumDivisionFeatures();
	std::vector<size_t> divWeightIds(numDivWeights);
	std::iota(divWeightIds.begin(), divWeightIds.end(), numLinkWeights + numDetWeights); // fill with increasing values starting at 0

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		iter->second.addToOpenGMModel(model_, weights, detWeightIds, divWeightIds);
	}

	for(auto iter = exclusionConstraints_.begin(); iter != exclusionConstraints_.end() ; ++iter)
	{
		iter->addToOpenGMModel(model_, segmentationHypotheses_);
	}
}

Solution Model::infer(const std::vector<ValueType>& weights)
{
	// use weights that were given
	WeightsType weightObject(computeNumWeights());
	assert(weights.size() == weightObject.numberOfWeights());
	for(size_t i = 0; i < weights.size(); i++)
		weightObject.setWeight(i, weights[i]);
	initializeOpenGMModel(weightObject);

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

std::vector<ValueType> Model::learn(const std::string gt_filename)
{
	DatasetType dataset;
	WeightsType initialWeights(computeNumWeights());
	dataset.setWeights(initialWeights);
	initializeOpenGMModel(dataset.getWeights());

	// Solution gt;
	// NodeVisitor node_to_GT_visitor([&](CoverTreeNodePtr node){
	// 	gt.push_back(node->getCoverLabel());
	// 	gt.push_back(node->getAddLabel());
	// });
	// tree_.getRoot()->accept(&node_to_GT_visitor);

	// std::cout << "got ground truth: ";
	// for(size_t s : gt)
	// 	std::cout << s << " ";
	// std::cout << std::endl;

	// dataset.pushBackInstance(graphical_model_, gt);
	
	std::cout << "Done setting up dataset, creating learner" << std::endl;
	opengm::learning::StructMaxMargin<DatasetType>::Parameter learnerParam;
	opengm::learning::StructMaxMargin<DatasetType> learner(dataset, learnerParam);

#ifdef WITH_CPLEX
	typedef opengm::LPCplex2<GraphicalModelType, opengm::Minimizer> OptimizerType;
#else
	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
#endif
	
	OptimizerType::Parameter optimizerParam;
	optimizerParam.integerConstraintNodeVar_ = true;
	optimizerParam.relaxation_ = OptimizerType::Parameter::TightPolytope;
	optimizerParam.verbose_ = true;
	optimizerParam.useSoftConstraints_ = false;

	std::cout << "Calling learn()..." << std::endl;
	learner.learn<OptimizerType>(optimizerParam); 
	std::cout << "extracting weights" << std::endl;
	const WeightsType& finalWeights = learner.getWeights();
	std::vector<double> resultWeights;
	for(size_t i = 0; i < finalWeights.numberOfWeights(); ++i)
		resultWeights.push_back(finalWeights.getWeight(i));
	return resultWeights;
}

} // end namespace mht