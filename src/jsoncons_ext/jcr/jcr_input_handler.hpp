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
    typedef RuleT rule_type;
    typedef typename RuleT::string_type string_type;
    typedef typename string_type::value_type char_type;

    virtual ~basic_jcr_input_handler() {}

    void rule_definition(std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) 
    {
        do_rule_definition(rule, context);
    }

    void named_rule(const string_type& name, std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) 
    {
        do_named_rule(name, rule, context);
    }

private:
    virtual void do_rule_definition(std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) = 0;

    virtual void do_named_rule(const string_type& name, std::shared_ptr<rule_type> rule, const basic_parsing_context<char_type>& context) = 0;
};

}}

#endif
