#ifndef COVER_TREE_H
#define COVER_TREE_H

#include <memory>
#include <iostream>

#include "covertreenode.h"

namespace covertree
{

/**
 * Cover tree
 */
class CoverTree
{
public: // API

	/// create empty tree
	CoverTree();

	/// create tree from initialized root
	CoverTree(CoverTreeNodePtr root);

	/// methods to print the cover tree to a graphviz dot file
	void toDot(const std::string& filename) const;

	CoverTreeNodePtr getRoot() const;
private: // members
	CoverTreeNodePtr root_;
};

} // end namespace covertree

#endif
