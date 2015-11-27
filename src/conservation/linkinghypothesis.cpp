#include "conservation/linkinghypothesis.h"
#include <stdexcept>

using namespace helpers;

namespace constracking
{

LinkingHypothesis::LinkingHypothesis():
	srcId_(-1),
	destId_(-1)
{}

LinkingHypothesis::LinkingHypothesis(int srcId, int destId, const helpers::StateFeatureVector& features):
	srcId_(srcId),
	destId_(destId),
	variable_(features)
{}

const std::pair<int, int> LinkingHypothesis::readFromJson(const Json::Value& entry)
{
	if(!entry.isObject())
		throw std::runtime_error("Cannot extract LinkingHypothesis from non-object JSON entry");
	if(!entry.isMember(JsonTypeNames[JsonTypes::SrcId]) || !entry[JsonTypeNames[JsonTypes::SrcId]].isInt() 
		|| !entry.isMember(JsonTypeNames[JsonTypes::DestId]) || !entry[JsonTypeNames[JsonTypes::DestId]].isInt() 
		|| !entry.isMember(JsonTypeNames[JsonTypes::Features]) || !entry[JsonTypeNames[JsonTypes::Features]].isArray())
		throw std::runtime_error("JSON entry for SegmentationHytpohesis is invalid");

	srcId_ = entry[JsonTypeNames[JsonTypes::SrcId]].asInt();
	destId_ = entry[JsonTypeNames[JsonTypes::DestId]].asInt();

	// get transition features
	variable_ = Variable(extractFeatures(entry, JsonTypes::Features));

	// std::cout << "Found linking hypothesis between " << srcId_ << " and " << destId_ << std::endl;
	return std::make_pair(srcId_, destId_);
}

void LinkingHypothesis::toDot(std::ostream& stream, const Solution* sol) const
{
	stream << "\t" << srcId_ << " -> " << destId_;

	if(sol != nullptr && variable_.getOpenGMVariableId() > 0 && sol->at(variable_.getOpenGMVariableId()) > 0)
		stream << "[ color=\"blue\" fontcolor=\"blue\" ]";

	stream << "; \n" << std::flush;
}

void LinkingHypothesis::registerWithSegmentations(std::map<int, SegmentationHypothesis>& segmentationHypotheses)
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
