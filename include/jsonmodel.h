#ifndef JSON_MODEL_H
#define JSON_MODEL_H

#include <memory>
#include <vector>
#include <map>

#include "model.h"

namespace mht
{

/**
 * @brief Model specialized for Json loading and writing
 * @detail WARNING: at the moment you can only run either learn or infer once on the model. 
 *         Build a new one if you need it multiple times
 */
class JsonModel : public Model
{
public: 
    /**
     * @brief Read a model consisting of segmentation hypotheses and linking hypotheses from a json file
     * @param filename
     */
    void readFromJson(const std::string& filename);

    /**
     * @brief Export a found solution vector as a readable json file
     * 
     * @param filename where to save the result
     * @param sol the labeling to save
     */
    void saveResultToJson(const std::string& filename, const helpers::Solution& sol) const;

    /**
     * @brief Read in a ground truth solution (a boolean value per link) from a json file
     * 
     * @param filename where to find the ground truth
     * @return the solution as a vector of per-opengm-variable labelings
     */
    helpers::Solution readGTfromJson(const std::string& filename);
};

} // end namespace mht

#endif // JSON_MODEL_H
