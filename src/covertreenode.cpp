#include "covertreenode.h"
#include "helpers.h"
#include "nodevisitor.h"

namespace covertree
{

// initialize static member
size_t CoverTreeNode::next_id_ = 0;

CoverTreeNode::CoverTreeNode(FeatureVector cover_features,
							FeatureVector add_features):
	cover_features_(cover_features),
	add_features_(add_features),
	id_(next_id_++)
{}

void CoverTreeNode::nodeToDot(std::ostream& stream) const
{
	stream << " " << id_ << " [ label=\"cover_features=" << cover_features_ 
		   << "\\nadd_features=" << add_features_ 
		   << "\\ny=" << getCoverLabel()
		   << "\\na=" << getAddLabel()
		   << "\" ];\n";
}

void CoverTreeNode::childLinksToDot(std::ostream& stream) const
{
	for(auto c : children_)
	{
		stream << " " << id_ << " -> " << c->id_ << ";\n";
	}
}

void CoverTreeNode::accept(NodeVisitor* visitor)
{
	visitor->visit(shared_from_this());
	
	for(auto c : children_)
	{
		c->accept(visitor);
	}
}

void CoverTreeNode::addChild(CoverTreeNodePtr child)
{
	children_.push_back(child);
	child->setParent(shared_from_this());
}

void CoverTreeNode::setParent(CoverTreeNodePtr parent)
{
	parent_ = parent;
}

void CoverTreeNode::setCoverFeatures(const FeatureVector& f)
{
	cover_features_ = f;
}

void CoverTreeNode::setAddFeatures(const FeatureVector& f)
{
	add_features_ = f;
}

void CoverTreeNode::setCoverLabel(size_t y)
{
	cover_label_ = y;
}

size_t CoverTreeNode::getCoverLabel() const
{
	return cover_label_;
}

size_t CoverTreeNode::getAddLabel() const
{
	size_t childSum = 0;
	for (CoverTreeNodePtr child : children_)
		childSum += child->getCoverLabel();

	return getCoverLabel() - childSum;
}

const FeatureVector& CoverTreeNode::getCoverFeatures() const
{
	return cover_features_;
}

const FeatureVector& CoverTreeNode::getAddFeatures() const
{
	return add_features_;
}

const std::vector<CoverTreeNodePtr>& CoverTreeNode::getChildren() const
{
	return children_;
}

} // end namespace covertree
