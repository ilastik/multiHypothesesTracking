#include <iostream>
#include <opengm/datastructures/marray/marray_hdf5.hxx>
#include "covertree.h"
#include "covertreeinferencemodel.h"

using namespace covertree;

int main(int argc, char** argv) {

	hid_t projectFile = marray::hdf5::openFile("training_dataset.hdf");

	marray::Marray<double> featuresCover;
	marray::hdf5::load(projectFile, "features_cover", featuresCover);

	int numNodes         = featuresCover.shape(0);
	int numCoverFeatures = featuresCover.shape(1);

	std::cout
			<< "creating covertree with " << numNodes
			<< " nodes and " << numCoverFeatures
			<< " cover features" << std::endl;

	std::vector<CoverTreeNodePtr> nodes;
	for (int i = 0; i < numNodes; i++) {

		std::vector<double> fc(numCoverFeatures);
		for (int j = 0; j < numCoverFeatures; j++)
			fc[j] = featuresCover(i, j);

		// TODO: if we have special 'add features', set them here instead of 
		// repeating the 'cover features'
		nodes.push_back(CoverTreeNodePtr(new CoverTreeNode(fc,fc)));
	}

	marray::Marray<int> childEdges;
	marray::hdf5::load(projectFile, "child_edges", childEdges);

	std::cout << "adding " << childEdges.shape(0) << " child edges" << std::endl;

	for (int i = 0; i < childEdges.shape(0); i++)
		nodes[childEdges(i, 0)]->addChild(nodes[childEdges(i, 1)]);

	marray::Marray<int> labels;
	marray::hdf5::load(projectFile, "labels", labels);

	for (int i = 0; i < numNodes; i++)
		nodes[i]->setCoverLabel(labels(i));

	// find root
	CoverTreeNodePtr root;
	for (CoverTreeNodePtr n : nodes)
		if (!n->getParent())
			if (!root)
				root = n;
			else
				throw std::runtime_error("Tree contains multiple root nodes!");

	CoverTree tree(root);
	CoverTreeInferenceModel infModel(
		tree,
		10, // max num objects within node
		5); // max num objects that can be added if the children do not account for them

	infModel.learn();
	const auto& weights = infModel.getWeights();

	// get result on training data
	std::vector<double> w;
	for (int i = 0; i < weights.numberOfWeights(); i++)
		w.push_back(weights.getWeight(i));

	infModel.infer(w);

	for (int i = 0; i < numNodes; i++)
		if (nodes[i]->getCoverLabel() != labels(i))
			std::cout
					<< "cover label of node " << i
					<< " is wrong: should be " << labels(i)
					<< ", is " << nodes[i]->getCoverLabel()
					<< std::endl;
}
