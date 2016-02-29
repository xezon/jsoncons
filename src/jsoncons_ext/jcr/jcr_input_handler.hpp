// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_INPUT_HANDLER_HPP
#define JSONCONS_JCR_JCR_INPUT_HANDLER_HPP

#include <string>
#include "jsoncons/jsoncons.hpp"
#include "jsoncons/parse_error_handler.hpp"

namespace jsoncons { namespace jcr {

template <typename CharT>
class basic_jcr_input_handler
{
public:
    virtual ~basic_jcr_input_handler() {}

    void begin_json()
    {
        do_begin_json();
    }

    void end_json()
    {
        do_end_json();
    }

    void begin_object(const basic_parsing_context<CharT>& context)
    {
        do_begin_object(context);
    }

    void end_object(const basic_parsing_context<CharT>& context)
    {
        do_end_object(context);
    }

    void begin_array(const basic_parsing_context<CharT>& context)
    {
        do_begin_array(context);
    }

    void end_array(const basic_parsing_context<CharT>& context)
    {
        do_end_array(context);
    }

    void name(const std::basic_string<CharT>& name, const basic_parsing_context<CharT>& context)
    {
        do_name(name.data(), name.length(), context);
    }

    void name(const CharT* p, size_t length, const basic_parsing_context<CharT>& context) 
    {
        do_name(p, length, context);
    }

    void value(const std::basic_string<CharT>& value, const basic_parsing_context<CharT>& context) 
    {
        do_string_value(value.data(), value.length(), context);
    }

    void value(const CharT* p, size_t length, const basic_parsing_context<CharT>& context) 
    {
        do_string_value(p, length, context);
    }

    void value(const CharT* p, const basic_parsing_context<CharT>& context) 
    {
        do_string_value(p, std::char_traits<CharT>::length(p), context);
    }

    void value(int value, const basic_parsing_context<CharT>& context) 
    {
        do_integer_value(value,context);
    }

    void value(long value, const basic_parsing_context<CharT>& context) 
    {
        do_integer_value(value,context);
    }

    void value(long long value, const basic_parsing_context<CharT>& context) 
    {
        do_integer_value(value,context);
    }

    void range_value(long long from, long long to, const basic_parsing_context<CharT>& context) 
    {
        do_integer_range_value(from, to, context);
    }

    void range_value(unsigned long long from, unsigned long long to, const basic_parsing_context<CharT>& context) 
    {
        do_uinteger_range_value(from, to, context);
    }

    void value(unsigned long value, const basic_parsing_context<CharT>& context) 
    {
        do_uinteger_value(value,context);
    }

    void value(unsigned long long value, const basic_parsing_context<CharT>& context) 
    {
        do_uinteger_value(value,context);
    }

    void value(float value, uint8_t precision, const basic_parsing_context<CharT>& context)
    {
        do_double_value(value, precision, context);
    }

    void value(double value, uint8_t precision, const basic_parsing_context<CharT>& context)
    {
        do_double_value(value, precision, context);
    }

    void value(bool value, const basic_parsing_context<CharT>& context) 
    {
        do_bool_value(value,context);
    }

    void value(null_type, const basic_parsing_context<CharT>& context)
    {
        do_null_value(context);
    }

private:
    virtual void do_begin_json() = 0;

    virtual void do_end_json() = 0;

    virtual void do_begin_object(const basic_parsing_context<CharT>& context) = 0;

    virtual void do_end_object(const basic_parsing_context<CharT>& context) = 0;

    virtual void do_begin_array(const basic_parsing_context<CharT>& context) = 0;

    virtual void do_end_array(const basic_parsing_context<CharT>& context) = 0;

    virtual void do_name(const CharT* name, size_t length, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_null_value(const basic_parsing_context<CharT>& context) = 0;

    virtual void do_string_value(const CharT* value, size_t length, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_double_value(double value, uint8_t precision, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_integer_value(int64_t value, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_uinteger_value(uint64_t value, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_integer_range_value(int64_t from, int64_t to, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_uinteger_range_value(uint64_t from, uint64_t to, const basic_parsing_context<CharT>& context) = 0;

    virtual void do_bool_value(bool value, const basic_parsing_context<CharT>& context) = 0;
};

typedef basic_jcr_input_handler<char> jcr_input_handler;
typedef basic_jcr_input_handler<wchar_t> wjcr_input_handler;

}}

#endif
