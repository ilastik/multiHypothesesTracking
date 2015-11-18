#include "nodevisitor.h"

namespace covertree
{

NodeVisitor::NodeVisitor(NodeVisitor::NodeCallbackFunc func):
	node_callback_func_(func)
{}

void NodeVisitor::visit(CoverTreeNodePtr node)
{
	node_callback_func_(node);
}

} // end namespace covertree
