// Copyright 2013 Daniel Parker
// Distributed under Boost license

#ifdef __linux__
#define BOOST_TEST_DYN_LINK
#endif

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <ctime>
#include <new>
#include <codecvt>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpatch/jsonpatch.hpp>

using namespace jsoncons;

BOOST_AUTO_TEST_SUITE(jsonpatch_tests)

void check_add(json& example, const std::string& path, const json& value, const json& expected)
{
    try
    {
        jsonpatch::add(example, path, value);
        BOOST_CHECK_EQUAL(expected, example);
    }
    catch (const parse_error& e)
    {
        std::cout << e.what() << ". " << path << std::endl;
    }
}

void check_replace(json& example, const std::string& path, const json& value, const json& expected)
{
    try
    {
        jsonpatch::replace(example, path, value);
        BOOST_CHECK_EQUAL(expected, example);
    }
    catch (const parse_error& e)
    {
        std::cout << e.what() << ". " << path << std::endl;
    }
}

void check_remove(json& example, const std::string& path, const json& expected)
{
    try
    {
        jsonpatch::remove(example, path);
        BOOST_CHECK_EQUAL(expected, example);
    }
    catch (const parse_error& e)
    {
        std::cout << e.what() << ". " << path << std::endl;
    }
}

void check_move(json& example, const std::string& from, const std::string& path, const json& expected)
{
    try
    {
        jsonpatch::move(example, from, path);
        BOOST_CHECK_EQUAL(expected, example);
    }
    catch (const parse_error& e)
    {
        std::cout << e.what() << ". " << path << std::endl;
    }
}

// add

BOOST_AUTO_TEST_CASE(test_add_object_member)
{
    json example = json::parse(R"(
    { "foo": "bar"}
    )");

    const json expected = json::parse(R"(
    { "foo": "bar", "baz" : "qux"}
    )");

    check_add(example,"/baz", json("qux"), expected);
}

BOOST_AUTO_TEST_CASE(test_add_array_element)
{
    json example = json::parse(R"(
    { "foo": [ "bar", "baz" ] }
    )");

    const json expected = json::parse(R"(
    { "foo": [ "bar", "qux", "baz" ] }
    )");

    check_add(example,"/foo/1", json("qux"), expected);
}

BOOST_AUTO_TEST_CASE(test_add_array_value)
{
    json example = json::parse(R"(
     { "foo": ["bar"] }
    )");

    const json expected = json::parse(R"(
    { "foo": ["bar", ["abc", "def"]] }
    )");

    check_add(example,"/foo/-", json::array({"abc", "def"}), expected);
}

// remove

BOOST_AUTO_TEST_CASE(test_remove_array_element)
{
    json example = json::parse(R"(
        { "foo": [ "bar", "qux", "baz" ] }
    )");

    const json expected = json::parse(R"(
        { "foo": [ "bar", "baz" ] }
    )");

    check_remove(example,"/foo/1", expected);
}

// replace

BOOST_AUTO_TEST_CASE(test_replace_value)
{
    json example = json::parse(R"(
        {
          "baz": "qux",
          "foo": "bar"
        }
    )");

    const json expected = json::parse(R"(
        {
          "baz": "boo",
          "foo": "bar"
        }
    )");

    check_replace(example,"/baz", json("boo"), expected);
}

// move
/*
BOOST_AUTO_TEST_CASE(test_move_value)
{
    json example = json::parse(R"(
        {
          "foo": {
            "bar": "baz",
            "waldo": "fred"
          },
          "qux": {
            "corge": "grault"
          }
        }
    )");

    const json expected = json::parse(R"(
        {
          "foo": {
            "bar": "baz"
          },
          "qux": {
            "corge": "grault",
            "thud": "fred"
          }
        }
    )");

    check_move(example, "/foo/waldo", "/qux/thud", expected);
}

BOOST_AUTO_TEST_CASE(test_move_array_element)
{
    json example = json::parse(R"(
        { "foo": [ "all", "grass", "cows", "eat" ] }
    )");

    const json expected = json::parse(R"(
        { "foo": [ "all", "cows", "eat", "grass" ] }
    )");

    check_move(example, "/foo/1", "/foo/3", expected);
}
*/

BOOST_AUTO_TEST_SUITE_END()



