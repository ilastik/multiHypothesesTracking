#ifndef NODE_VISITOR_H
#define NODE_VISITOR_H

#include <functional>
#include "covertreenode.h"

namespace covertree
{

/**
* A simple visitor that allows to plug in a callback that will be called for each node of the tree.
*/
class NodeVisitor
{
public: //typedefs
	typedef std::function<void(CoverTreeNodePtr)> NodeCallbackFunc;

public: // API
	NodeVisitor() = delete;
	NodeVisitor(NodeCallbackFunc func);

	// this method is called by visited nodes
	void visit(CoverTreeNodePtr node);

private: // members
	NodeCallbackFunc node_callback_func_;
};

} // end namespace covertree

#endif
