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
    static const int default_stack_size = 1000;

    typedef typename ValT::char_type char_type;
    typedef typename ValT::string_type string_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef typename ValT::allocator_type allocator_type;
    typedef typename ValT::array array;
    typedef typename array::allocator_type array_allocator;
    typedef typename ValT::object object;
    typedef typename ValT::json_type json_type;
    typedef typename object::allocator_type object_allocator;
    typedef typename std::shared_ptr<typename ValT::rule_type> value_type;

    string_allocator sa_;
    object_allocator oa_;
    array_allocator aa_;

    ValT result_;
    size_t top_;
    std::vector<value_type> stack_;
    std::vector<size_t> stack2_;
    bool is_valid_;

public:
    basic_jcr_deserializer(const string_allocator& sa = string_allocator(),
                            const allocator_type& allocator = allocator_type())
        : sa_(sa),
          oa_(allocator),
          aa_(allocator),
          top_(0),
          stack_(default_stack_size),
          stack2_(),
          is_valid_(true) // initial json value is an empty object

    {
        stack2_.reserve(100);
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

#if !defined(JSONCONS_NO_DEPRECATED)
    ValT& root()
    {
        return result_;
    }
#endif

private:

    void push_initial()
    {
        top_ = 0;
        if (top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_initial()
    {
        JSONCONS_ASSERT(top_ == 1);
        result_.set_rule(stack_[0]);
        --top_;
    }

    void push_object()
    {
        stack2_.push_back(top_);
        stack_[top_] = std::make_shared<object>();
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_object()
    {
        stack2_.pop_back();
        JSONCONS_ASSERT(top_ > 0);
    }

    void push_array()
    {
        stack2_.push_back(top_);
        stack_[top_] = std::make_shared<array>();
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_array()
    {
        stack2_.pop_back();
        JSONCONS_ASSERT(top_ > 0);
    }

    void do_begin_json() override
    {
        is_valid_ = false;
        push_initial();
    }

    void do_end_json() override
    {
        is_valid_ = true;
        pop_initial();
    }

    void do_begin_object(const basic_parsing_context<char_type>&) override
    {
        push_object();
    }

    void do_end_object(const basic_parsing_context<char_type>&) override
    {
        end_structure();
        pop_object();
    }

    void do_begin_array(const basic_parsing_context<char_type>&) override
    {
        push_array();
    }

    void do_end_array(const basic_parsing_context<char_type>&) override
    {
        end_structure();
        pop_array();
    }

    void end_structure() 
    {
        JSONCONS_ASSERT(stack2_.size() > 0);
        if (stack_[stack2_.back()]->is_object())
        {
            size_t count = top_ - (stack2_.back() + 1);
            auto s = stack_.begin() + (stack2_.back()+1);
            auto send = s + count;
            stack_[stack2_.back()]->insert(
                std::make_move_iterator(s),
                std::make_move_iterator(send));
            top_ -= count;
        }
        else
        {
            size_t count = top_ - (stack2_.back() + 1);
            auto s = stack_.begin() + (stack2_.back()+1);
            auto send = s + count;
            stack_[stack2_.back()]->insert(
                std::make_move_iterator(s),
                std::make_move_iterator(send));
            top_ -= count;
        }
    }

    void do_rule_name(const char_type* p, size_t length, const basic_parsing_context<char_type>&) override
    {
        stack_[top_] = std::make_shared<jcr_rule_name<json_type>>(p,length,sa_);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_rule_definition(value_type rule, const basic_parsing_context<char_type>& context) override
    {
        stack_[top_] = rule;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_named_rule(const string_type& name, value_type rule, const basic_parsing_context<char_type>& context) override
    {
        result_.add_named_rule(name,rule);
    }
};

}}

#endif
