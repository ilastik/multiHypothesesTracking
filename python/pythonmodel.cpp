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

void PythonModel::readExclusionConstraints(list& entry)
{
	std::vector<helpers::IdLabelType> ids;
    for(size_t i = 0; (int)i < len(entry); i++)
    {
        ids.push_back(extract<IdLabelType>(entry[i]));
    }

    // add to list
    exclusionConstraints_.push_back(ExclusionConstraint(ids));
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
		if(settings.has_key(JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]))
			settings_->requireSeparateChildrenOfDivision_ = extract<bool>(settings[JsonTypeNames[JsonTypes::RequireSeparateChildrenOfDivision]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerEpGap]))
			settings_->optimizerEpGap_ = extract<bool>(settings[JsonTypeNames[JsonTypes::OptimizerEpGap]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerVerbose]))
			settings_->optimizerVerbose_ = extract<bool>(settings[JsonTypeNames[JsonTypes::OptimizerVerbose]]);
		if(settings.has_key(JsonTypeNames[JsonTypes::OptimizerNumThreads]))
			settings_->optimizerNumThreads_ = extract<bool>(settings[JsonTypeNames[JsonTypes::OptimizerNumThreads]]);
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
	if(graphDict.has_key(JsonTypeNames[JsonTypes::Divisions]))
	{
		list divisionHypotheses = extract<list>(graphDict[JsonTypeNames[JsonTypes::Divisions]]);
		for(size_t i = 0; (int)i < len(divisionHypotheses); i++)
		{
			dict jsonHyp = extract<dict>(divisionHypotheses[i]);
			readDivisionHypothesis(jsonHyp);
		}
	}

	// // read exclusion constraints between detections
	// list exclusions = extract<list>(graphDict[JsonTypeNames[JsonTypes::Exclusions]]);
	// std::cout << "\tcontains " << exclusions.size() << " exclusions" << std::endl;
	// for(size_t i = 0; i < (int)exclusions.size(); i++)
	// {
	// 	const Json::Value jsonExc = exclusions[i];
	//  readExclusionConstraints(jsonExc);
	// }
	if(graphDict.has_key(JsonTypeNames[JsonTypes::Exclusions]) && len(graphDict[JsonTypeNames[JsonTypes::Exclusions]]) > 0)
		throw std::runtime_error("FlowSolver cannot deal with exclusion constraints yet!");
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
	throw std::runtime_error("Loading the ground truth from python is not implemented yet!");
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

