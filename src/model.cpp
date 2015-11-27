#include "model.h"
#include <json/json.h>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <sstream>

// include the LPDef symbols only once!
#undef OPENGM_LPDEF_NO_SYMBOLS
#include <opengm/inference/auxiliary/lpdef.hxx>

#ifdef WITH_CPLEX
#include <opengm/inference/lpcplex2.hxx>
#else
#include <opengm/inference/lpgurobi2.hxx>
#endif

#include <opengm/learning/struct-max-margin.hxx>

using namespace helpers;

namespace mht
{

void Model::readFromJson(const std::string& filename)
{
	std::ifstream input(filename.c_str());
	if(!input.good())
		throw std::runtime_error("Could not open JSON model file " + filename);

	Json::Value root;
	input >> root;

	// read weight configuration
	if(root.isMember(JsonTypeNames[JsonTypes::StatesShareWeights]))
		statesShareWeights_ = root[JsonTypeNames[JsonTypes::StatesShareWeights]].asBool();
	else
		statesShareWeights_ = false;

	// read segmentation hypotheses
	const Json::Value segmentationHypotheses = root[JsonTypeNames[JsonTypes::Segmentations]];
	std::cout << "\tcontains " << segmentationHypotheses.size() << " segmentation hypotheses" << std::endl;
	
	for(int i = 0; i < (int)segmentationHypotheses.size(); i++)
	{
		const Json::Value jsonHyp = segmentationHypotheses[i];
		SegmentationHypothesis hyp;
		int id = hyp.readFromJson(jsonHyp);
		segmentationHypotheses_[id] = hyp;
	}

	// read linking hypotheses
	const Json::Value linkingHypotheses = root[JsonTypeNames[JsonTypes::Links]];
	std::cout << "\tcontains " << linkingHypotheses.size() << " linking hypotheses" << std::endl;
	for(int i = 0; i < (int)linkingHypotheses.size(); i++)
	{
		const Json::Value jsonHyp = linkingHypotheses[i];
		std::shared_ptr<LinkingHypothesis> hyp = std::make_shared<LinkingHypothesis>();
		std::pair<int, int> ids = hyp->readFromJson(jsonHyp);
		hyp->registerWithSegmentations(segmentationHypotheses_);
		linkingHypotheses_[ids] = hyp;
	}

	// read exclusion constraints between detections
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

size_t Model::computeNumWeights()
{
	int numDetWeights = -1;
	int numDivWeights = -1;
	int numAppWeights = -1;
	int numDisWeights = -1;
	int numLinkWeights = -1;

	auto checkNumWeights = [&](const Variable& var, int& previousNumWeights, const std::string& name)
	{
		int numWeights = var.getNumWeights(statesShareWeights_);
		if(previousNumWeights < 0 && numWeights > 0)
			previousNumWeights = numWeights;
		else
			if(numWeights > 0 && numWeights != previousNumWeights)
				throw std::runtime_error(name + " do not have the same number of features/weights!");
	};

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		checkNumWeights(iter->second.getDetectionVariable(), numDetWeights, "Detections");
		checkNumWeights(iter->second.getDivisionVariable(), numDivWeights, "Divisions");
		checkNumWeights(iter->second.getAppearanceVariable(), numAppWeights, "Appearances");
		checkNumWeights(iter->second.getDisappearanceVariable(), numDisWeights, "Disappearances");
	}

	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		if(numLinkWeights < 0)
			numLinkWeights = iter->second->getVariable().getNumWeights(statesShareWeights_);
		else
			if(iter->second->getVariable().getNumWeights(statesShareWeights_) != numLinkWeights)
				throw std::runtime_error("Links do not have the same number of features!");
	}

	// we don't want -1 weights
	numDetWeights_ = std::max((int)0, numDetWeights);
	numDivWeights_ = std::max((int)0, numDivWeights);
	numAppWeights_ = std::max((int)0, numAppWeights);
	numDisWeights_ = std::max((int)0, numDisWeights);
	numLinkWeights_ = std::max((int)0, numLinkWeights);

	return numDetWeights_ + numDivWeights_ + numAppWeights_ + numDisWeights_ + numLinkWeights_;
}

