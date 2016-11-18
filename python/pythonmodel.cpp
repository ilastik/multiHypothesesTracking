#include "pythonmodel.h"
#include <assert.h>
#include <fstream>

using namespace boost::python;
using namespace helpers;

namespace mht
{

void PythonModel::readLinkingHypothesis(dict& entry)
{
	if(!entry.has_key(JsonTypeNames[JsonTypes::SrcId]))
        throw std::runtime_error("Python dict entry for LinkingHypothesis is invalid: missing srcId"); 
    if(!entry.has_key(JsonTypeNames[JsonTypes::DestId]))
        throw std::runtime_error("Python dict entry for LinkingHypothesis is invalid: missing destId");
    if(!entry.has_key(JsonTypeNames[JsonTypes::Features]))
        throw std::runtime_error("Python dict entry for LinkingHypothesis is invalid: missing features");

    helpers::IdLabelType srcId = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::SrcId]]);
    helpers::IdLabelType destId = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::DestId]]);

    // get transition features
    helpers::StateFeatureVector features = extractFeatures(entry, JsonTypes::Features);

    // add to list
    std::shared_ptr<LinkingHypothesis> hyp = std::make_shared<LinkingHypothesis>(srcId, destId, features);
    std::pair<helpers::IdLabelType, helpers::IdLabelType> ids = std::make_pair(srcId, destId);
    hyp->registerWithSegmentations(segmentationHypotheses_);
    linkingHypotheses_[ids] = hyp;
}

void PythonModel::readSegmentationHypothesis(dict& entry)
{
	if(!entry.has_key(JsonTypeNames[JsonTypes::Id]))
		throw std::runtime_error("Cannot read detection hypothesis without Id!");
	if(!entry.has_key(JsonTypeNames[JsonTypes::Features]))
		throw std::runtime_error("Cannot read detection hypothesis without features!");

	IdLabelType id = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Id]]);

	StateFeatureVector detectionFeatures = extractFeatures(entry, JsonTypes::Features);
    StateFeatureVector divisionFeatures;
    StateFeatureVector appearanceFeatures;
    StateFeatureVector disappearanceFeatures;

	if(entry.has_key(JsonTypeNames[JsonTypes::DivisionFeatures]))
        divisionFeatures = extractFeatures(entry, JsonTypes::DivisionFeatures);

    // read appearance and disappearance if present
    if(entry.has_key(JsonTypeNames[JsonTypes::AppearanceFeatures]))
        appearanceFeatures = extractFeatures(entry, JsonTypes::AppearanceFeatures);
    
    if(entry.has_key(JsonTypeNames[JsonTypes::DisappearanceFeatures]))
        disappearanceFeatures = extractFeatures(entry, JsonTypes::DisappearanceFeatures);

    // add to list
    SegmentationHypothesis hyp(id, detectionFeatures, divisionFeatures, appearanceFeatures, disappearanceFeatures);
    segmentationHypotheses_[id] = hyp;
}

void PythonModel::readDivisionHypothesis(dict& entry)
{
    if(!entry.has_key(JsonTypeNames[JsonTypes::Parent]))
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: missing srcId"); 
    if(!entry.has_key(JsonTypeNames[JsonTypes::Children]) || len(entry[JsonTypeNames[JsonTypes::Children]]) != 2)
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: must have two children as array");
    if(!entry.has_key(JsonTypeNames[JsonTypes::Features]))
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: missing features");

    IdLabelType parentId = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Parent]]);
    std::vector<helpers::IdLabelType> childrenIds;

    list children = extract<list>(entry[JsonTypeNames[JsonTypes::Children]]);
    for(size_t i = 0; (int)i < len(children); ++i)
    {
        childrenIds.push_back(extract<IdLabelType>(children[i]));
    }

    // always use ordered list of children!
    std::sort(childrenIds.begin(), childrenIds.end());

    // get transition features
    StateFeatureVector features = extractFeatures(entry, JsonTypes::Features);

    // add to list
    std::shared_ptr<DivisionHypothesis> hyp = std::make_shared<DivisionHypothesis>(parentId, childrenIds, features);
    hyp->registerWithSegmentations(segmentationHypotheses_);
    auto ids = std::make_tuple(parentId, childrenIds[0], childrenIds[1]);
    divisionHypotheses_[ids] = hyp;
}

