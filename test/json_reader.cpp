#define BOOST_TEST_MODULE simplest_example

#include <fstream>
#include <json/json.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( json_test )
{
	Json::Value root;
	std::ifstream doc("example.json");
	BOOST_CHECK(doc.good());
	doc >> root;

	std::string author = root["author"].asString();
	std::string date = root["date"].asString();
	BOOST_CHECK_EQUAL(author, "carsten");

	std::cout << "Found Project created by " << author << " on " << date << std::endl;

	const Json::Value segmentationHypotheses = root["segmentation-hypotheses"];
	std::cout << "\tcontains " << segmentationHypotheses.size() << " segmentation hypotheses" << std::endl;
	BOOST_CHECK_EQUAL(segmentationHypotheses.size(), 3);
	for(int i = 0; i < (int)segmentationHypotheses.size(); i++)
	{
		const Json::Value hyp = segmentationHypotheses[i];
		BOOST_CHECK(hyp.isObject());
		BOOST_CHECK(hyp.isMember("id"));
		BOOST_CHECK(hyp.isMember("probability"));
		std::cout << "\t\tHypothesis " << i << " has id " << hyp["id"].asInt() << " and prob " << hyp["probability"].asFloat() << std::endl; 
	}

	const Json::Value linkingHypotheses = root["linking-hypotheses"];
	std::cout << "\tcontains " << linkingHypotheses.size() << " linking hypotheses" << std::endl;
	BOOST_CHECK_EQUAL(linkingHypotheses.size(), 2);
	for(int i = 0; i < (int)linkingHypotheses.size(); i++)
	{
		const Json::Value hyp = linkingHypotheses[i];
		BOOST_CHECK(hyp.isObject());
		BOOST_CHECK(hyp.isMember("src"));
		BOOST_CHECK(hyp.isMember("dest"));
		BOOST_CHECK(hyp.isMember("probability"));
		std::cout << "\t\tHypothesis " << i << " links " << hyp["src"].asInt() << " and " << hyp["dest"] << " with prob " << hyp["probability"].asFloat() << std::endl; 
	}

	const Json::Value exclusions = root["exclusions"];
	std::cout << "\tcontains " << exclusions.size() << " exclusions" << std::endl;
	BOOST_CHECK_EQUAL(exclusions.size(), 1);
	for(int i = 0; i < (int)exclusions.size(); i++)
	{
		const Json::Value hyp = exclusions[i];
		BOOST_CHECK(hyp.isArray());
		BOOST_CHECK_EQUAL(hyp.size(),2);
		BOOST_CHECK_EQUAL(hyp[0].asInt(), 7);
		BOOST_CHECK_EQUAL(hyp[1].asInt(), 9);
	}
}