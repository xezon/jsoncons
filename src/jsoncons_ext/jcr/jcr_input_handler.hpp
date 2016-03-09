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

template <typename RuleT>
class basic_jcr_input_handler
{
public:
    typedef typename RuleT rule_type;
    typedef typename RuleT::string_type string_type;
    typedef typename string_type::value_type char_type;

    virtual ~basic_jcr_input_handler() {}

    void begin_json()
    {
        do_begin_json();
    }

    void end_json()
    {
        do_end_json();
    }

    void begin_object(const basic_parsing_context<char_type>& context)
    {
        do_begin_object(context);
    }

    void end_object(const basic_parsing_context<char_type>& context)
    {
        do_end_object(context);
    }

    void begin_array(const basic_parsing_context<char_type>& context)
    {
        do_begin_array(context);
    }

    void end_array(const basic_parsing_context<char_type>& context)
    {
        do_end_array(context);
    }

    void rule_name(const char_type* p, size_t length, const basic_parsing_context<char_type>& context) 
    {
        do_rule_name(p, length, context);
    }

    void rule_definition(std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) 
    {
        do_rule_definition(rule, context);
    }

    void named_rule(const string_type& name, std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) 
    {
        do_named_rule(name, rule, context);
    }

private:
    virtual void do_begin_json() = 0;

    virtual void do_end_json() = 0;

    virtual void do_begin_object(const basic_parsing_context<char_type>& context) = 0;

    virtual void do_end_object(const basic_parsing_context<char_type>& context) = 0;

    virtual void do_begin_array(const basic_parsing_context<char_type>& context) = 0;

    virtual void do_end_array(const basic_parsing_context<char_type>& context) = 0;

    virtual void do_rule_name(const char_type* value, size_t length, const basic_parsing_context<char_type>& context) = 0;

    virtual void do_rule_definition(std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) = 0;

    virtual void do_named_rule(const string_type& name, std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) = 0;
};

}}

#endif