void Model::initializeOpenGMModel(WeightsType& weights)
{
	// make sure the numbers of features are initialized
	computeNumWeights();

	std::cout << "Initializing opengm model..." << std::endl;
	// we need two sets of weights for all features to represent state "on" and "off"!
	std::vector<size_t> linkWeightIds(numLinkWeights_);
	std::iota(linkWeightIds.begin(), linkWeightIds.end(), 0); // fill with increasing values starting at 0

	// first add all link variables, because segmentations will use them when defining constraints
	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		iter->second->addToOpenGMModel(model_, weights, statesShareWeights_, linkWeightIds);
	}

	std::vector<size_t> detWeightIds(numDetWeights_);
	std::iota(detWeightIds.begin(), detWeightIds.end(), numLinkWeights_); // fill with increasing values starting at the next valid index

	std::vector<size_t> divWeightIds(numDivWeights_);
	std::iota(divWeightIds.begin(), divWeightIds.end(), numLinkWeights_ + numDetWeights_);

	std::vector<size_t> appWeightIds(numAppWeights_);
	std::iota(appWeightIds.begin(), appWeightIds.end(), numLinkWeights_ + numDetWeights_ + numDivWeights_);

	std::vector<size_t> disWeightIds(numDisWeights_);
	std::iota(disWeightIds.begin(), disWeightIds.end(), numLinkWeights_ + numDetWeights_ + numDivWeights_ + numAppWeights_);

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		iter->second.addToOpenGMModel(model_, weights, statesShareWeights_, detWeightIds, divWeightIds, appWeightIds, disWeightIds);
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

	std::cout << " found solution: " << solution << std::endl;

	return solution;
}

std::vector<ValueType> Model::learn(const std::string& gt_filename)
{
	DatasetType dataset;
	WeightsType initialWeights(computeNumWeights());
	dataset.setWeights(initialWeights);
	initializeOpenGMModel(dataset.getWeights());

	Solution gt = readGTfromJson(gt_filename);
	dataset.pushBackInstance(model_, gt);
	
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

Solution Model::readGTfromJson(const std::string& filename)
{
	std::ifstream input(filename.c_str());
	if(!input.good())
		throw std::runtime_error("Could not open JSON ground truth file " + filename);

	Json::Value root;
	input >> root;

	const Json::Value linkingResults = root[JsonTypeNames[JsonTypes::LinkResults]];
	std::cout << "\tcontains " << linkingResults.size() << " linking annotations" << std::endl;

	// create a solution vector that holds a value for each segmentation / detection / link
	Solution solution(model_.numberOfVariables(), 0);

	// first set all source nodes to active. If a node is already active, this means a division
	for(int i = 0; i < linkingResults.size(); ++i)
	{
		const Json::Value jsonHyp = linkingResults[i];
		int srcId = jsonHyp[JsonTypeNames[JsonTypes::SrcId]].asInt();
		int destId = jsonHyp[JsonTypeNames[JsonTypes::DestId]].asInt();
		size_t value = jsonHyp[JsonTypeNames[JsonTypes::Value]].asUInt();
		if(value > 0)
		{
			// try to find link
			if(linkingHypotheses_.find(std::make_pair(srcId, destId)) == linkingHypotheses_.end())
			{
				std::stringstream s;
				s << "Cannot find link to annotate: " << srcId << " to " << destId;
				throw std::runtime_error(s.str());
			}
			
			// set link active
			std::shared_ptr<LinkingHypothesis> hyp = linkingHypotheses_[std::make_pair(srcId, destId)];
			solution[hyp->getVariable().getOpenGMVariableId()] = value;

			// set source active, if it was active already then this is a division
			if(solution[segmentationHypotheses_[srcId].getDetectionVariable().getOpenGMVariableId()] == 1)
				solution[segmentationHypotheses_[srcId].getDivisionVariable().getOpenGMVariableId()] = 1;
			else
			{
				if(solution[segmentationHypotheses_[srcId].getDetectionVariable().getOpenGMVariableId()] == 1)
					throw std::runtime_error("A source node has been used more than once!");
				solution[segmentationHypotheses_[srcId].getDetectionVariable().getOpenGMVariableId()] = 1;
			}
		}
	}

	// enable target nodes so that the last node of each track is also active
	for(int i = 0; i < linkingResults.size(); ++i)
	{
		const Json::Value jsonHyp = linkingResults[i];
		int srcId = jsonHyp[JsonTypeNames[JsonTypes::SrcId]].asInt();
		int destId = jsonHyp[JsonTypeNames[JsonTypes::DestId]].asInt();
		bool value = jsonHyp[JsonTypeNames[JsonTypes::Value]].asBool();

		if(value)
		{
			solution[segmentationHypotheses_[destId].getDetectionVariable().getOpenGMVariableId()] = 1;
		}
	}

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		size_t detValue = solution[iter->second.getDetectionVariable().getOpenGMVariableId()];

		if(detValue > 0)
		{
			// each variable that has no active incoming links but is active should have its appearance variables set
			if(iter->second.getNumActiveIncomingLinks(solution) == 0)
			{
				if(iter->second.getAppearanceVariable().getOpenGMVariableId() == -1)
				{
					std::stringstream s;
					s << "Segmentation Hypothesis: " << iter->first << " - GT contains appearing variable that has no appearance features set!";
					throw std::runtime_error(s.str());
				}
				else
				{
					solution[iter->second.getAppearanceVariable().getOpenGMVariableId()] = detValue;
				}
			}

			// each variable that has no active outgoing links but is active should have its disappearance variables set
			if(iter->second.getNumActiveOutgoingLinks(solution) == 0)
			{
				if(iter->second.getDisappearanceVariable().getOpenGMVariableId() == -1)
				{
					std::stringstream s;
					s << "Segmentation Hypothesis: " << iter->first << " - GT contains disappearing variable that has no disappearance features set!";
					throw std::runtime_error(s.str());
				}
				else
				{
					solution[iter->second.getDisappearanceVariable().getOpenGMVariableId()] = detValue;
				}
			}
		}
	}

	std::cout << "found gt solution: " << solution << std::endl;

	return solution;
}

