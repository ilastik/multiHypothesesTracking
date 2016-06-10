// #define BOOST_TEST_MODULE simplest_example

#include <iostream>
#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "helpers.h"
#include "segmentationhypothesis.h"
#include "linkinghypothesis.h"
#include "jsonmodel.h"

using namespace mht;
using namespace helpers;

// BOOST_AUTO_TEST_CASE( OpenGMInference )
int main()
{
	JsonModel model;
	model.readFromJson("magic.json");
	size_t numWeights = model.computeNumWeights();
	Solution gt = model.readGTfromJson("gt.json");
	std::vector<double> weights = model.learn(gt);
	std::cout << "GT is valid? " << (model.verifySolution(gt)?"yes":"no") << std::endl;
	model.toDot("gt.dot", &gt);

	saveWeightsToJson(weights, "weights.json");
	std::vector<double> weights2 = readWeightsFromJson("weights.json");
	// BOOST_CHECK_EQUAL(weights.size(), weights2.size());
	// for(size_t i = 0; i < weights.size(); i++)
	// 	BOOST_CHECK_EQUAL(weights[i], weights2[i]);

	JsonModel model2;
	model2.readFromJson("magic.json");
	Solution sol = model2.infer(weights);
	model2.saveResultToJson("result.json", sol);
	std::cout << "Solution is valid? " << (model2.verifySolution(sol)?"yes":"no") << std::endl;
	model2.toDot("result.dot", &sol);

	// BOOST_CHECK_EQUAL(numWeights, 6);

	// run inference with some dummy weight vector
	// std::vector<double> weights(6, 1.0);
	// Solution sol = model.infer(weights);

	return 0;
}

