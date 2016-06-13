#include "divisionhypothesis.h"
#include <stdexcept>
#include <algorithm>

using namespace helpers;

namespace mht
{

DivisionHypothesis::DivisionHypothesis()
{}

DivisionHypothesis::DivisionHypothesis(helpers::IdLabelType parent, 
                                       const std::vector<helpers::IdLabelType>& children, 
                                       const helpers::StateFeatureVector& features):
    parentId_(parent),
    childrenIds_(children),
    variable_(features)
{}

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

} // end namespace mht
