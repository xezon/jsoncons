// Copyright 2016 Daniel Parker
// Distributed under Boost license

#ifdef __linux__
#define BOOST_TEST_DYN_LINK
#endif

#include <boost/test/unit_test.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jcr/json_content_rules.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <limits>

using namespace jsoncons;
using namespace jsoncons::jcr;

BOOST_AUTO_TEST_SUITE(jcr_tests)

BOOST_AUTO_TEST_CASE(jcr_parse_test)
{
    std::string s = R"(
        { "line-count" : 3426, "word-count" : 27886 }
    )";

    json_content_rules rules = json_content_rules::parse(s);
}

BOOST_AUTO_TEST_SUITE_END()

