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
#include "jsoncons_ext/jcr/jcr_validator.hpp"

using namespace jsoncons;
using namespace jsoncons::jcr;

BOOST_AUTO_TEST_SUITE(jcr_test_suite)

BOOST_AUTO_TEST_CASE(test_jcr)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    json val1 = json::parse(R"(
    {
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");
    json val2 = json::parse(R"(
    {
        "line-count" : 3426,
        "word-count" : 27887
    }
    )");

    BOOST_CHECK(schema.validate(val1));
    BOOST_CHECK(!schema.validate(val2));
}

BOOST_AUTO_TEST_CASE(test_jcr_integer)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        "line-count" : integer,
        "word-count" : integer
    }
    )");

    json val1 = json::parse(R"(
    {
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_jcr_integer_range)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        "line-count" : 0..,
        "word-count" : 0..
    }
    )");

    json val1 = json::parse(R"(
    {
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));

    jcr_validator schema2 = jcr_validator::parse(R"(
    {
        "line-count" : 3427..,
        "word-count" : 0..
    }
    )");

    BOOST_CHECK(!schema2.validate(val1));
    
}

BOOST_AUTO_TEST_CASE(test_jcr_string)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        "file-name"  : string,
        "line-count" : 0..,
        "word-count" : 0..
    }
    )");

    json val1 = json::parse(R"(
    {
        "file-name"  : "rfc7159.txt",
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_named_rules)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        fn,
        lc,
        wc
    }
    fn "file-name"  : string
    lc "line-count" : 0..
    wc "word-count" : 0..    
    )");

    json val1 = json::parse(R"(
    {
        "file-name"  : "rfc7159.txt",
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));
} 
 
BOOST_AUTO_TEST_CASE(test_named_rules2)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        fn
    }
    fn "file-name"  : "rfc7159.txt"
    )");

    json val1 = json::parse(R"(
    {
        "file-name"  : "rfc7159.txt",
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_named_rules3)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        fn,
        lc,
        wc
    }
    fn "file-name"  : "rfc7159.txt"
    lc "line-count" : 3426
    wc "word-count" : 27886
    )");

    json val1 = json::parse(R"(
    {
        "file-name"  : "rfc7159.txt",
        "line-count" : 3426,
        "word-count" : 27886
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_member_range_value_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        v1
    }
    v1 "value"  : 0..3
    )");

    json val1 = json::parse(R"(
    {
        "value"  : 1
    }
    )");

    json val2 = json::parse(R"(
    {
        "value"  : -1
    }
    )");

    json val3 = json::parse(R"(
    {
        "value"  : 4
    }
    )");

    BOOST_CHECK(schema.validate(val1));
    BOOST_CHECK(!schema.validate(val2));
    BOOST_CHECK(!schema.validate(val3));
}

BOOST_AUTO_TEST_CASE(test_range_value_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        m1
    }
    v1 : 0..3
    m1 "value" : v1
    )");

    json val1 = json::parse(R"(
    {
        "value"  : 1
    }
    )");

    json val2 = json::parse(R"(
    {
        "value"  : -1
    }
    )");

    json val3 = json::parse(R"(
    {
        "value"  : 4
    }
    )");

    BOOST_CHECK(schema.validate(val1));
    BOOST_CHECK(!schema.validate(val2));
    BOOST_CHECK(!schema.validate(val3));
}
BOOST_AUTO_TEST_CASE(test_optional_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {o1}
    v1 : 0..3
    m1 "m1" : v1
    m2 "m2" : v1
    o1 "m0" : { m1, m2 }
    )");

    json val1 = json::parse(R"(
    {
        "m0" : {"m1":1,"m2":2}
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_optional_member_optional_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {o1}
    v1 : 0..3
    m1 "m1" : v1
    m2 "m2" : v1
    o1 "m0" : { m1, ?m2 }
    )");

    json val1 = json::parse(R"(
    {
        "m0" : {"m1":1,"m2":2}
    }
    )");

    BOOST_CHECK(schema.validate(val1));
}
BOOST_AUTO_TEST_CASE(test_array_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    [o1]
    v1 : 0..3
    m1 "m1" : v1
    m2 "m2" : v1
    o1 : { m1, ?m2 }
    )");

    json val1 = json::parse(R"(
    [
        {"m1":1,"m2":2}
    ]
    )");

    BOOST_CHECK(schema.validate(val1));

    json val2 = json::parse(R"(
    [
        {"m2":2}
    ]
    )");

    BOOST_CHECK(!schema.validate(val2));

    json val3 = json::parse(R"(
    [
        {"m1":1}
    ]
    )");

    BOOST_CHECK(schema.validate(val3));

    json val4 = json::parse(R"(
    [
        {"m1":-1}
    ]
    )");

    BOOST_CHECK(!schema.validate(val4));
}

