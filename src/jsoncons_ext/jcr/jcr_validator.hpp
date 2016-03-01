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
#include "jsoncons/json.hpp"
#include "jcr_deserializer.hpp"
#include "jcr_parser.hpp"
#include "jcr_structures.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif

namespace jsoncons { namespace jcr {

template <typename CharT,class T> inline
void serialize(basic_json_output_handler<CharT>& os, const T&)
{
    os.value(null_type());
}

template <typename JsonT>
class basic_jcr_validator;

enum class value_types : uint8_t 
{
    // Simple types
    empty_object_t,
    null_t,
    // Non simple types
    rule_t,
    object_t,
    array_t
};

inline
bool is_simple(value_types type)
{
    return type < value_types::rule_t;
}

template <class JsonT>
class basic_jcr_validator
{
public:
    typedef JsonT json_type;

    typedef typename JsonT::allocator_type allocator_type;

    typedef typename JsonT::char_type char_type;
    typedef typename JsonT::char_traits_type char_traits_type;

    typedef typename JsonT::string_allocator string_allocator;
    typedef typename JsonT::string_type string_type;
    typedef basic_jcr_validator<JsonT> value_type;

    typedef typename rule<JsonT> rule_type;

    typedef name_value_pair<string_type,std::shared_ptr<rule_type>> member_type;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<JsonT> array_allocator;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<member_type> object_allocator;

    typedef array_rule<json_type,array_allocator> array;
    typedef object_rule<string_type,json_type,object_allocator>  object;

    typedef jsoncons::null_type null_type;

    typedef typename object::iterator object_iterator;
    typedef typename object::const_iterator const_object_iterator;
    typedef typename array::iterator array_iterator;
    typedef typename array::const_iterator const_array_iterator;

    class any_object_rule : public rule<JsonT>
    {
    public:
        any_object_rule()
        {
        }

        rule<JsonT>* clone() const override
        {
            return new any_object_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_object();
        }
    };

    class any_integer_rule : public rule<JsonT>
    {
    public:
        any_integer_rule()
        {
        }

        rule<JsonT>* clone() const override
        {
            return new any_integer_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() || val.as_uinteger();
        }
    };

    class string_rule : public rule<JsonT>
    {
        string_type s_;
    public:
        string_rule(const char_type* p, size_t length, string_allocator sa)
            : s_(p,length,sa)
        {
        }
        string_rule(const string_type& s)
            : s_(s)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new string_rule(s_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_string() && val.as_string() == s_;
        }
    };

    class any_string_rule : public rule<JsonT>
    {
    public:
        any_string_rule()
        {
        }

        rule<JsonT>* clone() const override
        {
            return new any_string_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_string();
        }
    };

    class null_rule : public rule<JsonT>
    {
    public:
        null_rule()
        {
        }

        rule<JsonT>* clone() const override
        {
            return new null_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_null();
        }
    };

    class bool_rule : public rule<JsonT>
    {
        bool val_;

    public:
        bool_rule(bool val)
            : val_(val)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new bool_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_bool() && val.as_bool() == val_;
        }
    };

    class double_rule : public rule<JsonT>
    {
        double val_;
        uint8_t precision_;

    public:
        double_rule(double val, uint8_t precision)
            : val_(val), precision_(precision)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new double_rule(val_,precision_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_double() && val.as_double() == val_;
        }
    };

    class integer_rule : public rule<JsonT>
    {
        int64_t val_;

    public:
        integer_rule(int64_t val)
            : val_(val)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new integer_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() && val.as_integer() == val_;
        }
    };

    class uinteger_rule : public rule<JsonT>
    {
        uint64_t val_;
        uint64_t to_;

    public:
        uinteger_rule(uint64_t val)
            : val_(val)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new uinteger_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_uinteger() && val.as_uinteger() == val_;
        }
    };

    class integer_range_rule : public rule<JsonT>
    {
        int64_t from_;
        int64_t to_;

    public:
        integer_range_rule(int64_t from, int64_t to)
            : from_(from), to_(to)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new integer_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() && val.as_integer() >= from_ && val.as_integer() <= to_;
        }
    };

    class uinteger_range_rule : public rule<JsonT>
    {
        uint64_t from_;
        uint64_t to_;
    public:
        uinteger_range_rule(uint64_t from, uint64_t to)
            : from_(from), to_(to)
        {
        }

        rule<JsonT>* clone() const override
        {
            return new uinteger_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_uinteger() && val.as_uinteger() >= from_ && val.as_uinteger() <= to_;
        }
    };

    std::shared_ptr<rule<JsonT>> rule_val_;

    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is);
    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is, basic_parse_error_handler<char_type>& err_handler);

    static basic_jcr_validator parse(const string_type& s)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<char_type> parser(handler);
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

    static basic_jcr_validator parse(const string_type& s, basic_parse_error_handler<char_type>& err_handler)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<char_type> parser(handler,err_handler);
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
        return *this;
    }

    basic_jcr_validator<JsonT>& operator=(std::shared_ptr<rule<JsonT>> val)
    {
        rule_val_ = val;
        return *this;
    }

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return rule_val_->is_object();
    }

    void swap(basic_jcr_validator& b)
    {
        rule_val_.swap(b.rule_val_);
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
        return rule_val_->validate(val);
    }

private:
};

template <class JsonT>
void swap(typename JsonT::member_type& a, typename JsonT::member_type& b)
{
    a.swap(b);
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_stream(std::basic_istream<char_type>& is)
{
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    basic_json_reader<char_type> reader(is, handler);
    reader.read_next();
    reader.check_done();
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

            basic_jcr_parser<char_type> parser(handler);
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

            basic_jcr_parser<char_type> parser(handler,err_handler);
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
