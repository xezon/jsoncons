// Copyright 2013 Daniel Parker
// Distributed under Boost license

#ifdef __linux__
#define BOOST_TEST_DYN_LINK
#endif

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <new>
#include "jsoncons/json.hpp"
#include "jsoncons_ext/jsonpath/json_query_set.hpp"

using namespace jsoncons;
using namespace jsoncons::jsonpath;

BOOST_AUTO_TEST_SUITE(json_query_set_test_suite)


BOOST_AUTO_TEST_CASE(test_query_set)
{
    json val1 = json::array{1,2,3,4};
    json val2 = json::object{{"first", 1},{"second", 2}};
    std::vector<json*> v = {&val1,&val2};
    
    json_query_set<json> qs(std::move(v));

    std::cout << pretty_print(qs) << std::endl;

    //json j = qs;

    //std::cout << pretty_print(j) << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()