void PythonModel::readExclusionConstraint(list& entry)
{
	std::vector<helpers::IdLabelType> ids;
    for(size_t i = 0; (int)i < len(entry); i++)
    {
        ids.push_back(extract<IdLabelType>(entry[i]));
    }

	if(ids.size() < 2)
    {
        // std::cout << "Ignoring exclusion constraint with less than two elements" << std::endl;
        return;
    }

    // add to list
    exclusionConstraints_.push_back(ExclusionConstraint(ids));
}

void PythonModel::setPythonGt(boost::python::dict& gtDict)
{
	groundTruthDict_ = gtDict;
}

void PythonModel::readFromPython(dict& graphDict)
{
	// get flag whether states should share weights or not
	settings_ = std::make_shared<helpers::Settings>();

	if(graphDict.has_key(JsonTypeNames[JsonTypes::Settings]))
	{
		dict settings = extract<dict>(graphDict[JsonTypeNames[JsonTypes::Settings]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::StatesShareWeights]))
			settings_->statesShareWeights_ = extract<bool>(settings[JsonTypeNames[JsonTypes::StatesShareWeights]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::AllowPartialMergerAppearance]))
			settings_->allowPartialMergerAppearance_ = extract<bool>(settings[JsonTypeNames[JsonTypes::AllowPartialMergerAppearance]]);
        if(settings.has_key(JsonTypeNames[JsonTypes::AllowLengthOneTracks]))
			settings_->allowLengthOneTracks_ = extract<bool>(settings[JsonTypeNames[JsonTypes::AllowLengthOneTracks]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]))
			settings_->requireSeparateChildrenOfDivision_ = extract<bool>(settings[JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerEpGap]))
			settings_->optimizerEpGap_ = extract<double>(settings[JsonTypeNames[JsonTypes::OptimizerEpGap]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerVerbose]))
			settings_->optimizerVerbose_ = extract<bool>(settings[JsonTypeNames[JsonTypes::OptimizerVerbose]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerNumThreads]))
			settings_->optimizerNumThreads_ = extract<int>(settings[JsonTypeNames[JsonTypes::OptimizerNumThreads]]);
	}
	else
	{
		std::cout << "WARNING: Python Graph Dict has no settings specified, using defaults" << std::endl;
	}

	settings_->print();

	list segmentationHypotheses = extract<list>(graphDict[JsonTypeNames[JsonTypes::Segmentations]]);
	list linkingHypotheses = extract<list>(graphDict[JsonTypeNames[JsonTypes::Links]]);
	
	// ------------------------------------------------------------------------------
	// read segmentation hypotheses and add to flowgraph
	std::cout << "\tcontains " << len(segmentationHypotheses) << " segmentation hypotheses" << std::endl;
	
	for(size_t i = 0; (int)i < len(segmentationHypotheses); i++)
	{
		dict jsonHyp = extract<dict>(segmentationHypotheses[i]);
		readSegmentationHypothesis(jsonHyp);
	}

	// read linking hypotheses
	std::cout << "\tcontains " << len(linkingHypotheses) << " linking hypotheses" << std::endl;
	for(size_t i = 0; (int)i < len(linkingHypotheses); i++)
	{
		dict jsonHyp = extract<dict>(linkingHypotheses[i]);
		readLinkingHypothesis(jsonHyp);
	}

	// read divisions
	if(graphDict.has_key(JsonTypeNames[JsonTypes::Divisions]) && len(graphDict[JsonTypeNames[JsonTypes::Divisions]]) > 0)
	{
		list divisionHypotheses = extract<list>(graphDict[JsonTypeNames[JsonTypes::Divisions]]);
		for(size_t i = 0; (int)i < len(divisionHypotheses); i++)
		{
			dict jsonHyp = extract<dict>(divisionHypotheses[i]);
			readDivisionHypothesis(jsonHyp);
		}
	}

	// read exclusion constraints between detections
	if(graphDict.has_key(JsonTypeNames[JsonTypes::Exclusions]) && len(graphDict[JsonTypeNames[JsonTypes::Exclusions]]) > 0)
	{
		list exclusions = extract<list>(graphDict[JsonTypeNames[JsonTypes::Exclusions]]);
		std::cout << "\tcontains " << len(exclusions) << " exclusions" << std::endl;
		for(size_t i = 0; (int)i < len(exclusions); i++)
		{
			list exclusionSet = extract<list>(exclusions[i]);
			readExclusionConstraint(exclusionSet);
		}
	}
}

