// Copyright 2013-2016 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_DECODER_HPP
#define JSONCONS_JSON_DECODER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <istream>
#include <cstdlib>
#include <memory>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_input_handler.hpp>

namespace jsoncons {

template <class Json>
class json_decoder : public basic_json_input_handler<typename Json::char_type>
{
public:
    typedef typename Json::char_type char_type;
    using typename basic_json_input_handler<char_type>::string_view_type                                 ;

    static const int default_stack_size = 1000;

    typedef Json json_type;
    typedef typename Json::key_value_pair_type key_value_pair_type;
    typedef typename Json::string_type string_type;
    typedef typename Json::key_storage_type key_storage_type;
    typedef typename string_type::allocator_type char_allocator;
    typedef typename Json::allocator_type allocator_type;
    typedef typename Json::array array;
    typedef typename array::allocator_type array_allocator;
    typedef typename Json::object object;
    typedef typename object::allocator_type object_allocator;

    char_allocator sa_;
    object_allocator oa_;
    array_allocator aa_;

    Json result_;
    size_t top_;

    struct stack_item
    {
        size_t structure_index_;
        key_storage_type name_;
        Json value_;
    };
    std::vector<stack_item> stack_;
    bool is_valid_;

public:
    json_decoder(const char_allocator& sa = char_allocator(),
                            const allocator_type& allocator = allocator_type())
        : sa_(sa),
          oa_(allocator),
          aa_(allocator),
          top_(0),
          stack_(default_stack_size),
          is_valid_(false) 

    {
    }

    bool is_valid() const
    {
        return is_valid_;
    }

    Json get_result()
    {
        is_valid_ = false;
        return std::move(result_);
    }

#if !defined(JSONCONS_NO_DEPRECATED)
    Json& root()
    {
        return result_;
    }
#endif

private:

    void push_initial()
    {
        top_ = 1;
        JSONCONS_ASSERT(top_ < stack_.size());
        stack_[0].structure_index_ = 0;
    }

    void pop_initial()
    {
        JSONCONS_ASSERT(top_ == 2);
        result_.swap(stack_[1].value_);
        --top_;
    }

    void push_object()
    {
        stack_[top_].structure_index_ = top_;
        stack_[top_].value_ = object(oa_);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_object()
    {
        JSONCONS_ASSERT(top_ > 0);
    }

    void push_array()
    {
        stack_[top_].structure_index_ = top_;
        stack_[top_].value_ = array(aa_);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_array()
    {
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

    void do_begin_object(const parsing_context&) override
    {
        push_object();
    }

    void do_end_object(const parsing_context&) override
    {
        end_structure();
        pop_object();
    }

    void do_begin_array(const parsing_context&) override
    {
        push_array();
    }

    void do_end_array(const parsing_context&) override
    {
        end_structure();
        pop_array();
    }

    void end_structure() 
    {
        size_t structure_index = stack_[top_-1].structure_index_;
        size_t count = top_ - (structure_index + 1);
        auto it = stack_.begin() + (structure_index + 1);
        auto end = it + count;
        if (stack_[structure_index].value_.is_object())
        {
            stack_[structure_index].value_.object_value().insert(
                std::make_move_iterator(it),
                std::make_move_iterator(end),
                [](stack_item&& val){return key_value_pair_type(std::move(val.name_),std::move(val.value_));});
        }
        else
        {
            auto& j = stack_[structure_index].value_;
            j.reserve(count);
            while (it != end)
            {
                j.push_back(std::move(it->value_));
                ++it;
            }
        }
        top_ -= count;
        stack_[top_-1].structure_index_ = stack_[top_-2].structure_index_;
    }

    void do_name(string_view_type name, const parsing_context&) override
    {
        stack_[top_].name_ = key_storage_type(name.begin(),name.end(),sa_);
    }

    void do_string_value(string_view_type val, const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = Json(val.data(),val.length(),sa_);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_integer_value(int64_t value, const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_uinteger_value(uint64_t value, const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_double_value(double value, uint8_t precision, const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = Json(value,precision);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_bool_value(bool value, const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_null_value(const parsing_context&) override
    {
        stack_[top_].structure_index_ = stack_[top_-1].structure_index_;
        stack_[top_].value_ = Json::null();
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }
};

}

#endif