bool Model::verifySolution(const Solution& sol) const
{
	std::cout << "Checking solution..." << std::endl;

	bool valid = true;

	// check that all exclusions are obeyed
	for(auto iter = exclusionConstraints_.begin(); iter != exclusionConstraints_.end() ; ++iter)
	{
		if(!iter->verifySolution(sol, segmentationHypotheses_))
		{
			std::cout << "\tFound violated exclusion constraint " << std::endl;
			valid = false;
		}
	}

	// check that flow-conservation + division constraints are satisfied
	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		if(!iter->second.verifySolution(sol))
		{
			std::cout << "\tFound violated flow conservation constraint " << std::endl;
			valid = false;
		}
	}

	return valid;
}

void Model::saveResultToJson(const std::string& filename, const Solution& sol) const
{
	std::ofstream output(filename.c_str());
	if(!output.good())
		throw std::runtime_error("Could not open JSON result file for saving: " + filename);

	Json::Value root;
	Json::Value& linksJson = root[JsonTypeNames[JsonTypes::LinkResults]];

	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		size_t value = sol[iter->second->getVariable().getOpenGMVariableId()];
		if(value > 0)
			linksJson.append(iter->second->toJson(value));
	}

	if(!linksJson.isArray())
		throw std::runtime_error("Cannot save results to non-array JSON entry");

	output << root << std::endl;
}

void Model::toDot(const std::string& filename, const Solution* sol) const
{
	std::ofstream out_file(filename.c_str());

    if(!out_file.good())
    {
        throw std::runtime_error("Could not open file " + filename + " to save graph to");
    }

    out_file << "digraph G {\n";

    // nodes
    for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
		iter->second.toDot(out_file, sol);

	// links
	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
		iter->second->toDot(out_file, sol);

	// exclusions
	for(auto iter = exclusionConstraints_.begin(); iter != exclusionConstraints_.end() ; ++iter)
		iter->toDot(out_file);
	
    out_file << "}";
}

std::vector<std::string> Model::getWeightDescriptions()
{
	std::vector<std::string> descriptions;
	computeNumWeights();

	auto addVariableWeightDescriptions = [&](size_t numWeights, const std::string& name)
	{
		for(size_t f = 0; f < numWeights; ++f)
		{
			// append this variable's state/feature combination description
			std::stringstream d;
			d << name << " - feature " << f;
			descriptions.push_back(d.str());
		}
	};

	addVariableWeightDescriptions(numDetWeights_, "Detection");
	addVariableWeightDescriptions(numDivWeights_, "Division");
	addVariableWeightDescriptions(numAppWeights_, "Appearance");
	addVariableWeightDescriptions(numDisWeights_, "Disappearance");
	addVariableWeightDescriptions(numLinkWeights_, "Link");

	return descriptions;
}

} // end namespace mht