dict PythonModel::saveWeightsToPython(const std::vector<double>& weights) const
{
	dict result;
	list weightNumbers;

	for(double w : weights)
		weightNumbers.append(w);

	result[JsonTypeNames[JsonTypes::Weights]] = weightNumbers;
	return result;
}

dict PythonModel::saveResultToPython(const Solution& sol) const
{
	list detectionResults;
	list linkResults;
	list divisionResults;

	// save links
	for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
	{
		// store in extra list
		size_t value = sol[iter->second->getVariable().getOpenGMVariableId()];
		if(value > 0)
		linkResults.append(linkToPython(iter->second, value));
	}

	// save divisions
    for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
    {
        if(iter->second.getDivisionVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second.getDivisionVariable().getOpenGMVariableId()];
            if(value > 0)
                divisionResults.append(divisionToPython(iter->second, value));
        }
    }
    for(auto iter = divisionHypotheses_.begin(); iter != divisionHypotheses_.end() ; ++iter)
    {
        if(iter->second->getVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second->getVariable().getOpenGMVariableId()];
            if(value > 0)
                divisionResults.append(divisionToPython(iter->second, value));
        }
    }

    // save detections
    for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
    {
        if(iter->second.getDetectionVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second.getDetectionVariable().getOpenGMVariableId()];
            if(value > 0)
                detectionResults.append(detectionToPython(iter->second, value));
        }
    }

	dict result;
	result[JsonTypeNames[JsonTypes::DetectionResults]] = detectionResults;
	result[JsonTypeNames[JsonTypes::LinkResults]] = linkResults;
	result[JsonTypeNames[JsonTypes::DivisionResults]] = divisionResults;
	return result;
}

dict PythonModel::linkToPython(const std::shared_ptr<LinkingHypothesis>& link, size_t state) const
{
	dict linkRes;
	linkRes[JsonTypeNames[JsonTypes::SrcId]] = link->getSrcId();
    linkRes[JsonTypeNames[JsonTypes::DestId]] = link->getDestId();
    linkRes[JsonTypeNames[JsonTypes::Value]] = (unsigned int)state;
	return linkRes;
}

dict PythonModel::divisionToPython(const std::shared_ptr<DivisionHypothesis>& division, size_t state) const
{
	dict divRes;
	divRes[JsonTypeNames[JsonTypes::Id]] = division->getParentId();
	divRes[JsonTypeNames[JsonTypes::Value]] = state;
	return divRes;
}

dict PythonModel::divisionToPython(const SegmentationHypothesis& segmentation, size_t value) const
{
	dict divRes;
	divRes[JsonTypeNames[JsonTypes::Id]] = segmentation.getId();
	divRes[JsonTypeNames[JsonTypes::Value]] = value;
	return divRes;
}

dict PythonModel::detectionToPython(const SegmentationHypothesis& segmentation, size_t value) const
{
	dict detRes;
	detRes[JsonTypeNames[JsonTypes::Id]] = segmentation.getId();
	detRes[JsonTypeNames[JsonTypes::Value]] = value;
	return detRes;
}


