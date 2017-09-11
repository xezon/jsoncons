// Copyright 2013-2016 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONDIRECT_CPPDECODER_HPP
#define JSONCONS_JSONDIRECT_CPPDECODER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <istream>
#include <cstdlib>
#include <memory>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_input_handler.hpp>

namespace jsoncons { namespace jsondirect {

// cpp_array_decoder

template <class Json>
class cpp_array_decoder : public basic_json_input_handler<char>
{
public:
    typedef typename char char_type;
    using typename basic_json_input_handler<char>::string_view_type;

    static const int default_stack_size = 1000;

    typedef Json json_type;
    typedef std::string string_type;
    typedef std::string key_storage_type;

    Json& result_;

public:
    cpp_array_decoder(Json& result)
        : result_(result)
    {
    }

private:
    void do_begin_json() override
    {
    }

    void do_end_json() override
    {
    }

    void do_begin_object(const basic_parsing_context<char_type>&) override
    {
    }

    void do_end_object(const basic_parsing_context<char_type>&) override
    {
    }

    void do_begin_array(const basic_parsing_context<char_type>&) override
    {
    }

    void do_end_array(const basic_parsing_context<char_type>&) override
    {
    }

    void do_name(string_view_type name, const basic_parsing_context<char_type>&) override
    {
        // Error
    }

    void do_string_value(string_view_type val, const basic_parsing_context<char_type>&) override
    {
        result_.push_back(val);
    }

    void do_integer_value(int64_t value, const basic_parsing_context<char_type>&) override
    {
        //result_.push_back(value);
    }

    void do_uinteger_value(uint64_t value, const basic_parsing_context<char_type>&) override
    {
        //result_.push_back(value);
    }

    void do_double_value(double value, uint8_t precision, const basic_parsing_context<char_type>&) override
    {
        //result_.push_back(value);
    }

    void do_bool_value(bool value, const basic_parsing_context<char_type>&) override
    {
        //result_.push_back(value);
    }

    void do_null_value(const basic_parsing_context<char_type>&) override
    {
    }
};

// cpp_decoder

template <class Json>
class cpp_decoder : public basic_json_input_handler<char>
{
public:
    typedef basic_json_input_handler<char> input_handler;
    typedef typename char char_type;
    using typename basic_json_input_handler<char>::string_view_type;

    static const int default_stack_size = 1000;

    typedef Json json_type;
    //typedef typename Json::key_value_pair_type key_value_pair_type;
    typedef std::string string_type;
    typedef std::string key_storage_type;

    Json result_;

    std::vector<std::shared_ptr<input_handler>> stack_;
    bool is_valid_;

public:
    cpp_decoder()
        : is_valid_(false) 

    {
        stack_.reserve(default_stack_size);
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

private:

    void do_begin_json() override
    {
        is_valid_ = false;
    }

    void do_end_json() override
    {
        is_valid_ = true;
    }

    void do_begin_object(const basic_parsing_context<char_type>&) override
    {
    }

    void do_end_object(const basic_parsing_context<char_type>&) override
    {
    }

    void do_begin_array(const basic_parsing_context<char_type>&) override
    {
        stack_.push_back(std::make_shared<cpp_array_decoder<Json>>(result_));
    }

    void do_end_array(const basic_parsing_context<char_type>&) override
    {
        stack_.pop_back();
    }

    void do_name(string_view_type name, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->name(name, context);
    }

    void do_string_value(string_view_type val, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->string_value(val, context);
    }

    void do_integer_value(int64_t value, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->integer_value(value, context);
    }

    void do_uinteger_value(uint64_t value, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->uinteger_value(value, context);
    }

    void do_double_value(double value, uint8_t precision, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->double_value(value, precision, context);
    }

    void do_bool_value(bool value, const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->bool_value(value, context);
    }

    void do_null_value(const basic_parsing_context<char_type>& context) override
    {
        stack_.back()->null_value(context);
    }
};


// cpp_object_decoder

template <class Json>
class cpp_object_decoder : public basic_json_input_handler<char>
{
public:
    typedef typename char char_type;
    using typename basic_json_input_handler<char>::string_view_type;

    static const int default_stack_size = 1000;

    typedef Json json_type;
    typedef typename Json::key_value_pair_type key_value_pair_type;
    typedef typename Json::string_type string_type;
    typedef typename std::string key_storage_type;

    Json result_;
    size_t top_;

    struct stack_item
    {
        key_storage_type name_;
        Json value_;
    };
    std::vector<stack_item> stack_;
    std::vector<size_t> stack_offsets_;
    bool is_valid_;

public:
    cpp_object_decoder()
        : top_(0),
          stack_(default_stack_size),
          stack_offsets_(),
          is_valid_(false) 

    {
        stack_offsets_.reserve(100);
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
        result_.swap(stack_[0].value_);
        --top_;
    }

    void push_object()
    {
        stack_offsets_.push_back(top_);
        stack_[top_].value_ = object(oa_);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_object()
    {
        stack_offsets_.pop_back();
        JSONCONS_ASSERT(top_ > 0);
    }

    void push_array()
    {
        stack_offsets_.push_back(top_);
        stack_[top_].value_ = array();
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void pop_array()
    {
        stack_offsets_.pop_back();
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
        JSONCONS_ASSERT(stack_offsets_.size() > 0);
        if (stack_[stack_offsets_.back()].value_.is_object())
        {
            size_t count = top_ - (stack_offsets_.back() + 1);
            auto s = stack_.begin() + (stack_offsets_.back()+1);
            auto send = s + count;
            stack_[stack_offsets_.back()].value_.object_value().insert(
                std::make_move_iterator(s),
                std::make_move_iterator(send),
                [](stack_item&& val){return key_value_pair_type(std::move(val.name_),std::move(val.value_));});
            top_ -= count;
        }
        else
        {
            auto& j = stack_[stack_offsets_.back()].value_;

            auto it = stack_.begin() + (stack_offsets_.back()+1);
            auto end = stack_.begin() + top_;
            size_t count = end - it;
            j.reserve(count);

            while (it != end)
            {
                j.push_back(std::move(it->value_));
                ++it;
            }
            top_ -= count;
        }
    }

    void do_name(string_view_type name, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].name_ = name;
    }

    void do_string_value(string_view_type val, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = Json(val.data(),val.length());
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_integer_value(int64_t value, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_uinteger_value(uint64_t value, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_double_value(double value, uint8_t precision, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = Json(value,precision);
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_bool_value(bool value, const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = value;
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }

    void do_null_value(const basic_parsing_context<char_type>&) override
    {
        stack_[top_].value_ = Json::null();
        if (++top_ >= stack_.size())
        {
            stack_.resize(top_*2);
        }
    }
};

}}

#endif
