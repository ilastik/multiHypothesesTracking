#ifndef PYTHON_MODEL_H
#define PYTHON_MODEL_H

#include <boost/python.hpp>

#include <memory>
#include <vector>
#include <map>
#include <tuple>

#include "model.h"
#include "helpers.h"

namespace mht
{

/**
 * @brief read weights from a python dictionary
 * 
 * @param weightsDict
 * @return a vector of weights
 */
helpers::FeatureVector readWeightsFromPython(boost::python::dict& weightsDict);


/**
 * @brief Model specialized for Python loading and writing
 * @detail WARNING: at the moment you can only run either learn or infer once on the model. 
 *         Build a new one if you need it multiple times
 */
class PythonModel : public Model
{
public: 
    /**
     * @brief Read a model consisting of segmentation hypotheses and linking hypotheses from a json file
     * @param filename
     */
    void readFromPython(boost::python::dict& graphDict);

    /**
     * @brief Export a found solution vector as a python dictionary
     * 
     * @param sol the labeling to save
     */
    boost::python::dict saveResultToPython(const helpers::Solution& sol) const;

    /**
     * @brief Read in a ground truth solution (a boolean value per link) from a json file
     * 
     * @param filename where to find the ground truth
     * @return the solution as a vector of per-opengm-variable labelings
     */
    void setPythonGtFile(const std::string& filename);

    /**
     * @brief get the ground truth for learning from a JSON file
     * @return the solution vector that fits the initialized OpenGM model
     */
    virtual helpers::Solution getGroundTruth();

private:
    /**
     * @brief read linking hypothesis from Python and adds it to linkingHypotheses_
     * @details expects the json value to contain attributes "src"(helpers::IdLabelType), 
     *  "dest"(helpers::IdLabelType), and "features"(list of double)
     * 
     * @param entry json object for this hypothesis
     */
    void readLinkingHypothesis(boost::python::dict& entry);

    /**
     * @brief read segmentation hypothesis from Python and adds it to segmentationHypotheses_
     * @details expects the json value to contain attributes "id"(helpers::IdLabelType) and "features"(list of double),
     *          as well as "divisionFeatures", "appearanceFeatures" and "disappearanceFeatures", where
     *          the presence of the latter two toggles the presence of an appearance or disappearance node.
     *          Hypotheses which do not have these, are not allowed to appear/disappear!
     * 
     * @param entry json object for this hypothesis
     */
    void readSegmentationHypothesis(boost::python::dict& entry);

    /**
     * @brief read division hypothesis from Python
     *
     * @param entry json object for this hypothesis
     */
    void readDivisionHypothesis(boost::python::dict& entry);

    /**
     * @brief read exclusion constraint from Python
     * @details expects the json array to be a list of ints representing ids
     * 
     * @param entry json object for this hypothesis
     */
    void readExclusionConstraints(boost::python::list& entry);

    /**
     * @brief Create a json string describing this link with its value (for result saving)
     * 
     * @param state the state that this link has (will be saved as "value" in JSON)
     * @return the Python value to put in an array into the result file
     */
    boost::python::dict linkToPython(const std::shared_ptr<LinkingHypothesis>& link, size_t state) const;

    /**
     * @brief Create a json string describing this division with its value (for result saving)
     * 
     * @param state the state that this division has (will be saved as "value" in JSON)
     * @return the Python value to put in an array into the result file
     */
    boost::python::dict divisionToPython(const std::shared_ptr<DivisionHypothesis>& division, size_t state) const;

    /**
     * @brief Create json value containing the state of this division, linked to this detection's id
     */
    boost::python::dict divisionToPython(const SegmentationHypothesis& segmentation, size_t value) const;

    /**
     * @brief Create json value containing the state of this detection
     */
    boost::python::dict detectionToPython(const SegmentationHypothesis& segmentation, size_t value) const;

    /**
     * @brief Extract a state feature vector for a given type of features from a python dictionary
     */
    helpers::StateFeatureVector extractFeatures(boost::python::dict& entry, helpers::JsonTypes type);

private:
    // ground truth filename
    std::string groundTruthFilename_;
};

} // end namespace mht

#endif // PYTHON_MODEL_H