helpers::StateFeatureVector PythonModel::extractFeatures(boost::python::dict& entry, JsonTypes type)
{
	StateFeatureVector stateFeatVec;
	if(!entry.has_key(JsonTypeNames[type]))
		throw std::runtime_error("Could not find dict entry for " + JsonTypeNames[type]);

	list featuresPerState = extract<list>(entry[JsonTypeNames[type]]);

	if(len(featuresPerState) == 0)
		throw std::runtime_error("Features may not be empty for " + JsonTypeNames[type]);

	// std::cout << "\tReading features for: " << JsonTypeNames[type] << std::endl;

	// get the features per state
	for(size_t i = 0; (int)i < len(featuresPerState); i++)
	{
		// get features for the specific state
		FeatureVector featVec;
		list featuresForState = extract<list>(featuresPerState[i]);

		
		if(len(featuresForState) ==  0)
			throw std::runtime_error("Features for state may not be empty for " + JsonTypeNames[type]);

		for(size_t j = 0; (int)j < len(featuresForState); j++)
		{
			featVec.push_back(extract<ValueType>(featuresForState[j]));
		}

		// std::cout << "\t\tfound " << featVec.size() << " features for state " << i << std::endl;

		stateFeatVec.push_back(featVec);
	}

	return stateFeatVec;
}