BOOST_AUTO_TEST_CASE(test_nested_object_rules)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {"n1" : { m1, m2 }}
    v1 : 0..3
    m1 "m1" : v1
    m2 "m2" : v1
    o1 "n1" : { m1, m2 }
    )");

    json val1 = json::parse(R"(
    {
        "n1" : {"m1":1,"m2":2}
    }
    )");
    BOOST_CHECK(schema.validate(val1));

    json val2 = json::parse(R"(
    {
        "n1" : {"m1":1,"m2":4}
    }
    )");
    BOOST_CHECK(!schema.validate(val2));
}

BOOST_AUTO_TEST_CASE(test_nested_array_rules)
{
    jcr_validator schema = jcr_validator::parse(R"(
    [v1, [ v1, v2 ]]
    v1 : 0..3
    v2 : 4..7
    )");

    json val1 = json::parse(R"(
    [
        1, [2,5]
    ]
    )");
    BOOST_CHECK(schema.validate(val1));

    json val2 = json::parse(R"(
    [
        1, [4,5]
    ]
    )");
    BOOST_CHECK(!schema.validate(val2));
}

BOOST_AUTO_TEST_CASE(test_example)
{
    jcr_validator schema = jcr_validator::parse(R"(
     { image }

       image "Image" : {
           width, height, "Title" : string,
           thumbnail, "IDs" : [ *integer ]
       }

       thumbnail "Thumbnail" : {
           width, height, "Url" : uri
       }

       width "Width" : width_v
       height "Height" : height_v

       width_v : 0..1280
       height_v : 0..1024
    )");

    json val1 = json::parse(R"(
       {
         "Image": {
             "Width":  800,
             "Height": 600,
             "Title":  "View from 15th Floor",
             "Thumbnail": {
                 "Url":    "http://www.example.com/image/481989943",
                 "Height": 125,
                 "Width":  100
             },
             "IDs": [116, 943, 234, 38793]
          }
       }
    )");
    BOOST_CHECK(schema.validate(val1));
}

BOOST_AUTO_TEST_CASE(test_boolean_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    {
        "FistName" : string,
        "LastName" : string,
        "IsRetired" : boolean,
        "Income" : float
    }
    )");

    json val1 = json::parse(R"(
    {
        "FistName" : "John",
        "LastName" : "Smith",
        "IsRetired" : false,
        "Income" : 100000.00
    }
    )");

    BOOST_CHECK(schema.validate(val1));
} 

BOOST_AUTO_TEST_CASE(test_repeating_array_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
    [v1,*o1]
    v1 : 0..3
    m1 "m1" : v1
    m2 "m2" : v1
    o1 : { m1, ?m2 }
    )");

    json val1 = json::parse(R"(
    [
        0,{"m1":1,"m2":2}
    ]
    )");
    BOOST_CHECK(schema.validate(val1));

    json val2 = json::parse(R"(
    [
        0,{"m2":2}
    ]
    )");
    BOOST_CHECK(!schema.validate(val2));

    json val3 = json::parse(R"(
    [
        0,{"m1":1}
    ]
    )");
    BOOST_CHECK(schema.validate(val3));

    json val4 = json::parse(R"(
    [
        0,{"m1":-1}
    ]
    )");
    BOOST_CHECK(!schema.validate(val4));

    json val5 = json::parse(R"(
    [
        0,{"m1":1},{"m1":3}
    ]
    )");
    BOOST_CHECK(schema.validate(val5));

    json val6 = json::parse(R"(
    [
        0,{"m1":1},{"m1":5}
    ]
    )");
    BOOST_CHECK(!schema.validate(val6));
}

BOOST_AUTO_TEST_CASE(test_group_rule)
{
    jcr_validator schema = jcr_validator::parse(R"(
        [ parents, children ]

        children ( :"Greg", :"Marsha", :"Bobby", :"Jan" )
        parents ( :"Mike", :"Carol" )
    )");

    json val1 = json::parse(R"(
        ["Mike", "Carol", "Greg", "Marsha", "Bobby", "Jan"]
    )");

    BOOST_CHECK(schema.validate(val1));
}
/*
BOOST_AUTO_TEST_CASE(test_group_rule2)
{
    jcr_validator schema = jcr_validator::parse(R"(
        @(root) the_bradys : [ parents, children ]
        children ( :"Greg", :"Marsha", :"Bobby", :"Jan" )
        parents ( :"Mike", :"Carol" )
    )");

    json val1 = json::parse(R"(
        ["Mike", "Carol", "Greg", "Marsha", "Bobby", "Jan"]
    )");

    BOOST_CHECK(schema.validate(val1));
}
*/
BOOST_AUTO_TEST_SUITE_END()
