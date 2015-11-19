// #include "covertreeinferencemodel.h"
// #include "nodevisitor.h"
// #include <stdexcept>

// #ifdef WITH_CPLEX
// #include <opengm/inference/lpcplex2.hxx>
// #else
// #include <opengm/inference/lpgurobi2.hxx>
// #endif

// #include <opengm/learning/struct-max-margin.hxx>

// namespace covertree
// {

// CoverTreeInferenceModel::CoverTreeInferenceModel(CoverTree& tree, size_t max_label, size_t max_add):
// 	tree_(tree),
// 	max_label_(max_label),
// 	max_add_(max_add)
// {}

// void CoverTreeInferenceModel::buildModel(CoverTreeInferenceModel::WeightsType& weights)
// {	
// 	if(weights.numberOfWeights() != getNumWeights())
// 		throw new std::runtime_error("Trying to build inference model with wrong number of weights");

// 	graphical_model_ = GraphicalModelType();
// 	variable_mapping_.clear();

// 	// recursively add all nodes with their unaries
// 	NodeVisitor node_to_GM_visitor([&](CoverTreeNodePtr node){
// 		graphical_model_.addVariable(max_label_); // variable y_node
// 		graphical_model_.addVariable(max_add_); // variable a_node
// 		IndexType label_var = graphical_model_.numberOfVariables()-2;
// 		IndexType add_var = graphical_model_.numberOfVariables()-1;
// 		variable_mapping_[node] = std::make_pair(label_var, add_var);

// 		// add cover-unary
// 		const FeatureVector& cover_features = node->getCoverFeatures();
// 		std::vector<FeaturesAndIndicesType> features_and_weights_per_label;
// 		size_t num_features = cover_features.size();

// 		for(size_t l = 0; l < max_label_; l++)
// 		{
// 			FeaturesAndIndicesType feat_and_ind;

// 			feat_and_ind.features = cover_features;
// 			for(size_t i = 0; i < num_features; ++i)
// 				feat_and_ind.weightIds.push_back(l * num_features + i);

// 			features_and_weights_per_label.push_back(feat_and_ind);
// 		}

// 		LearnableUnaryFuncType cover_unary(weights, features_and_weights_per_label);
// 		GraphicalModelType::FunctionIdentifier fid = graphical_model_.addFunction(cover_unary);
// 		graphical_model_.addFactor(fid, &label_var, &label_var+1);

// 		// add add-unary
// 		const FeatureVector& add_features = node->getAddFeatures();
// 		features_and_weights_per_label.clear();
// 		size_t offset = num_features * max_label_;
// 		num_features = add_features.size();

// 		for(size_t l = 0; l < max_add_; l++)
// 		{
// 			FeaturesAndIndicesType feat_and_ind;

// 			feat_and_ind.features = add_features;
// 			for(size_t i = 0; i < num_features; ++i)
// 				feat_and_ind.weightIds.push_back(offset + l * num_features + i);

// 			features_and_weights_per_label.push_back(feat_and_ind);
// 		}

// 		LearnableUnaryFuncType add_unary(weights, features_and_weights_per_label);
// 		fid = graphical_model_.addFunction(add_unary);
// 		graphical_model_.addFactor(fid, &add_var, &add_var+1);
// 	});
// 	tree_.getRoot()->accept(&node_to_GM_visitor);

// 	std::vector<LabelType> constraintShape;

// 	// add constraints
// 	NodeVisitor constraint_visitor([&](CoverTreeNodePtr node){
// 		const std::vector<CoverTreeNodePtr>& children = node->getChildren();
// 		std::vector<LabelType> factorVariables;
// 		LinearConstraintFunctionType::LinearConstraintType consistency_constraint;

// 		// add this variable's states with negative coefficient (bring to other side of =)
// 		for (size_t state = 1; state < max_label_; ++state){
//             const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(0, LabelType(state));
//             consistency_constraint.add(indicatorVariable, -1.0 * (ValueType)state);
//         }

//         // add this variable's "add" states with positive coeff
//         for (size_t state = 1; state < max_add_; ++state){
//             const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(1, LabelType(state));
//             consistency_constraint.add(indicatorVariable, (ValueType)state);
//         }

//         factorVariables.push_back(variable_mapping_[node].first);
//         constraintShape.push_back(graphical_model_.numberOfLabels(factorVariables.back()));
//         factorVariables.push_back(variable_mapping_[node].second);
//         constraintShape.push_back(graphical_model_.numberOfLabels(factorVariables.back()));

//         // add all children with positive coefficient
//         for(size_t i = 0; i < children.size(); ++i)
//         {
//         	CoverTreeNodePtr child = children[i];
//         	for (size_t state = 1; state < max_label_; ++state){
//                 const LinearConstraintFunctionType::LinearConstraintType::IndicatorVariableType indicatorVariable(2 + i, LabelType(state));
//                 consistency_constraint.add(indicatorVariable, (ValueType)state);
//             }

//             factorVariables.push_back(variable_mapping_[child].first);
//             constraintShape.push_back(graphical_model_.numberOfLabels(factorVariables.back()));
//         }

//         consistency_constraint.setBound( 0 );
//         consistency_constraint.setConstraintOperator(LinearConstraintFunctionType::LinearConstraintType::LinearConstraintOperatorType::Equal);