Solution PythonModel::getGroundTruth()
{
	if(!model_.numberOfVariables() > 0)
        throw std::runtime_error("OpenGM model must be initialized before reading a ground truth!");
	
	list linkingResults = extract<list>(groundTruthDict_[JsonTypeNames[JsonTypes::LinkResults]]);
    std::cout << "\tcontains " << len(linkingResults) << " linking annotations" << std::endl;

    // create a solution vector that holds a value for each segmentation / detection / link
    Solution solution(model_.numberOfVariables(), 0);

    // first set all links and the respective source nodes to active
    for(int i = 0; i < len(linkingResults); ++i)
    {
		dict entry = extract<dict>(linkingResults[i]);
		if(!entry.has_key(JsonTypeNames[JsonTypes::SrcId]))
			throw std::runtime_error("Python dict entry for LinkingResult is invalid: missing srcId"); 
		if(!entry.has_key(JsonTypeNames[JsonTypes::DestId]))
			throw std::runtime_error("Python dict entry for LinkingResult is invalid: missing destId");
		if(!entry.has_key(JsonTypeNames[JsonTypes::Value]))
			throw std::runtime_error("Python dict entry for LinkingResult is invalid: missing value");

		helpers::IdLabelType srcId = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::SrcId]]);
		helpers::IdLabelType destId = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::DestId]]);
        size_t value = extract<size_t>(entry[JsonTypeNames[JsonTypes::Value]]);

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
        }
    }

    // read segmentation variables
    list segmentationResults = extract<list>(groundTruthDict_[JsonTypeNames[JsonTypes::DetectionResults]]);
    std::cout << "\tcontains " << len(segmentationResults) << " detection annotations" << std::endl;
    for(int i = 0; i < len(segmentationResults); ++i)
    {
        dict entry = extract<dict>(segmentationResults[i]);

		if(!entry.has_key(JsonTypeNames[JsonTypes::Id]))
			throw std::runtime_error("Cannot read detection result without Id!");
		if(!entry.has_key(JsonTypeNames[JsonTypes::Value]))
			throw std::runtime_error("Cannot read detection result without value!");

		IdLabelType id = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Id]]);
        size_t value = extract<size_t>(entry[JsonTypeNames[JsonTypes::Value]]);

        solution[segmentationHypotheses_[id].getDetectionVariable().getOpenGMVariableId()] = value;
    }

    // read division variable states
	list divisionResults = extract<list>(groundTruthDict_[JsonTypeNames[JsonTypes::DivisionResults]]);
    std::cout << "\tcontains " << len(divisionResults) << " division annotations" << std::endl;
    for(int i = 0; i < len(divisionResults); ++i)
    {
		dict entry = extract<dict>(divisionResults[i]);
		if(!entry.has_key(JsonTypeNames[JsonTypes::Value]))
        	throw std::runtime_error("JSON entry for DivisionResult is invalid: missing Value");
        bool value = extract<bool>(entry[JsonTypeNames[JsonTypes::Value]]);

        if(value)
        {
            // depending on internal or external division node setup, handle both gracefully!
            helpers::IdLabelType id;
            if(entry.has_key(JsonTypeNames[JsonTypes::Id]))
            {
                // id is given for internal division
                id = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Id]]);
            }
            else
            {
                // parent is given for external
                if(!entry.has_key(JsonTypeNames[JsonTypes::Parent]))
                    throw std::runtime_error("Invalid configuration of a JSON division result entry");

                id = extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Parent]]);
            }

            if(solution[segmentationHypotheses_[id].getDetectionVariable().getOpenGMVariableId()] == 0)
            {
                // in any case the parent must be active!
                std::stringstream error;
                error << "Cannot activate division of node " << id << " that is not active!";
                throw std::runtime_error(error.str());
            }

            if(entry.has_key(JsonTypeNames[JsonTypes::Id]))
            {
                if(segmentationHypotheses_[id].getDivisionVariable().getOpenGMVariableId() < 0)
                {
                    std::stringstream error;
                    error << "Trying to set division of " << id << " active but the variable had no division features!";
                    throw std::runtime_error(error.str());
                }
                // internal if id is given AND there is a opengm variable for the internal division
                solution[segmentationHypotheses_[id].getDivisionVariable().getOpenGMVariableId()] = 1;
            }
            else if(entry.has_key(JsonTypeNames[JsonTypes::Parent]) && entry.has_key(JsonTypeNames[JsonTypes::Children]))
            {
				list children = extract<list>(entry[JsonTypeNames[JsonTypes::Children]]);

                if(len(children) != 2)
                {
                    std::stringstream error;
                    error << "Activating an external division of parent " << id << " requires two children!";
                    throw std::runtime_error(error.str());
                }

                std::vector<IdLabelType> childrenIds;
                for(int i = 0; i < len(children); ++i)
                {
                    childrenIds.push_back(extract<IdLabelType>(children[i]));
                }

                // always use ordered list of children!
                std::sort(childrenIds.begin(), childrenIds.end());

                DivisionHypothesis::IdType idx = std::make_tuple((IdLabelType)extract<IdLabelType>(entry[JsonTypeNames[JsonTypes::Parent]]),
                                                                childrenIds[0],
                                                                childrenIds[1]);

                if(divisionHypotheses_.find(idx) == divisionHypotheses_.end())
                {
                    std::stringstream error;
                    error << "Parent " << id << " does not have division to " << childrenIds[0] << " and " << childrenIds[1] << " to set active!";
                    throw std::runtime_error(error.str());
                }

                std::cout << "Setting external division to active! " << std::endl;
                auto divHyp = divisionHypotheses_[idx];
                solution[divHyp->getVariable().getOpenGMVariableId()] = 1;
            }
            else
            {
                std::stringstream error;
                error << "Trying to set division of " << id << " active but the variable had no division features and no external divisions!";
                throw std::runtime_error(error.str());
            }
                
        }
    }

	deduceAppearanceDisappearanceStates(solution);

    return solution;
}

helpers::FeatureVector readWeightsFromPython(boost::python::dict& weightsDict)
{
	list weightsList = extract<list>(weightsDict[JsonTypeNames[JsonTypes::Weights]]);
	FeatureVector weights;
	for(size_t i = 0; (int)i < len(weightsList); i++)
	{
		weights.push_back(extract<ValueType>(weightsList[i]));
	}
	return weights;
}
	
} // end namespace mht

