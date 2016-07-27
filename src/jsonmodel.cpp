#include "jsonmodel.h"
#include <json/json.h>
#include <fstream>
#include <stdexcept>
#include <numeric>
#include <sstream>
#include <tuple>

using namespace helpers;

namespace mht
{

void JsonModel::readLinkingHypothesis(const Json::Value& entry)
{
    if(!entry.isObject())
        throw std::runtime_error("Cannot extract LinkingHypothesis from non-object JSON entry");
    if(!entry.isMember(JsonTypeNames[JsonTypes::SrcId]) || !entry[JsonTypeNames[JsonTypes::SrcId]].isLabelType())
        throw std::runtime_error("JSON entry for LinkingHypothesis is invalid: missing srcId"); 
    if(!entry.isMember(JsonTypeNames[JsonTypes::DestId]) || !entry[JsonTypeNames[JsonTypes::DestId]].isLabelType())
        throw std::runtime_error("JSON entry for LinkingHypothesis is invalid: missing destId");
    if(!entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
        throw std::runtime_error("JSON entry for LinkingHypothesis is invalid: missing features");

    helpers::IdLabelType srcId = entry[JsonTypeNames[JsonTypes::SrcId]].asLabelType();
    helpers::IdLabelType destId = entry[JsonTypeNames[JsonTypes::DestId]].asLabelType();

    // get transition features
    helpers::StateFeatureVector features = extractFeatures(entry, JsonTypes::Features);

    // add to list
    std::shared_ptr<LinkingHypothesis> hyp = std::make_shared<LinkingHypothesis>(srcId, destId, features);
    std::pair<helpers::IdLabelType, helpers::IdLabelType> ids = std::make_pair(srcId, destId);
    hyp->registerWithSegmentations(segmentationHypotheses_);
    linkingHypotheses_[ids] = hyp;
}

void JsonModel::readSegmentationHypothesis(const Json::Value& entry)
{
    if(!entry.isObject())
        throw std::runtime_error("Cannot extract SegmentationHypothesis from non-object JSON entry");
    if(!entry.isMember(JsonTypeNames[JsonTypes::Id]) || !entry[JsonTypeNames[JsonTypes::Id]].isLabelType() 
        || !entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
        throw std::runtime_error("JSON entry for SegmentationHytpohesis is invalid");

    StateFeatureVector detectionFeatures;
    StateFeatureVector divisionFeatures;
    StateFeatureVector appearanceFeatures;
    StateFeatureVector disappearanceFeatures;

    IdLabelType id = entry[JsonTypeNames[JsonTypes::Id]].asLabelType();

    detectionFeatures = extractFeatures(entry, JsonTypes::Features);

    if(entry.isMember(JsonTypeNames[JsonTypes::DivisionFeatures]))
        divisionFeatures = extractFeatures(entry, JsonTypes::DivisionFeatures);

    // read appearance and disappearance if present
    if(entry.isMember(JsonTypeNames[JsonTypes::AppearanceFeatures]))
        appearanceFeatures = extractFeatures(entry, JsonTypes::AppearanceFeatures);
    
    if(entry.isMember(JsonTypeNames[JsonTypes::DisappearanceFeatures]))
        disappearanceFeatures = extractFeatures(entry, JsonTypes::DisappearanceFeatures);

    // add to list
    SegmentationHypothesis hyp(id, detectionFeatures, divisionFeatures, appearanceFeatures, disappearanceFeatures);
    segmentationHypotheses_[id] = hyp;
}

void JsonModel::readDivisionHypothesis(const Json::Value& entry)
{
    if(!entry.isObject())
        throw std::runtime_error("Cannot extract DivisionHypothesis from non-object JSON entry");
    if(!entry.isMember(JsonTypeNames[JsonTypes::Parent]) || !entry[JsonTypeNames[JsonTypes::Parent]].isLabelType())
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: missing srcId"); 
    if(!entry.isMember(JsonTypeNames[JsonTypes::Children]) || !entry[JsonTypeNames[JsonTypes::Children]].isArray() 
            || entry[JsonTypeNames[JsonTypes::Children]].size() != 2)
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: must have two children as array");
    if(!entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
        throw std::runtime_error("JSON entry for DivisionHypothesis is invalid: missing features");

    IdLabelType parentId = entry[JsonTypeNames[JsonTypes::Parent]].asLabelType();
    std::vector<helpers::IdLabelType> childrenIds;

    const Json::Value children = entry[JsonTypeNames[JsonTypes::Children]];
    for(int i = 0; i < children.size(); ++i)
    {
        childrenIds.push_back(children[i].asLabelType());
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

void JsonModel::readExclusionConstraints(const Json::Value& entry)
{
    if(!entry.isArray())
        throw std::runtime_error("Cannot extract Constraint from non-array JSON entry");

    std::vector<helpers::IdLabelType> ids;
    for(int i = 0; i < (int)entry.size(); i++)
    {
        ids.push_back(entry[i].asLabelType());
    }

    if(ids.size() < 2)
    {
        // std::cout << "Ignoring exclusion constraint with less than two elements" << std::endl;
        return;
    }

    // add to list
    exclusionConstraints_.push_back(ExclusionConstraint(ids));
}

void JsonModel::readFromJson(const std::string& filename)
{
    std::ifstream input(filename.c_str());
    if(!input.good())
        throw std::runtime_error("Could not open JSON model file " + filename);

    Json::Value root;
    input >> root;

    // read settings:
    Json::Value settingsJson;
    if(!root.isMember(JsonTypeNames[JsonTypes::Settings]))
        std::cout << "WARNING: JSON JsonModel has no settings specified, using defaults" << std::endl;
    else
        settingsJson = root[JsonTypeNames[JsonTypes::Settings]];
    settings_ = std::make_shared<helpers::Settings>(settingsJson);
    settings_->print();

    // read segmentation hypotheses
    const Json::Value segmentationHypotheses = root[JsonTypeNames[JsonTypes::Segmentations]];
    std::cout << "\tcontains " << segmentationHypotheses.size() << " segmentation hypotheses" << std::endl;
    
    for(int i = 0; i < (int)segmentationHypotheses.size(); i++)
    {
        const Json::Value jsonHyp = segmentationHypotheses[i];
        readSegmentationHypothesis(jsonHyp);
    }

    // read linking hypotheses
    const Json::Value linkingHypotheses = root[JsonTypeNames[JsonTypes::Links]];
    std::cout << "\tcontains " << linkingHypotheses.size() << " linking hypotheses" << std::endl;
    for(int i = 0; i < (int)linkingHypotheses.size(); i++)
    {
        const Json::Value jsonHyp = linkingHypotheses[i];
        readLinkingHypothesis(jsonHyp);
    }

    // read division hypotheses
    const Json::Value divisionHypotheses = root[JsonTypeNames[JsonTypes::Divisions]];
    std::cout << "\tcontains " << divisionHypotheses.size() << " division hypotheses" << std::endl;
    for(int i = 0; i < (int)divisionHypotheses.size(); i++)
    {
        const Json::Value jsonHyp = divisionHypotheses[i];
        readDivisionHypothesis(jsonHyp);
    }

    // read exclusion constraints between detections
    const Json::Value exclusions = root[JsonTypeNames[JsonTypes::Exclusions]];
    std::cout << "\tcontains " << exclusions.size() << " exclusions" << std::endl;
    for(int i = 0; i < (int)exclusions.size(); i++)
    {
        const Json::Value jsonExc = exclusions[i];
        readExclusionConstraints(jsonExc);
    }
}

void JsonModel::setJsonGtFile(const std::string& filename)
{
    groundTruthFilename_ = filename;
}

Solution JsonModel::getGroundTruth()
{
    std::ifstream input(groundTruthFilename_.c_str());
    if(!input.good())
        throw std::runtime_error("Could not open JSON ground truth file " + groundTruthFilename_);

    if(!model_.numberOfVariables() > 0)
        throw std::runtime_error("OpenGM model must be initialized before reading a ground truth file!");

    Json::Value root;
    input >> root;

    const Json::Value linkingResults = root[JsonTypeNames[JsonTypes::LinkResults]];
    std::cout << "\tcontains " << linkingResults.size() << " linking annotations" << std::endl;

    // create a solution vector that holds a value for each segmentation / detection / link
    Solution solution(model_.numberOfVariables(), 0);

    // first set all links and the respective source nodes to active
    for(int i = 0; i < linkingResults.size(); ++i)
    {
        const Json::Value jsonHyp = linkingResults[i];
        helpers::IdLabelType srcId = jsonHyp[JsonTypeNames[JsonTypes::SrcId]].asLabelType();
        helpers::IdLabelType destId = jsonHyp[JsonTypeNames[JsonTypes::DestId]].asLabelType();
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
        }
    }

    // read segmentation variables
    const Json::Value segmentationResults = root[JsonTypeNames[JsonTypes::DetectionResults]];
    std::cout << "\tcontains " << segmentationResults.size() << " detection annotations" << std::endl;
    for(int i = 0; i < segmentationResults.size(); ++i)
    {
        const Json::Value jsonHyp = segmentationResults[i];
        helpers::IdLabelType id = jsonHyp[JsonTypeNames[JsonTypes::Id]].asLabelType();
        size_t value = jsonHyp[JsonTypeNames[JsonTypes::Value]].asUInt();

        solution[segmentationHypotheses_[id].getDetectionVariable().getOpenGMVariableId()] = value;
    }

    // read division variable states
    const Json::Value divisionResults = root[JsonTypeNames[JsonTypes::DivisionResults]];
    std::cout << "\tcontains " << divisionResults.size() << " division annotations" << std::endl;
    for(int i = 0; i < divisionResults.size(); ++i)
    {
        const Json::Value jsonHyp = divisionResults[i];
        bool value = jsonHyp[JsonTypeNames[JsonTypes::Value]].asBool();

        if(value)
        {
            // depending on internal or external division node setup, handle both gracefully!
            helpers::IdLabelType id;
            if(jsonHyp.isMember(JsonTypeNames[JsonTypes::Id]))
            {
                // id is given for internal division
                id = jsonHyp[JsonTypeNames[JsonTypes::Id]].asLabelType();
            }
            else
            {
                // parent is given for external
                if(!jsonHyp.isMember(JsonTypeNames[JsonTypes::Parent]))
                    throw std::runtime_error("Invalid configuration of a JSON division result entry");

                id = jsonHyp[JsonTypeNames[JsonTypes::Parent]].asLabelType();
            }

            if(solution[segmentationHypotheses_[id].getDetectionVariable().getOpenGMVariableId()] == 0)
            {
                // in any case the parent must be active!
                std::stringstream error;
                error << "Cannot activate division of node " << id << " that is not active!";
                throw std::runtime_error(error.str());
            }

            if(jsonHyp.isMember(JsonTypeNames[JsonTypes::Id]))
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
            else if(jsonHyp.isMember(JsonTypeNames[JsonTypes::Parent]) && jsonHyp.isMember(JsonTypeNames[JsonTypes::Children]))
            {
                const Json::Value& children = jsonHyp[JsonTypeNames[JsonTypes::Children]];

                if(!children.isArray() || children.size() != 2)
                {
                    std::stringstream error;
                    error << "Activating an external division of parent " << id << " requires two children!";
                    throw std::runtime_error(error.str());
                }

                std::vector<IdLabelType> childrenIds;
                for(int i = 0; i < children.size(); ++i)
                {
                    childrenIds.push_back(children[i].asLabelType());
                }

                // always use ordered list of children!
                std::sort(childrenIds.begin(), childrenIds.end());

                DivisionHypothesis::IdType idx = std::make_tuple(jsonHyp[JsonTypeNames[JsonTypes::Parent]].asLabelType(),
                                                                childrenIds[0],
                                                                childrenIds[1]);

                if(divisionHypotheses_.find(idx) == divisionHypotheses_.end())
                {
                    std::stringstream error;
                    error << "Parent " << id << " does not have division to " << children[0].asLabelType() << " and " << children[1].asLabelType() << " to set active!";
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
                    // std::cout << "deducing appearance value " << detValue << " for node " << iter->first << std::endl;
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
                    // std::cout << "deducing disappearance value " << detValue << " for node " << iter->first << std::endl;
                    solution[iter->second.getDisappearanceVariable().getOpenGMVariableId()] = detValue;
                }
            }
        }
    }

    // std::cout << "found gt solution: " << solution << std::endl;

    return solution;
}

void JsonModel::saveResultToJson(const std::string& filename, const Solution& sol) const
{
    std::ofstream output(filename.c_str());
    if(!output.good())
        throw std::runtime_error("Could not open JSON result file for saving: " + filename);

    Json::Value root;

    // save links
    Json::Value& linksJson = root[JsonTypeNames[JsonTypes::LinkResults]];
    for(auto iter = linkingHypotheses_.begin(); iter != linkingHypotheses_.end() ; ++iter)
    {
        size_t value = sol[iter->second->getVariable().getOpenGMVariableId()];
        if(value > 0)
            linksJson.append(linkToJson(iter->second, value));
    }

    // save divisions
    Json::Value& divisionsJson = root[JsonTypeNames[JsonTypes::DivisionResults]];
    for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
    {
        if(iter->second.getDivisionVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second.getDivisionVariable().getOpenGMVariableId()];
            if(value > 0)
                divisionsJson.append(divisionToJson(iter->second, value));
        }
    }
    for(auto iter = divisionHypotheses_.begin(); iter != divisionHypotheses_.end() ; ++iter)
    {
        if(iter->second->getVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second->getVariable().getOpenGMVariableId()];
            if(value > 0)
                divisionsJson.append(divisionToJson(iter->second, value));
        }
    }

    // save detections
    Json::Value& detectionsJson = root[JsonTypeNames[JsonTypes::DetectionResults]];
    for(auto iter = segmentationHypotheses_.begin(); iter != segmentationHypotheses_.end() ; ++iter)
    {
        if(iter->second.getDetectionVariable().getOpenGMVariableId() >= 0)
        {
            size_t value = sol[iter->second.getDetectionVariable().getOpenGMVariableId()];
            if(value > 0)
                detectionsJson.append(detectionToJson(iter->second, value));
        }
    }

    output << root << std::endl;
}

const Json::Value JsonModel::linkToJson(const std::shared_ptr<LinkingHypothesis>& link, size_t state) const
{
    Json::Value val;
    val[JsonTypeNames[JsonTypes::SrcId]] = Json::Value(link->getSrcId());
    val[JsonTypeNames[JsonTypes::DestId]] = Json::Value(link->getDestId());
    val[JsonTypeNames[JsonTypes::Value]] = Json::Value((unsigned int)state);
    return val;
}

const Json::Value JsonModel::divisionToJson(const std::shared_ptr<DivisionHypothesis>& division, size_t state) const
{
    Json::Value val;
    val[JsonTypeNames[JsonTypes::Parent]] = Json::Value(division->getParentId());
    Json::Value& children = val[JsonTypeNames[JsonTypes::Children]];
    for(auto c : division->getChildrenIds())
        children.append(c);

    val[JsonTypeNames[JsonTypes::Value]] = Json::Value(state==1);
    return val;
}

const Json::Value JsonModel::divisionToJson(const SegmentationHypothesis& segmentation, size_t value) const
{
    // save as bool
    Json::Value val;
    val[JsonTypeNames[JsonTypes::Id]] = Json::Value(segmentation.getId());
    val[JsonTypeNames[JsonTypes::Value]] = Json::Value((bool)(value > 0));
    return val;
}

const Json::Value JsonModel::detectionToJson(const SegmentationHypothesis& segmentation, size_t value) const
{
    // save as int
    Json::Value val;
    val[JsonTypeNames[JsonTypes::Id]] = Json::Value(segmentation.getId());
    val[JsonTypeNames[JsonTypes::Value]] = Json::Value((int)(value));
    return val;
}


} // end namespace mht