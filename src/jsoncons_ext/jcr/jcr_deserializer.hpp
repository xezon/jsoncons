// Copyright 2013-2016 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_DESERIALIZER_HPP
#define JSONCONS_JCR_JCR_DESERIALIZER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <istream>
#include <cstdlib>
#include <memory>
#include "jsoncons/jsoncons.hpp"
#include "jcr_input_handler.hpp"
#include "jcr_rules.hpp"

namespace jsoncons { namespace jcr {

template <class ValT>
class basic_jcr_deserializer : public basic_jcr_input_handler<typename ValT::rule_type>
{
    typedef typename ValT::char_type char_type;
    typedef typename ValT::string_type string_type;
    typedef typename ValT::allocator_type allocator_type;
    typedef typename ValT::json_type json_type;
    typedef typename std::shared_ptr<typename ValT::rule_type> value_type;

    ValT result_;
    bool is_valid_;

public:
    basic_jcr_deserializer()
        : is_valid_(true) // initial json value is an empty object

    {
    }

    bool is_valid() const
    {
        return is_valid_;
    }

    ValT get_result()
    {
        is_valid_ = false;
        return std::move(result_);
    }

private:

    void do_rule_definition(value_type rule, const basic_parsing_context<char_type>&) override
    {
        result_.set_root(rule);
    }

    void do_named_rule(const string_type& name, value_type rule, const basic_parsing_context<char_type>&) override
    {
        result_.add_named_rule(name,rule);
    }
};

}}

#endif
