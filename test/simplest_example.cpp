#define BOOST_TEST_MODULE simplest_example

#include <iostream>
#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "helpers.h"
#include "covertreenode.h"
#include "covertree.h"
#include "covertreeinferencemodel.h"

using namespace covertree;

FeatureVector sizeToFeatureVector(size_t pixelSize)
{
	FeatureVector f(6, 0.0);

	// insert binned size feature
	size_t idx = pixelSize / 10;
	if(idx > 4)
		idx == 4;
	f[idx] = 1.0;

	f[5] = 1.0; // constant feature
	
	return f;
}

CoverTreeNodePtr createTestRootWithGT()
{
	// ATTENTION: std::make_shared doesn't work when initializer lists are involved!
	// root with empty parent, empty children
	CoverTreeNodePtr root = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(48), {3.0, 2.0, 1.0}));
	root->setCoverLabel(4);

	// add children:
	CoverTreeNodePtr c1 = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(26), {4.0, 2.0, 1.0}));
	CoverTreeNodePtr c2 = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(19), {10.0, 2.0, 1.0}));
	c1->setCoverLabel(2);
	c2->setCoverLabel(1);
	root->addChild(c1);
	root->addChild(c2);

	CoverTreeNodePtr c1a = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(11), {11.0, 0.0, 1.0}));
	CoverTreeNodePtr c1b = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(11), {11.0, 0.0, 1.0}));
	c1a->setCoverLabel(1);
	c1b->setCoverLabel(1);
	c1->addChild(c1a);
	c1->addChild(c1b);

	CoverTreeNodePtr c2a = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(3), {3.0, 0.0, 1.0}));
	CoverTreeNodePtr c2b = CoverTreeNodePtr(new CoverTreeNode(sizeToFeatureVector(6), {6.0, 0.0, 1.0}));
	c2a->setCoverLabel(0);
	c2b->setCoverLabel(0);
	c2->addChild(c2a);
	c2->addChild(c2b);

	return root;
}

BOOST_AUTO_TEST_CASE( TreeCreationAndDotExport )
{
	// create tree
	CoverTree tree(createTestRootWithGT());

	// print to dot file
	std::string filename = std::tmpnam(nullptr);
	tree.toDot(filename);
}

BOOST_AUTO_TEST_CASE( OpenGMInferenceOnTree )
{
	CoverTree tree(createTestRootWithGT());
	CoverTreeInferenceModel infModel(tree, 
		5, // max num objects within node
		3); // max num objects that can be added if the children do not account for them

	// run inference with some dummy weight vector
	std::vector<double> weights(39, 1.0);
	infModel.infer(weights);

	std::string filename = std::tmpnam(nullptr);
	tree.toDot(filename);
	
	BOOST_CHECK_EQUAL(tree.getRoot()->getCoverLabel(), 0);
	BOOST_CHECK_EQUAL(tree.getRoot()->getAddLabel(), 0);
}

BOOST_AUTO_TEST_CASE( LearnWeights )
{
	CoverTree tree(createTestRootWithGT());
	CoverTreeInferenceModel infModel(tree, 
		5, // max num objects within node
		3); // max num objects that can be added if the children do not account for them

	infModel.learn();

	std::string filename = std::tmpnam(nullptr);
	tree.toDot(filename);
}

BOOST_AUTO_TEST_CASE( InferenceWithLearnedWeights )
{
	CoverTree tree(createTestRootWithGT());
	CoverTreeInferenceModel infModel(tree, 
		5, // max num objects within node
		3); // max num objects that can be added if the children do not account for them

	// run inference with learned vector
	std::vector<double> weights = {-1, 2, 1, 0, 0, 2, 1, -3, 0, 0, 0, -2, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, -1, -1, 4, 0, -1, -4, 0, 1, 0, 0, 0};
	infModel.infer(weights);
	
	BOOST_CHECK_EQUAL(tree.getRoot()->getCoverLabel(), 4);
	BOOST_CHECK_EQUAL(tree.getRoot()->getAddLabel(), 0);

	std::string filename = std::tmpnam(nullptr);
	tree.toDot(filename);
}
