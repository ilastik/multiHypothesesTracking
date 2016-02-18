#include "divisionhypothesis.h"
#include <stdexcept>
#include <algorithm>

using namespace helpers;

namespace mht
{

DivisionHypothesis::DivisionHypothesis()
{}

DivisionHypothesis::DivisionHypothesis(helpers::IdLabelType parent, const std::vector<helpers::IdLabelType>& children, const helpers::StateFeatureVector& features):
    parentId_(parent),
    childrenIds_(children),
    variable_(features)
{}

const DivisionHypothesis::IdType DivisionHypothesis::readFromJson(const Json::Value& entry)
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

    parentId_ = entry[JsonTypeNames[JsonTypes::Parent]].asLabelType();
    childrenIds_.clear();

    const Json::Value children = entry[JsonTypeNames[JsonTypes::Children]];
    for(int i = 0; i < children.size(); ++i)
    {
        childrenIds_.push_back(children[i].asLabelType());
    }

    // always use ordered list of children!
    std::sort(childrenIds_.begin(), childrenIds_.end());

    // get transition features
    variable_ = Variable(extractFeatures(entry, JsonTypes::Features));

    // std::cout << "Found division hypothesis between " << srcId_ << " and " << destId_ << std::endl;
    return std::make_tuple(parentId_, childrenIds_[0], childrenIds_[1]);
}

void DivisionHypothesis::toDot(std::ostream& stream, const Solution* sol) const
{
    std::stringstream divNodeName;
    divNodeName << "\"divisionOf" << parentId_ << "To" << childrenIds_[0] << "And" << childrenIds_[1] << "\"";
    stream << "\t" << parentId_ << " -> " << divNodeName.str();

    if(sol != nullptr && variable_.getOpenGMVariableId() >= 0)
    {
        size_t value = sol->at(variable_.getOpenGMVariableId());
        stream << "[ label=\"value=" << value << "\" ";

        if(value > 0)
            stream << "color=\"blue\" fontcolor=\"blue\" ]";    
        else
            stream << "]";
    }

    stream << "; \n" << std::flush;
    stream << divNodeName.str() << " -> " << childrenIds_[0] << "; \n" << std::flush;
    stream << divNodeName.str() << " -> " << childrenIds_[1] << "; \n" << std::flush;
}

void DivisionHypothesis::registerWithSegmentations(std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses)
{
    assert(segmentationHypotheses.find(parentId_) != segmentationHypotheses.end());
    for(auto c : childrenIds_)
        assert(segmentationHypotheses.find(c) != segmentationHypotheses.end());

    // std::cout << "Registering outgoing link for " << srcId_ << std::endl;
    segmentationHypotheses[parentId_].addOutgoingDivision(shared_from_this());
    // std::cout << "Registering incoming link for " << destId_ << std::endl;
    for(auto c : childrenIds_)
        segmentationHypotheses[c].addIncomingDivision(shared_from_this());
}

void DivisionHypothesis::addToOpenGMModel(
    GraphicalModelType& model, 
    WeightsType& weights, 
    bool statesShareWeights,
    const std::vector<size_t>& weightIds)
{
    // std::cout << "Adding linking hypothesis between " << srcId_ << " and " << destId_ << " to opengm" << std::endl;

    variable_.addToOpenGM(model, statesShareWeights, weights, weightIds);
}

const Json::Value DivisionHypothesis::toJson(size_t state) const
{
    Json::Value val;
    val[JsonTypeNames[JsonTypes::Parent]] = Json::Value(parentId_);
    Json::Value& children = val[JsonTypeNames[JsonTypes::Children]];
    for(auto c : childrenIds_)
        children.append(c);

    val[JsonTypeNames[JsonTypes::Value]] = Json::Value(state==1);
    return val;
}

} // end namespace mht
