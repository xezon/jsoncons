// Copyright 201r3 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_HPP
#define JSONCONS_JCR_JCR_HPP

#include <limits>
#include <string>
#include <vector>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <memory>
#include <typeinfo>
#include <map>
#include "jsoncons/json.hpp"
#include "jcr_deserializer.hpp"
#include "jcr_parser.hpp"
#include "jcr_rules.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif

namespace jsoncons { namespace jcr {

template <class JsonT>
class basic_jcr_validator
{
    std::shared_ptr<rule<JsonT>> rule_val_;
public:
    typedef JsonT json_type;

    typedef typename JsonT::allocator_type allocator_type;

    typedef typename JsonT::char_type char_type;
    typedef typename JsonT::char_traits_type char_traits_type;

    typedef typename JsonT::string_allocator string_allocator;
    typedef typename JsonT::string_type string_type;
    typedef basic_jcr_validator<JsonT> value_type;

    typedef typename rule<JsonT> rule_type;
    std::map<string_type,std::shared_ptr<rule_type>> rule_definitions_;
    std::shared_ptr<rule_type> root_;

    //typedef name_value_pair<string_type,std::shared_ptr<rule_type>> member_type;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<rule_type> array_allocator;
    
    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<rule_type> object_allocator;

    void set_root(std::shared_ptr<rule<JsonT>> rule)
    {
        rule_val_ = rule;
    }

    void add_named_rule(const string_type& name, std::shared_ptr<rule<JsonT>> rule)
    {
        rule_definitions_[name] = rule;
    }

    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is);
    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is, basic_parse_error_handler<char_type>& err_handler);

    static basic_jcr_validator parse(const string_type& s)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<JsonT> parser(handler);
        parser.begin_parse();
        parser.parse(s.data(),0,s.length());
        parser.end_parse();
        //parser.check_done(s.data(),parser.index(),s.length());
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json string");
        }
        return handler.get_result();
    }

    static basic_jcr_validator parse(const string_type& s, basic_parse_error_handler<char_type>& err_handler)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<JsonT> parser(handler,err_handler);
        parser.begin_parse();
        parser.parse(s.data(),0,s.length());
        parser.end_parse();
        parser.check_done(s.data(),parser.index(),s.length());
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json string");
        }
        return handler.get_result();
    }

    static basic_jcr_validator parse_file(const std::string& s);

    static basic_jcr_validator parse_file(const std::string& s, basic_parse_error_handler<char_type>& err_handler);

    basic_jcr_validator() 
    {
    }

    basic_jcr_validator(const basic_jcr_validator<JsonT>& val)
    {
        rule_val_ = val.rule_val_;
        rule_definitions_ = val.rule_definitions_;
    }

    basic_jcr_validator(std::shared_ptr<rule<JsonT>>* rule)
        : rule_val_(rule)
    {
    }

    ~basic_jcr_validator()
    {
    }

    basic_jcr_validator& operator=(const value_type& rhs)
    {
        rule_val_ = rhs.rule_val_;
        rule_definitions_ = rhs.rule_definitions_;
        return *this;
    }

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return rule_val_->is_object();
    }

    void swap(basic_jcr_validator& b)
    {
        rule_val_.swap(b.rule_val_);
        rule_definitions_.swap(b.rule_definitions_);
    }

    friend void swap(JsonT& a, JsonT& b)
    {
        a.swap(b);
    }

    rule<JsonT>& array_value() 
    {
        return *rule_val_;
    }

    const rule<JsonT>& array_value() const
    {
        return *rule_val_;
    }

    rule<JsonT>& object_value()
    {
        return *rule_val_;
    }

    const rule<JsonT>& object_value() const
    {
        return *rule_val_;
    }

    bool validate(const JsonT& val) const
    {
        return rule_val_->validate(val, rule_definitions_);
    }

private:
};

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_stream(std::basic_istream<char_type>& is)
{
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    basic_json_reader<char_type> reader(is, handler);
    reader.read_next();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_stream(std::basic_istream<char_type>& is, 
                                                              basic_parse_error_handler<char_type>& err_handler)
{
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    basic_json_reader<char_type> reader(is, handler, err_handler);
    reader.read_next();
    reader.check_done();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_file(const std::string& filename)
{
    FILE* fp;

#if defined(JSONCONS_HAS_FOPEN_S)
    errno_t err = fopen_s(&fp, filename.c_str(), "rb");
    if (err != 0) 
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#else
    fp = std::fopen(filename.c_str(), "rb");
    if (fp == nullptr)
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#endif
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    try
    {
        // obtain file size:
        std::fseek (fp , 0 , SEEK_END);
        long size = std::ftell (fp);
        std::rewind(fp);

        if (size > 0)
        {
            std::vector<char_type> buffer(size);

            // copy the file into the buffer:
            size_t result = std::fread (buffer.data(),1,size,fp);
            if (result != static_cast<unsigned long long>(size))
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Error reading file %s", filename);
            }

            basic_jcr_parser<JsonT> parser(handler);
            parser.begin_parse();
            parser.parse(buffer.data(),0,buffer.size());
            parser.end_parse();
            parser.check_done(buffer.data(),parser.index(),buffer.size());
        }

        std::fclose (fp);
    }
    catch (...)
    {
        std::fclose (fp);
        throw;
    }
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json file");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_file(const std::string& filename, 
                                                            basic_parse_error_handler<char_type>& err_handler)
{
    FILE* fp;

#if !defined(JSONCONS_HAS_FOPEN_S)
    fp = std::fopen(filename.c_str(), "rb");
    if (fp == nullptr)
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#else
    errno_t err = fopen_s(&fp, filename.c_str(), "rb");
    if (err != 0) 
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#endif

    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    try
    {
        // obtain file size:
        std::fseek (fp , 0 , SEEK_END);
        long size = std::ftell (fp);
        std::rewind(fp);

        if (size > 0)
        {
            std::vector<char_type> buffer(size);

            // copy the file into the buffer:
            size_t result = std::fread (buffer.data(),1,size,fp);
            if (result != static_cast<unsigned long long>(size))
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Error reading file %s", filename);
            }

            basic_jcr_parser<JsonT> parser(handler,err_handler);
            parser.begin_parse();
            parser.parse(buffer.data(),0,buffer.size());
            parser.end_parse();
            parser.check_done(buffer.data(),parser.index(),buffer.size());
        }

        std::fclose (fp);
    }
    catch (...)
    {
        std::fclose (fp);
        throw;
    }
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json file");
    }
    return handler.get_result();
}

template <typename JsonT>
std::basic_istream<typename JsonT::char_type>& operator>>(std::basic_istream<typename JsonT::char_type>& is, JsonT& o)
{
    basic_jcr_deserializer<JsonT> handler;
    basic_json_reader<typename JsonT::char_type> reader(is, handler);
    reader.read_next();
    reader.check_done();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    o = handler.get_result();
    return is;
}

typedef basic_jcr_validator<json> jcr_validator;
typedef basic_jcr_validator<wjson> wjcr_validator;

typedef basic_jcr_deserializer<jcr_validator> jcr_deserializer;
typedef basic_jcr_deserializer<wjcr_validator> wjcr_deserializer;

}}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