//         LinearConstraintFunctionType linearConstraintFunction(constraintShape.begin(), constraintShape.end(), &consistency_constraint, &consistency_constraint + 1);
//         GraphicalModelType::FunctionIdentifier linearConstraintFunctionID = graphical_model_.addFunction(linearConstraintFunction);
//         graphical_model_.addFactor(linearConstraintFunctionID, factorVariables.begin(), factorVariables.end());
// 	});
// 	tree_.getRoot()->accept(&constraint_visitor);	

// 	// some debug output
// 	std::cout << "Created model has " << graphical_model_.numberOfVariables() 
// 			  << " variables and " << graphical_model_.numberOfFactors() << " factors" << std::endl;
// }

// void CoverTreeInferenceModel::learn()
// {
// 	DatasetType dataset;
// 	WeightsType initialWeights(getNumWeights());
// 	dataset.setWeights(initialWeights);
// 	buildModel(dataset.getWeights());

// 	Solution gt;
// 	NodeVisitor node_to_GT_visitor([&](CoverTreeNodePtr node){
// 		gt.push_back(node->getCoverLabel());
// 		gt.push_back(node->getAddLabel());
// 	});
// 	tree_.getRoot()->accept(&node_to_GT_visitor);

// 	std::cout << "got ground truth: ";
// 	for(size_t s : gt)
// 		std::cout << s << " ";
// 	std::cout << std::endl;

// 	dataset.pushBackInstance(graphical_model_, gt);
	
// 	std::cout << "Done setting up dataset, creating learner" << std::endl;
// 	opengm::learning::StructMaxMargin<DatasetType>::Parameter learner_param;
// 	opengm::learning::StructMaxMargin<DatasetType> learner(dataset, learner_param);

// #ifdef WITH_CPLEX
// 	typedef opengm::LPCplex2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #else
// 	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #endif
	
// 	OptimizerType::Parameter optimizer_param;
// 	optimizer_param.integerConstraintNodeVar_ = true;
// 	optimizer_param.relaxation_ = OptimizerType::Parameter::TightPolytope;
// 	optimizer_param.verbose_ = true;
// 	optimizer_param.useSoftConstraints_ = false;

// 	std::cout << "Calling learn()..." << std::endl;
// 	learner.learn<OptimizerType>(optimizer_param); 
// 	std::cout << "extracting weights" << std::endl;
// 	weights_ = learner.getWeights();

// 	//std::cout << "learner found weights: ";
// 	//for(size_t i = 0; i < weights_.numberOfWeights(); i++)
// 		//std::cout << weights_.getWeight(i) << " ";
// 	//std::cout << std::endl;
// }

// void CoverTreeInferenceModel::infer(std::vector<ValueType>& weights)
// {
// 	// use weights that were given
// 	weights_ = WeightsType(getNumWeights());
// 	assert(weights.size() == weights_.numberOfWeights());
// 	for(size_t i = 0; i < weights.size(); i++)
// 		weights_.setWeight(i, weights[i]);
// 	buildModel(weights_);

// 	// run inference
// #ifdef WITH_CPLEX
// 	std::cout << "Using cplex optimizer" << std::endl;
// 	typedef opengm::LPCplex2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #else
// 	std::cout << "Using gurobi optimizer" << std::endl;
// 	typedef opengm::LPGurobi2<GraphicalModelType, opengm::Minimizer> OptimizerType;
// #endif
// 	OptimizerType::Parameter optimizer_param;
// 	optimizer_param.integerConstraintNodeVar_ = true;
// 	optimizer_param.relaxation_ = OptimizerType::Parameter::TightPolytope;
// 	optimizer_param.verbose_ = true;
// 	optimizer_param.useSoftConstraints_ = false;
// 	OptimizerType optimizer(graphical_model_, optimizer_param);
// 	OptimizerType::VerboseVisitorType cplex_visitor;

// 	Solution solution(graphical_model_.numberOfVariables());
// 	optimizer.infer(cplex_visitor);
// 	optimizer.arg(solution);
// 	std::cout << "solution has energy: " << optimizer.value() << std::endl;

// 	std::cout << " found solution: ";
// 	for(size_t s : solution)
// 		std::cout << s << " ";
// 	std::cout << std::endl;

// 	//std::cout << " when using weights: ";
// 	//for(double w : weights)
// 		//std::cout << w << " ";
// 	//std::cout << std::endl;

// 	storeSolutionInTree(solution);
// }

// void CoverTreeInferenceModel::storeSolutionInTree(Solution& solution)
// {
// 	if(solution.size() != 2 * variable_mapping_.size())
// 		throw new std::runtime_error("Number of labels in solution is not equal to 2* numTreeNodes!");

// 	for(VariableMap::iterator it = variable_mapping_.begin(); it != variable_mapping_.end(); ++it)
// 	{
// 		it->first->setCoverLabel(solution[it->second.first]);
// 	}

// 	for(VariableMap::iterator it = variable_mapping_.begin(); it != variable_mapping_.end(); ++it)
// 	{
// 		if (it->first->getAddLabel() != solution[it->second.second]) {
// 			std::cerr << "Cover and add labels in solution don't add up!" << std::endl;
// 			std::cerr << "node y = " << it->first->getCoverLabel() << std::endl;
// 			std::cerr << "node a = " << it->first->getAddLabel() << std::endl;
// 			std::cerr << "solution y = " << solution[it->second.first] << std::endl;
// 			std::cerr << "solution a = " << solution[it->second.second] << std::endl;
// 		}
// 			//throw new std::runtime_error("Cover and add labels in solution 
// 			//don't add up!");
// 	}
// }

// } // end namespace covertree
