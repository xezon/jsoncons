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
#include "jsoncons_ext/jcr/jcr_rules.hpp"

using namespace jsoncons;
using namespace jsoncons::jcr;

BOOST_AUTO_TEST_SUITE(jcr_uri_rule_tests)

BOOST_AUTO_TEST_CASE(test_good_uri)
{
    auto rule_ptr = std::make_shared<uri_rule<json>>();
    std::map<std::string,std::shared_ptr<rule<json>>> named_rules;

    json uri1 = "ftp://ftp.is.co.za/rfc/rfc1808.txt";
    BOOST_ASSERT(rule_ptr->validate(uri1, false, named_rules));

    json uri2 = "http://www.ietf.org/rfc/rfc2396.txt";
    BOOST_ASSERT(rule_ptr->validate(uri2, false, named_rules));

    json uri3 = "ldap://[2001:db8::7]/c=GB?objectClass?one";
    BOOST_ASSERT(rule_ptr->validate(uri3, false, named_rules));

    json uri4 = "mailto:John.Doe@example.com";
    BOOST_ASSERT(rule_ptr->validate(uri4, false, named_rules));

    json uri5 = "news:comp.infosystems.www.servers.unix";
    BOOST_ASSERT(rule_ptr->validate(uri5, false, named_rules));

    json uri6 = "tel:+1-816-555-1212";
    BOOST_ASSERT(rule_ptr->validate(uri6, false, named_rules));

    json uri7 = "telnet://192.0.2.16:80/";
    BOOST_ASSERT(rule_ptr->validate(uri7, false, named_rules));

    json uri8 = "urn:oasis:names:specification:docbook:dtd:xml:4.1.2";
    BOOST_ASSERT(rule_ptr->validate(uri8, false, named_rules));
}

BOOST_AUTO_TEST_CASE(test_bad_uri)
{
    auto rule_ptr = std::make_shared<uri_rule<json>>();
    std::map<std::string,std::shared_ptr<rule<json>>> named_rules;

    json uri1 = "{/id*";
    BOOST_ASSERT(!rule_ptr->validate(uri1, false, named_rules));
}

BOOST_AUTO_TEST_SUITE_END()
