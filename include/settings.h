#ifndef SETTINGS_H
#define SETTINGS_H

#include "helpers.h"
#include <json/json.h>

namespace helpers
{

class Settings
{
public:
	/**
	 * @brief Default constructor
	 */
	Settings();

	/**
	 * @brief Create a settings object by loading it from a JSON file
	 */
	Settings(const Json::Value& entry);

	// TODO: implement constructor that can initialize all values from code
	
	/**
	 * @brief Store the current values to the given JSON entry
	 */
	void saveToJson(Json::Value& entry);

	/**
	 * @brief Prints the settings to std::cout
	 */
	void print();

public: // settings object makes these parameters public instead of writing tons of getters and setters
	bool statesShareWeights_; // default = false
	bool allowPartialMergerAppearance_; // default = true
	bool requireSeparateChildrenOfDivision_; // default = false
	double optimizerEpGap_; // default = 0.01
	bool optimizerVerbose_; // default = true
	size_t optimizerNumThreads_; // default = 1, use 0 for all CPU cores
};

} // end namespace helpers

#endif // SETTINGS_H
