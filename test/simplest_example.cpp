// #define BOOST_TEST_MODULE simplest_example

#include <iostream>
#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "helpers.h"
#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include "model.h"

using namespace mht;

// BOOST_AUTO_TEST_CASE( OpenGMInference )
int main()
{
	Model model;
	model.readFromJson("example.json");
	size_t numWeights = model.computeNumWeights();
	
	// BOOST_CHECK_EQUAL(numWeights, 6);

	// run inference with some dummy weight vector
	std::vector<double> weights(6, 1.0);
	Solution sol = model.infer(weights);
	return 0;
}

// BOOST_AUTO_TEST_CASE( LearnWeights )
// {
// 	CoverTree tree(createTestRootWithGT());
// 	CoverTreeInferenceModel infModel(tree, 
// 		5, // max num objects within node
// 		3); // max num objects that can be added if the children do not account for them

// 	infModel.learn();

// 	std::string filename = std::tmpnam(nullptr);
// 	tree.toDot(filename);
// }

// BOOST_AUTO_TEST_CASE( InferenceWithLearnedWeights )
// {
// 	CoverTree tree(createTestRootWithGT());
// 	CoverTreeInferenceModel infModel(tree, 
// 		5, // max num objects within node
// 		3); // max num objects that can be added if the children do not account for them

// 	// run inference with learned vector
// 	std::vector<double> weights = {-1, 2, 1, 0, 0, 2, 1, -3, 0, 0, 0, -2, 0, 1, -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, -1, -1, 4, 0, -1, -4, 0, 1, 0, 0, 0};
// 	infModel.infer(weights);
	
// 	BOOST_CHECK_EQUAL(tree.getRoot()->getCoverLabel(), 4);
// 	BOOST_CHECK_EQUAL(tree.getRoot()->getAddLabel(), 0);

// 	std::string filename = std::tmpnam(nullptr);
// 	tree.toDot(filename);
// }
