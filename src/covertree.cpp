#include <fstream>
#include <stdexcept>

#include "covertree.h"
#include "nodevisitor.h"

namespace covertree
{

CoverTree::CoverTree()
{}

CoverTree::CoverTree(CoverTreeNodePtr root):
	root_(root)
{}

void CoverTree::toDot(const std::string& filename) const
{
	if(!root_)
		throw new std::runtime_error("Cannot print empty graph");

	std::ofstream out(filename.c_str());

	if(!out.good())
		throw new std::runtime_error("Could not open output file " + filename);

	out << "digraph G {\n";

	// recursively add all nodes
	NodeVisitor nodeToDotVisitor([&](CoverTreeNodePtr node){
		node->nodeToDot(out);
	});
	root_->accept(&nodeToDotVisitor);

	// add all links to their children
	NodeVisitor childLinksToDotVisitor([&](CoverTreeNodePtr node){
		node->childLinksToDot(out);
	});
	root_->accept(&childLinksToDotVisitor);

	out << "}";	
}

CoverTreeNodePtr CoverTree::getRoot() const
{
	return root_;
}

} // end namespace covertree
