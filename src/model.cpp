#include "model.h"
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <sstream>

// include the LPDef symbols only once!
#undef OPENGM_LPDEF_NO_SYMBOLS
#include <opengm/inference/auxiliary/lpdef.hxx>

#include <opengm/inference/lpgurobi2.hxx>
#include <opengm/inference/lpgurobi.hxx>

#include <opengm/learning/struct-max-margin.hxx>

using namespace helpers;

namespace mht
{

size_t Model::computeNumWeights()
{
	// only compute if it wasn't initialized yet
	if(numDetWeights_ == 0)
	{
		int numDetWeights = -1;
		int numDivWeights = -1;
		int numAppWeights = -1;
		int numDisWeights = -1;
		int numExternalDivWeights = -1;
		int numLinkWeights = -1;

		auto checkNumWeights = [&](const Variable& var, int& previousNumWeights, const std::string& name)
		{
			int numWeights = var.getNumWeights(settings_->statesShareWeights_);
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

		for(auto iter = divisionHypotheses_.begin(); iter != divisionHypotheses_.end() ; ++iter)
		{
			checkNumWeights(iter->second->getVariable(), numExternalDivWeights, "External Divisions");
		}

		for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
		{
			if(numLinkWeights < 0)
				numLinkWeights = iter->second->getVariable().getNumWeights(settings_->statesShareWeights_);
			else
				if(iter->second->getVariable().getNumWeights(settings_->statesShareWeights_) != numLinkWeights)
					throw std::runtime_error("Links do not have the same number of features!");
		}

		// we don't want -1 weights
		numDetWeights_ = std::max((int)0, numDetWeights);
		numDivWeights_ = std::max((int)0, numDivWeights);
		numAppWeights_ = std::max((int)0, numAppWeights);
		numDisWeights_ = std::max((int)0, numDisWeights);
		numExternalDivWeights_ = std::max((int)0, numExternalDivWeights);
		numLinkWeights_ = std::max((int)0, numLinkWeights);

		if(numDivWeights_ != 0 && numExternalDivWeights_ != 0)
			throw std::runtime_error("Model cannot contain divisions within detection nodes and externally at the same time!");

		// std::cout << "need " << numDetWeights_ << " detection weights" << std::endl;
		// std::cout << "need " << numDivWeights_ << " division weights" << std::endl;
		// std::cout << "need " << numAppWeights_ << " appearance weights" << std::endl;
		// std::cout << "need " << numDisWeights_ << " disappearance weights" << std::endl;
		// std::cout << "need " << numLinkWeights_ << " link weights" << std::endl;
	}

	return numDetWeights_ + numDivWeights_ + numAppWeights_ + numDisWeights_ + numExternalDivWeights_ + numLinkWeights_;
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
		iter->second->addToOpenGMModel(model_, weights, settings_->statesShareWeights_, linkWeightIds);
	}

	std::vector<size_t> detWeightIds(numDetWeights_);
	std::iota(detWeightIds.begin(), detWeightIds.end(), numLinkWeights_); // fill with increasing values starting at the next valid index

	std::vector<size_t> divWeightIds(numDivWeights_);
	std::iota(divWeightIds.begin(), divWeightIds.end(), numLinkWeights_ + numDetWeights_);

	std::vector<size_t> appWeightIds(numAppWeights_);
	std::iota(appWeightIds.begin(), appWeightIds.end(), numLinkWeights_ + numDetWeights_ + numDivWeights_);

	std::vector<size_t> disWeightIds(numDisWeights_);
	std::iota(disWeightIds.begin(), disWeightIds.end(), numLinkWeights_ + numDetWeights_ + numDivWeights_ + numAppWeights_);

	std::vector<size_t> externalDivWeightIds(numExternalDivWeights_);
	std::iota(externalDivWeightIds.begin(), externalDivWeightIds.end(), numLinkWeights_ + numDetWeights_ + numDivWeights_ + numAppWeights_ + numDisWeights_);

	for(auto iter = divisionHypotheses_.begin(); iter != divisionHypotheses_.end() ; ++iter)
	{
		iter->second->addToOpenGMModel(model_, weights, settings_->statesShareWeights_, externalDivWeightIds);
	}

	for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
	{
		iter->second.addToOpenGMModel(model_, weights, settings_, detWeightIds, divWeightIds, appWeightIds, disWeightIds);
	}

	for(auto iter = exclusionConstraints_.begin(); iter != exclusionConstraints_.end() ; ++iter)
	{
		iter->addToOpenGMModel(model_, segmentationHypotheses_);
	}

	size_t numIndicatorVars = 0;
	for(size_t i = 0; i < model_.numberOfVariables(); i++)
	{
		numIndicatorVars += model_.numberOfLabels(i);
	}
	std::cout << "Model has " << numIndicatorVars << " indicator variables" << std::endl;
}

Solution Model::infer(const std::vector<ValueType>& weights, bool withIntegerConstraints)
{
	// use weights that were given
	WeightsType weightObject(computeNumWeights());
	assert(weights.size() == weightObject.numberOfWeights());
	if(weights.size() != weightObject.numberOfWeights())
	{
		std::cout << "Provided length of vector with initial weights has wrong length!" << std::endl;
		throw std::runtime_error("Provided length of vector with initial weights has wrong length!");
	}
	for(size_t i = 0; i < weights.size(); i++)
		weightObject.setWeight(i, weights[i]);
	initializeOpenGMModel(weightObject);

	if(withIntegerConstraints)
	{

		std::cout << "Using gurobi optimizer" << std::endl;
		typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;

		OptimizerType::Parameter optimizerParam;
		optimizerParam.relaxation_ = OptimizerType::Parameter::TightPolytope;
		optimizerParam.verbose_ = settings_->optimizerVerbose_;
		optimizerParam.useSoftConstraints_ = false;
		optimizerParam.integerConstraintNodeVar_ = true;
		optimizerParam.epGap_ = settings_->optimizerEpGap_;
		optimizerParam.numberOfThreads_ = settings_->optimizerNumThreads_;

		OptimizerType optimizer(model_, optimizerParam);

		Solution solution(model_.numberOfVariables());
		OptimizerType::VerboseVisitorType optimizerVisitor;
		optimizer.infer(optimizerVisitor);
		optimizer.arg(solution);
		std::cout << "solution has energy: " << optimizer.value() << std::endl;
		foundSolutionValue_ = optimizer.value();
		return solution;
	}
	else
	{
		std::cout << "Using gurobi optimizer" << std::endl;
		typedef opengm::LPGurobi<GraphicalModelType, opengm::Minimizer> OptimizerType;

		OptimizerType::Parameter optimizerParam;
		optimizerParam.verbose_ = settings_->optimizerVerbose_;
		optimizerParam.integerConstraint_ = true;
		optimizerParam.epGap_ = settings_->optimizerEpGap_;
		optimizerParam.numberOfThreads_ = settings_->optimizerNumThreads_;

		OptimizerType optimizer(model_, optimizerParam);

		Solution solution(model_.numberOfVariables());
		OptimizerType::VerboseVisitorType optimizerVisitor;
		optimizer.infer(optimizerVisitor);
		optimizer.arg(solution);
		
		for(size_t i = 0; i < solution.size(); i++)
        {
            opengm::IndependentFactor<double, size_t, size_t> values;
            optimizer.variable(i, values);
            std::cout << "Variable " << i << ": ";
            for(size_t state = 0; state < model_.numberOfLabels(i); state++)
            {
                std::cout << "(" << state << ")=" << values(state) << " ";
            }
            std::cout << std::endl;
        }
		
		std::cout << "solution has energy: " << optimizer.value() << std::endl;
		foundSolutionValue_ = optimizer.value();
		return solution;
	}
}

std::vector<ValueType> Model::learn()
{
	std::vector<helpers::ValueType> weights(computeNumWeights(), 0);
	return learn(weights);
}

std::vector<ValueType> Model::learn(const std::vector<helpers::ValueType>& weights)
{
	// prepare OpenGM for learning
	DatasetType dataset;
	WeightsType initialWeights(computeNumWeights());

	if(weights.size() != computeNumWeights())
	{
		std::cout << "Provided length of vector with initial weights has wrong length!" << std::endl;
		throw std::runtime_error("Provided length of vector with initial weights has wrong length!");
	}
	for(size_t i = 0; i < weights.size(); ++i)
	{
		initialWeights.setWeight(i, weights[i]);
	}
	
	dataset.setWeights(initialWeights);
	initializeOpenGMModel(dataset.getWeights());

	// load GT from subclass-specified method
	Solution gt = getGroundTruth();

	dataset.pushBackInstance(model_, gt);
	
	std::cout << "Done setting up dataset, creating learner" << std::endl;
	opengm::learning::StructMaxMargin<DatasetType>::Parameter learnerParam;
	learnerParam.optimizerParameter_.lambda = 1.00;
	learnerParam.optimizerParameter_.nonNegativeWeights = settings_->nonNegativeWeightsOnly_;
	opengm::learning::StructMaxMargin<DatasetType> learner(dataset, learnerParam);

	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
	
	OptimizerType::Parameter optimizerParam;
	optimizerParam.integerConstraintNodeVar_ = true;
	optimizerParam.relaxation_ = OptimizerType::Parameter::TightPolytope;
	optimizerParam.verbose_ = settings_->optimizerVerbose_;
	optimizerParam.useSoftConstraints_ = false;
	optimizerParam.epGap_ = settings_->optimizerEpGap_;
	optimizerParam.numberOfThreads_ = settings_->optimizerNumThreads_;

	std::cout << "Calling learn()..." << std::endl;
	learner.learn<OptimizerType>(optimizerParam); 
	std::cout << "extracting weights" << std::endl;
	const WeightsType& finalWeights = learner.getWeights();
	std::vector<double> resultWeights;
	for(size_t i = 0; i < finalWeights.numberOfWeights(); ++i)
		resultWeights.push_back(finalWeights.getWeight(i));
	return resultWeights;
}

double Model::evaluateSolution(const Solution& sol) const
{
	return model_.evaluate(sol);
}

double Model::getLastSolutionValue() const
{
	return foundSolutionValue_;
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
		if(!iter->second.verifySolution(sol, settings_))
		{
			std::cout << "\tFound violated flow conservation constraint " << std::endl;
			valid = false;
		}
	}

	return valid;
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

	// divisions
	for(auto iter = divisionHypotheses_.begin(); iter != divisionHypotheses_.end() ; ++iter)
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

	addVariableWeightDescriptions(numLinkWeights_, "Link");
	addVariableWeightDescriptions(numDetWeights_, "Detection");
	addVariableWeightDescriptions(numDivWeights_, "Division");
	addVariableWeightDescriptions(numAppWeights_, "Appearance");
	addVariableWeightDescriptions(numDisWeights_, "Disappearance");
	addVariableWeightDescriptions(numExternalDivWeights_, "External Division");

	return descriptions;
}

void Model::deduceAppearanceDisappearanceStates(helpers::Solution& solution)
{
	// deduce states of appearance and disappearance variables
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
}

} // end namespace mht
