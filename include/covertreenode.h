#ifndef COVER_TREE_NODE_H
#define COVER_TREE_NODE_H

#include <memory>
#include <vector>
#include <iostream>

#include "helpers.h"

namespace covertree
{

// --------------------------------------------------------------
// typedefs
// --------------------------------------------------------------
class CoverTreeNode;
typedef std::shared_ptr<CoverTreeNode> CoverTreeNodePtr;

// visitor forward declaration to walk along nodes in tree
class NodeVisitor;

// --------------------------------------------------------------
/**
 * Cover tree nodes contain pointers to parent as well as children, and their own features.
 */
class CoverTreeNode : public std::enable_shared_from_this<CoverTreeNode>
{
public: // API

	/// no default constructor
	CoverTreeNode() = delete;

	CoverTreeNode(
		FeatureVector cover_features = {},
		FeatureVector add_features = {});

	/// adding children and features
	void addChild(CoverTreeNodePtr child);
	void setCoverFeatures(const FeatureVector& f);
	void setAddFeatures(const FeatureVector& f);

	const FeatureVector& getCoverFeatures() const;
	const FeatureVector& getAddFeatures() const;

	CoverTreeNodePtr getParent() const { return parent_; }
	const std::vector<CoverTreeNodePtr>& getChildren() const;

	/**
	 * Set the cover label, i.e., the number of objects covered by this node. 
	 * This is used both for training to set the ground-truth, and during 
	 * inference to set the predicted number.
	 */
	void setCoverLabel(size_t y);
	size_t getCoverLabel() const;

	/**
	 * Get the difference of this node's cover label and the sum of its children 
	 * cover labels, i.e., the number of objects this node is additionally 
	 * accounting for.
	 */
	size_t getAddLabel() const;

	/// accept a visitor
	void accept(NodeVisitor* visitor);

	/// methods to print the cover graph to a graphviz dot file
	void nodeToDot(std::ostream& stream) const;
	void childLinksToDot(std::ostream& stream) const;

private: // methods
	void setParent(CoverTreeNodePtr parent);

private: // members
	/// parent node in cover tree
	CoverTreeNodePtr parent_;

	/// children in cover tree
	std::vector<CoverTreeNodePtr> children_;

	/// features that are used to determine how many objects this node covers
	FeatureVector cover_features_;

	/// features that are used to determine how many more objects this node 
	/// covers than it's active children
	FeatureVector add_features_;

	/// the node ID is only used for graphviz dot output
	size_t id_;

	/// inferred "cover" labels
	size_t cover_label_;

private: // class variables
	static size_t next_id_;
};

} // end namespace covertree

#endif
