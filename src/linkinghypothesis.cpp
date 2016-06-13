#include "linkinghypothesis.h"
#include <stdexcept>

using namespace helpers;

namespace mht
{

LinkingHypothesis::LinkingHypothesis()
{}

LinkingHypothesis::LinkingHypothesis(helpers::IdLabelType srcId, helpers::IdLabelType destId, const helpers::StateFeatureVector& features):
    srcId_(srcId),
    destId_(destId),
    variable_(features)
{}

void LinkingHypothesis::toDot(std::ostream& stream, const Solution* sol) const
{
    stream << "\t" << srcId_ << " -> " << destId_;

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
}

void LinkingHypothesis::registerWithSegmentations(std::map<helpers::IdLabelType, SegmentationHypothesis>& segmentationHypotheses)
{
    assert(segmentationHypotheses.find(srcId_) != segmentationHypotheses.end());
    assert(segmentationHypotheses.find(destId_) != segmentationHypotheses.end());

    // std::cout << "Registering outgoing link for " << srcId_ << std::endl;
    segmentationHypotheses[srcId_].addOutgoingLink(shared_from_this());
    // std::cout << "Registering incoming link for " << destId_ << std::endl;
    segmentationHypotheses[destId_].addIncomingLink(shared_from_this());
}

void LinkingHypothesis::addToOpenGMModel(
    GraphicalModelType& model, 
    WeightsType& weights, 
    bool statesShareWeights,
    const std::vector<size_t>& weightIds)
{
    // std::cout << "Adding linking hypothesis between " << srcId_ << " and " << destId_ << " to opengm" << std::endl;

    variable_.addToOpenGM(model, statesShareWeights, weights, weightIds);
}

const Json::Value LinkingHypothesis::toJson(size_t state) const
{
    Json::Value val;
    val[JsonTypeNames[JsonTypes::SrcId]] = Json::Value(srcId_);
    val[JsonTypeNames[JsonTypes::DestId]] = Json::Value(destId_);
    val[JsonTypeNames[JsonTypes::Value]] = Json::Value((unsigned int)state);
    return val;
}

} // end namespace mht
