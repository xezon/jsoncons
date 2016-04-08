// Copyright 2015 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_PARSER_HPP
#define JSONCONS_JCR_JCR_PARSER_HPP

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <istream>
#include <cstdlib>
#include <stdexcept>
#include <system_error>
#include "jsoncons/json.hpp"
#include "jsoncons/parse_error_handler.hpp"
#include "jcr_input_handler.hpp"
#include "jcr_error_category.hpp"
#include "jcr_rules.hpp"

namespace jsoncons { namespace jcr {

template <typename char_type>
struct jcr_char_traits
{
};

template <>
struct jcr_char_traits<char>
{
    static std::pair<const char*,size_t> integer_literal() 
    {
        static const char* value = "integer";
        return std::pair<const char*,size_t>(value,7);
    }
    static std::pair<const char*,size_t> string_literal() 
    {
        static const char* value = "string";
        return std::pair<const char*,size_t>(value,6);
    }
};

template <>
struct jcr_char_traits<wchar_t>
{

    static std::pair<const wchar_t*,size_t> integer_literal() 
    {
        static const wchar_t* value = L"integer";
        return std::pair<const wchar_t*,size_t>(value,7);
    }
    static std::pair<const wchar_t*,size_t> string_literal() 
    {
        static const wchar_t* value = L"string";
        return std::pair<const wchar_t*,size_t>(value,6);
    }
};

enum class states 
{
    root,
    start, 
    comment,  
    expect_comma_or_end,  
    object,
    min_repetitions,
    max_repetitions,
    expect_max_repetitions,
    expect_member_min_or_repeat_or_rule_or_name, 
    expect_member_repeat_or_rule_or_name, 
    expect_member_rule_or_name, 
    expect_member_name_or_colon, 
    expect_member_max_or_rule_or_name, 
    expect_repeat,
    expect_colon,
    expect_value,
    array, 
    expect_repeat_or_rule_or_value, 
    regex,
    string,
    string_pattern,
    escape, 
    u1, 
    u2, 
    u3, 
    u4, 
    expect_surrogate_pair1, 
    expect_surrogate_pair2, 
    u6, 
    u7, 
    u8, 
    u9, 
    minus, 
    zero,  
    integer,
    dot,
    dot_dot,
    fraction,
    exp1,
    exp2,
    exp3,
    n,
    t,  
    f,  
    any_integer,
    any_string,
    rule_name,
    group,
    expect_rule,
    expect_optional_rule,
    optional_rule,
    max_repeat,
    expect_max_or_repeating_rule,
    expect_repeating_rule,
    repeat_array_item_rule,
    cr,
    lf,
    expect_named_rule,
    member_name,
    value,
    target_rule_name,
    named_value,
    range_value,
    named_rule,
    done
};

template<typename JsonT>
class basic_jcr_parser : private basic_parsing_context<typename JsonT::char_type>
{
    typedef rule<JsonT> rule_type;
    typedef typename std::shared_ptr<rule<JsonT>> rule_ptr;
    typedef typename JsonT::char_type char_type;
    typedef typename JsonT::string_type string_type;

    static const int default_initial_stack_capacity = 100;

    std::map<string_type,rule_ptr> rule_map_;
    std::vector<states> stack_;
    basic_jcr_input_handler<rule_type> *handler_;
    basic_parse_error_handler<char_type> *err_handler_;
    size_t column_;
    size_t line_;
    uint32_t cp_;
    uint32_t cp2_;
    std::basic_string<char_type> string_buffer_;
    std::basic_string<char> number_buffer_;
    bool is_negative_;
    size_t index_;
    int initial_stack_capacity_;
    int max_depth_;
    int nesting_depth_;
    float_reader float_reader_;
    const char_type* begin_input_;
    const char_type* end_input_;
    const char_type* p_;
    uint8_t precision_;
    std::pair<const char_type*,size_t> literal_;
    size_t literal_index_;

    string_type rule_name_;

    std::shared_ptr<rule<JsonT>> from_rule_;
    size_t min_repetitions_;
    size_t max_repetitions_;

    std::vector<std::shared_ptr<member_rule<JsonT>>> member_rule_stack_;
    std::vector<std::pair<bool,std::shared_ptr<group_rule<JsonT>>>> group_rule_stack_;
    std::vector<std::pair<bool,std::shared_ptr<object_rule<JsonT>>>> object_rule_stack_;
    std::vector<std::pair<bool,std::shared_ptr<array_rule<JsonT>>>> array_rule_stack_;
    bool sequence_;

    void do_space()
    {
        while ((p_ + 1) < end_input_ && (*(p_ + 1) == ' ' || *(p_ + 1) == '\t')) 
        {                                      
            ++p_;                          
            ++column_;                     
        }                                      
    }

    void do_begin_object()
    {
        if (++nesting_depth_ >= max_depth_)
        {
            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
        }
        stack_.back() = states::object;
        min_repetitions_ = 1;
        max_repetitions_ = 1;
        stack_.push_back(states::expect_member_min_or_repeat_or_rule_or_name);
        object_rule_stack_.push_back(std::make_pair(sequence_,std::make_shared<object_rule<JsonT>>()));
        sequence_ = true;
    }

    void do_end_object()
    {
        --nesting_depth_;
        JSONCONS_ASSERT(!stack_.empty())
        stack_.pop_back();
        if (stack_.back() == states::object)
        {
            bool sequence = object_rule_stack_.back().first;
            auto rule_ptr = object_rule_stack_.back().second;
            object_rule_stack_.pop_back();
            end_rule(sequence,rule_ptr);
        }
        else if (stack_.back() == states::array)
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_comma_or_right_bracket, jcr_error_category()), *this);
        }
        else
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_brace, jcr_error_category()), *this);
        }
    }

    void do_begin_array()
    {
        if (++nesting_depth_ >= max_depth_)
        {
            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
        }
        stack_.back() = states::array;
        stack_.push_back(states::expect_repeat_or_rule_or_value);
        array_rule_stack_.push_back(std::make_pair(sequence_,std::make_shared<array_rule<JsonT>>()));
        sequence_ = true;
    }

    void do_end_array()
    {
        --nesting_depth_;
        JSONCONS_ASSERT(!stack_.empty())
        stack_.pop_back();
        if (stack_.back() == states::array)
        {
            bool sequence = array_rule_stack_.back().first;
            auto rule_ptr = array_rule_stack_.back().second;
            array_rule_stack_.pop_back();
            end_rule(sequence,rule_ptr);
        }
        else if (stack_.back() == states::object)
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_comma_or_right_brace, jcr_error_category()), *this);
        }
        else
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_bracket, jcr_error_category()), *this);
        }
    }

    void do_begin_group()
    {
        stack_.back() = states::group;
        stack_.push_back(states::expect_member_name_or_colon);
        group_rule_stack_.push_back(std::make_pair(sequence_,std::make_shared<group_rule<JsonT>>()));
        sequence_ = true;
    }

    void do_end_group()
    {
        JSONCONS_ASSERT(!stack_.empty())
        stack_.pop_back();
        if (stack_.back() == states::group)
        {
            bool sequence = group_rule_stack_.back().first;
            auto rule_ptr = group_rule_stack_.back().second;
            group_rule_stack_.pop_back();
            end_rule(sequence,rule_ptr);
        }
        else
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_bracket, jcr_error_category()), *this);
        }
    }

public:
    basic_jcr_parser(basic_jcr_input_handler<rule_type>& handler)
       : handler_(std::addressof(handler)),
         err_handler_(std::addressof(basic_default_parse_error_handler<char_type>::instance())),
         column_(0),
         line_(0),
         cp_(0),
         is_negative_(false),
         index_(0),
         initial_stack_capacity_(default_initial_stack_capacity)
    {
        init();
    }

    basic_jcr_parser(basic_jcr_input_handler<rule_type>& handler,
                      basic_parse_error_handler<char_type>& err_handler)
       : handler_(std::addressof(handler)),
         err_handler_(std::addressof(err_handler)),
         column_(0),
         line_(0),
         cp_(0),
         is_negative_(false),
         index_(0),
         initial_stack_capacity_(default_initial_stack_capacity)

    {
        init();
    }

    void pop_state(states state)
    {
        JSONCONS_ASSERT(!stack_.empty());
        stack_.pop_back();
        if (stack_.back() != state)
        {
            err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
        }
    }


    void init()
    {
        max_depth_ = std::numeric_limits<int>::max JSONCONS_NO_MACRO_EXP();
        rule_map_["boolean"] = std::make_shared<any_boolean_rule<JsonT>>(); 
        rule_map_["float"] = std::make_shared<any_float_rule<JsonT>>(); 
        rule_map_["integer"] = std::make_shared<any_integer_rule<JsonT>>(); 
        rule_map_["string"] = std::make_shared<any_string_rule<JsonT>>(); 
        rule_map_["true"] = std::make_shared<value_rule<JsonT,bool>>(true); 
        rule_map_["false"] = std::make_shared<value_rule<JsonT,bool>>(false); 
        rule_map_["null"] = std::make_shared<null_rule<JsonT>>(); 
        rule_map_["uri"] = std::make_shared<uri_rule<JsonT>>(); 
    }

    const basic_parsing_context<char_type>& parsing_context() const
    {
        return *this;
    }

    ~basic_jcr_parser()
    {
    }

    size_t max_nesting_depth() const
    {
        return static_cast<size_t>(max_depth_);
    }

    void max_nesting_depth(size_t max_nesting_depth)
    {
        max_depth_ = static_cast<int>(std::min(max_nesting_depth,static_cast<size_t>(std::numeric_limits<int>::max JSONCONS_NO_MACRO_EXP())));
    }

    bool done() const
    {
        return stack_.back() == states::start;
    }

    void begin_parse()
    {
        stack_.clear();
        stack_.reserve(initial_stack_capacity_);
        stack_.push_back(states::root);
        stack_.push_back(states::start);
        line_ = 1;
        column_ = 1;
        nesting_depth_ = 0;
        sequence_ = true;
        min_repetitions_ = 1;
        max_repetitions_ = 1;
    }

    void check_done(const char_type* input, size_t start, size_t length)
    {
        index_ = start;
        for (; index_ < length; ++index_)
        {
            char_type curr_char_ = input[index_];
            switch (curr_char_)
            {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                break;
            default:
                err_handler_->error(std::error_code(jcr_parser_errc::extra_character, jcr_error_category()), *this);
                break;
            }
        }
    }

    void parse_string()
    {
        const char_type* sb = p_;
        bool done = false;
        while (!done && p_ < end_input_)
        {
            switch (*p_)
            {
            case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:case 0x08:case 0x0b:
            case 0x0c:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:case 0x16:
            case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:
                string_buffer_.append(sb,p_-sb);
                column_ += (p_ - sb + 1);
                err_handler_->error(std::error_code(jcr_parser_errc::illegal_control_character, jcr_error_category()), *this);
                // recovery - skip
                done = true;
                ++p_;
                break;
            case '\r':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    stack_.push_back(states::cr);
                    done = true;
                    ++p_;
                }
                break;
            case '\n':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    stack_.push_back(states::lf);
                    done = true;
                    ++p_;
                }
                break;
            case '\t':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    done = true;
                    ++p_;
                }
                break;
            case '\\': 
                string_buffer_.append(sb,p_-sb);
                column_ += (p_ - sb + 1);
                stack_.back() = states::escape;
                done = true;
                ++p_;
                break;
            case '\"':
                if (string_buffer_.length() == 0)
                {
                    end_string_value(sb,p_-sb);
                }
                else
                {
                    string_buffer_.append(sb,p_-sb);
                    end_string_value(string_buffer_.data(),string_buffer_.length());
                    string_buffer_.clear();
                }
                column_ += (p_ - sb + 1);
                done = true;
                ++p_;
                break;
            default:
                ++p_;
                break;
            }
        }
        if (!done)
        {
            string_buffer_.append(sb,p_-sb);
            column_ += (p_ - sb + 1);
        }
    }

    void parse_rule()
    {
        bool done = false;
        while (!done && p_ < end_input_)
        {
            if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
            {
                string_buffer_.push_back(*p_);
                ++p_;
            }
            else 
            {
                if (parent() == states::named_rule)
                {
                    rule_name_ = string_buffer_;
                    stack_.back() = states::expect_member_name_or_colon;
                }
                else
                {
                    std::shared_ptr<rule<JsonT>> rule_ptr;
                    auto it = rule_map_.find(string_buffer_);
                    if (it != rule_map_.end())
                    {
                        rule_ptr = it->second;
                    }
                    else
                    {
                        rule_ptr = std::make_shared<jcr_rule_name<JsonT>>(string_buffer_);
                    }
                    end_rule(sequence_,rule_ptr);
                }
                string_buffer_.clear();
                done = true;
            }
        }
    }

    void parse_member_rule()
    {
        bool done = false;
        while (!done && p_ < end_input_)
        {
            if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
            {
                string_buffer_.push_back(*p_);
                ++p_;
            }
            else 
            {
                if (parent() == states::named_rule)
                {
                    rule_name_ = string_buffer_;
                    stack_.back() = states::expect_member_name_or_colon;
                }
                else
                {
                    std::shared_ptr<rule<JsonT>> rule_ptr;
                    auto it = rule_map_.find(string_buffer_);
                    if (it != rule_map_.end())
                    {
                        rule_ptr = it->second;
                    }
                    else
                    {
                        rule_ptr = std::make_shared<jcr_rule_name<JsonT>>(string_buffer_,min_repetitions_,max_repetitions_);
                    }
                    end_rule(sequence_,rule_ptr);
                }
                string_buffer_.clear();
                done = true;
            }
        }
    }

    void parse_optional_rule()
    {
        bool done = false;
        while (!done && p_ < end_input_)
        {
            if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
            {
                string_buffer_.push_back(*p_);
                ++p_;
            }
            else 
            {
                if (parent() == states::named_rule)
                {
                    rule_name_ = string_buffer_;
                    stack_.back() = states::expect_member_name_or_colon;
                }
                else
                {
                    std::shared_ptr<rule<JsonT>> rule_ptr;
                    auto it = rule_map_.find(string_buffer_);
                    if (it != rule_map_.end())
                    {
                        rule_ptr = it->second;
                    }
                    else
                    {
                        auto r = std::make_shared<jcr_rule_name<JsonT>>(string_buffer_);
                        rule_ptr = std::make_shared<optional_rule<JsonT>>(r);
                    }
                    object_rule_stack_.back().second->add_rule(sequence_,rule_ptr);
                    stack_.back() = states::expect_comma_or_end;
                }
                string_buffer_.clear();
                done = true;
            }
        }
    }

    void parse_repeat_array_item()
    {
        bool done = false;
        while (!done && p_ < end_input_)
        {
            if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
            {
                string_buffer_.push_back(*p_);
                ++p_;
            }
            else 
            {
                if (parent() == states::named_rule)
                {
                    rule_name_ = string_buffer_;
                    stack_.back() = states::expect_member_name_or_colon;
                }
                else
                {
                    std::shared_ptr<rule<JsonT>> rule_ptr;
                    auto it = rule_map_.find(string_buffer_);
                    if (it != rule_map_.end())
                    {
                        rule_ptr = it->second;
                    }
                    else
                    {
                        rule_ptr = std::make_shared<jcr_rule_name<JsonT>>(string_buffer_);
                    }
                    array_rule_stack_.back().second->base_rule(rule_ptr);
                    stack_.back() = states::expect_comma_or_end;
                }
                string_buffer_.clear();
                done = true;
            }
        }
    }

    void parse_string_pattern()
    {
        const char_type* sb = p_;
        bool done = false;
        while (!done && p_ < end_input_)
        {
            switch (*p_)
            {
            case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:case 0x08:case 0x0b:
            case 0x0c:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:case 0x16:
            case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:
                string_buffer_.append(sb,p_-sb);
                column_ += (p_ - sb + 1);
                err_handler_->error(std::error_code(jcr_parser_errc::illegal_control_character, jcr_error_category()), *this);
                // recovery - skip
                done = true;
                ++p_;
                break;
            case '\r':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    stack_.push_back(states::cr);
                    done = true;
                    ++p_;
                }
                break;
            case '\n':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    stack_.push_back(states::lf);
                    done = true;
                    ++p_;
                }
                break;
            case '\t':
                {
                    column_ += (p_ - sb + 1);
                    err_handler_->error(std::error_code(jcr_parser_errc::illegal_character_in_string, jcr_error_category()), *this);
                    // recovery - keep
                    string_buffer_.append(sb, p_ - sb + 1);
                    done = true;
                    ++p_;
                }
                break;
            case '\\': 
                string_buffer_.append(sb,p_-sb);
                column_ += (p_ - sb + 1);
                stack_.back() = states::escape;
                done = true;
                ++p_;
                break;
            case '/': // regex
                if (string_buffer_.length() == 0)
                {
                    end_string_pattern(sb,p_-sb);
                }
                else
                {
                    string_buffer_.append(sb,p_-sb);
                    end_string_pattern(string_buffer_.data(),string_buffer_.length());
                    string_buffer_.clear();
                }
                column_ += (p_ - sb + 1);
                done = true;
                ++p_;
                break;
            default:
                ++p_;
                break;
            }
        }
        if (!done)
        {
            string_buffer_.append(sb,p_-sb);
            column_ += (p_ - sb + 1);
        }
    }

    states& parent() 
    {
        JSONCONS_ASSERT(stack_.size() >= 2);
        return stack_[stack_.size()-2];
    }

    states parent() const
    {
        JSONCONS_ASSERT(stack_.size() >= 2);
        return stack_[stack_.size()-2];
    }

    void parse(const char_type* const input, size_t start, size_t length)
    {
        begin_input_ = input + start;
        end_input_ = input + length;
        p_ = begin_input_;

        //handler_->begin_json();
        index_ = start;
        while (p_ < end_input_)
        {
            switch (*p_)
            {
            case 0x00:case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07:case 0x08:case 0x0b:
            case 0x0c:case 0x0e:case 0x0f:case 0x10:case 0x11:case 0x12:case 0x13:case 0x14:case 0x15:case 0x16:
            case 0x17:case 0x18:case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1e:case 0x1f:
                err_handler_->error(std::error_code(jcr_parser_errc::illegal_control_character, jcr_error_category()), *this);
                break;
            default:
                break;
            }

            switch (stack_.back())
            {
            case states::cr:
                ++line_;
                column_ = 1;
                switch (*p_)
                {
                case '\n':
                    JSONCONS_ASSERT(!stack_.empty())
                    stack_.pop_back();
                    ++p_;
                    break;
                default:
                    JSONCONS_ASSERT(!stack_.empty())
                    stack_.pop_back();
                    break;
                }
                break;
            case states::lf:
                ++line_;
                column_ = 1;
                JSONCONS_ASSERT(!stack_.empty())
                stack_.pop_back();
                break;
            case states::start: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case '{':
                        do_begin_object();
                        break;
                    case '[':
                        do_begin_array();
                        break;
                    case '/': // regex
                        stack_.back() = states::string_pattern;
                        break;
                    case '\"':
                        stack_.back() = states::string;
                        break;
                    case '-':
                        is_negative_ = true;
                        stack_.back() = states::minus;
                        break;
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::integer;
                        break;
                    case '}':
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_brace, jcr_error_category()), *this);
                        break;
                    case ']':
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_bracket, jcr_error_category()), *this);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::named_rule;
                            stack_.push_back(states::rule_name);
                        }
                        else
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;

            case states::expect_comma_or_end: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case '}':
                        do_end_object();
                        break;
                    case ']':
                        do_end_array();
                        break;
                    case ',':
                        sequence_ = true;
                        begin_member_or_element();
                        break;
                    case '|':
                        sequence_ = false;
                        begin_member_or_element();
                        break;
                    case ')':
                        do_end_group();
                        break;
                    default:
                        if (parent() == states::array)
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_comma_or_right_bracket, jcr_error_category()), *this);
                        }
                        else if (parent() == states::object)
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_comma_or_right_brace, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_member_min_or_repeat_or_rule_or_name: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        ++p_;
                        ++column_;
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        ++p_;
                        ++column_;
                        break;   
                    case ' ':case '\t':
                        do_space();
                        ++p_;
                        ++column_;
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        ++p_;
                        ++column_;
                        break;
                    case '?':
                        stack_.back() = states::expect_optional_rule;
                        ++p_;
                        ++column_;
                        break;
                    case '0':
                        min_repetitions_ = 0;
                        stack_.back() = states::expect_member_repeat_or_rule_or_name;                       
                        ++p_;
                        ++column_;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        stack_.back() = states::min_repetitions;
                        break;
                    case '*':
                        min_repetitions_ = 1;
                        max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                        stack_.back() = states::expect_max_repetitions;                       
                        ++p_;
                        ++column_;
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::rule_name;
                            ++p_;
                            ++column_;
                        }
                        else
                        {
                            stack_.back() = states::expect_member_rule_or_name;
                        }
                        break;
                    }
                }
                break;
            case states::min_repetitions: 
                {
                    switch (*p_)
                    {
                    case '*':
                        {
                            min_repetitions_ = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                            max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                            number_buffer_.clear();
                            stack_.back() = states::expect_max_repetitions;
                            ++p_;
                            ++column_;
                        }
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        ++p_;
                        ++column_;
                        break;
                    default:
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_star, jcr_error_category()), *this);
                        break;
                    }
                }
                break;
            case states::max_repetitions: 
                {
                    switch (*p_)
                    {
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        ++p_;
                        ++column_;
                        break;
                    default:
                        max_repetitions_ = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                        number_buffer_.clear();
                        stack_.back() = states::expect_member_rule_or_name;
                        break;
                    }
                }
                break;
            case states::expect_max_repetitions: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        ++p_;
                        ++column_;
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        ++p_;
                        ++column_;
                        break;   
                    case ' ':case '\t':
                        do_space();
                        ++p_;
                        ++column_;
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        ++p_;
                        ++column_;
                        break;
                    case '0': 
                        max_repetitions_ = 0;
                        ++p_;
                        ++column_;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        stack_.back() = states::max_repetitions;
                        break;
                    default:
                        stack_.back() = states::expect_member_rule_or_name;
                        break;
                    }
                }
                break;
            case states::expect_member_repeat_or_rule_or_name: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        ++p_;
                        ++column_;
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        ++p_;
                        ++column_;
                        break;   
                    case ' ':case '\t':
                        do_space();
                        ++p_;
                        ++column_;
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        ++p_;
                        ++column_;
                        break;
                    case '?':
                        stack_.back() = states::expect_optional_rule;
                        ++p_;
                        ++column_;
                        break;
                    case '*':
                        min_repetitions_ = 1;
                        max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                        stack_.back() = states::expect_max_or_repeating_rule;                       
                        ++p_;
                        ++column_;
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::rule_name;
                            ++p_;
                            ++column_;
                        }
                        else
                        {
                            stack_.back() = states::expect_member_rule_or_name;
                        }
                        break;
                    }
                }
                break;
            case states::expect_repeat: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        ++p_;
                        ++column_;
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        ++p_;
                        ++column_;
                        break;   
                    case ' ':case '\t':
                        do_space();
                        ++p_;
                        ++column_;
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        ++p_;
                        ++column_;
                        break;
                    case '*':
                        max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                        stack_.back() = states::expect_max_repetitions;                       
                        ++p_;
                        ++column_;
                        break;
                    default:
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_star, jcr_error_category()), *this);
                        break;
                    }
                }
                break;
            case states::expect_member_rule_or_name: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case '/':
                        stack_.back() = states::member_name;
                        stack_.push_back(states::string_pattern);
                        break;
                    case '\"':
                        stack_.back() = states::member_name;
                        stack_.push_back(states::string);
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    case '?':
                        stack_.back() = states::expect_optional_rule;
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::rule_name;
                        }
                        else
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_repeat_or_rule_or_value: 
                {
                    switch (*p_)
                    {
                    case '\r':                                   
                        stack_.push_back(states::cr);            
                        ++p_;
                        ++column_;
                        break;                                   
                    case '\n':                                   
                        stack_.push_back(states::lf);            
                        ++p_;
                        ++column_;
                        break;                                   
                    case ' ':case '\t':                          
                        do_space();                              
                        ++p_;
                        ++column_;
                        break;                                   
                    case ';':                                    
                        stack_.push_back(states::comment);         
                        ++p_;
                        ++column_;
                        break;
                    case '*':
                        min_repetitions_ = 1;
                        max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                        stack_.back() = states::expect_max_or_repeating_rule;                       
                        ++p_;
                        ++column_;
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::rule_name;
                            ++p_;
                            ++column_;
                        }
                        else
                        {
                            stack_.back() = states::expect_value;
                        }
                        break;
                    }
                }
                break;
            case states::expect_optional_rule: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::optional_rule;
                        }
                        else
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_repeating_rule: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::repeat_array_item_rule;
                        }
                        else
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_max_or_repeating_rule: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        ++p_;
                        ++column_;
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        ++p_;
                        ++column_;
                        break;   
                    case ' ':case '\t':
                        do_space();
                        ++p_;
                        ++column_;
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        ++p_;
                        ++column_;
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        stack_.back() = states::max_repeat;
                        break;
                    default:
                        switch (parent())
                        {
                        case states::array:
                            {
                                auto repeating_rule_ptr = std::make_shared<repeat_array_item_rule<JsonT>>(min_repetitions_);
                                array_rule_stack_.back().second->add_rule(sequence_, repeating_rule_ptr);
                                stack_.back() = states::expect_repeating_rule;
                            }
                            break;
                        }
                        break;
                    }
                }
                break;
            case states::expect_member_name_or_colon: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case '/':
                        stack_.back() = states::member_name;
                        stack_.push_back(states::string_pattern);
                        break;
                    case '\"':
                        stack_.back() = states::member_name;
                        stack_.push_back(states::string);
                        break;
                    case ':':
                        stack_.back() = states::value;
                        stack_.push_back(states::expect_value);
                        break;
                    case '(':
                        do_begin_group();
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_colon: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case ':':
                        stack_.back() = states::expect_value;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_colon, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_value: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        do_space();
                        break;
                    case ';': 
                        stack_.push_back(states::comment);
                        break;
                    case '{':
                        do_begin_object();
                        break;
                    case '[':
                        do_begin_array();
                        break;
                    case '/':
                        stack_.back() = states::string_pattern;
                        break;
                    case '\"':
                        stack_.back() = states::string;
                        break;
                    case '-':
                        is_negative_ = true;
                        stack_.back() = states::minus;
                        break;
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::integer;
                        break;
                    case ']':
                        if (parent() == states::array)
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::extra_comma, jcr_error_category()), *this);
                        }
                        else
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        }
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            string_buffer_.push_back(*p_);
                            stack_.back() = states::target_rule_name;
                        }
                        else
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::target_rule_name:
                if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
                {
                    string_buffer_.push_back(*p_);
                    ++p_;
                }
                else 
                {
                    std::shared_ptr<rule<JsonT>> rule_ptr;
                    auto it = rule_map_.find(string_buffer_);
                    if (it != rule_map_.end())
                    {
                        rule_ptr = it->second;
                    }
                    else
                    {
                        rule_ptr = std::make_shared<jcr_rule_name<JsonT>>(string_buffer_);
                    }
                    end_rule(sequence_,rule_ptr);
                    string_buffer_.clear();
                }
                break;
            case states::rule_name:
                parse_rule();
                break;
            case states::optional_rule:
                parse_optional_rule();
                break;
            case states::repeat_array_item_rule:
                parse_repeat_array_item();
                break;
            case states::string: 
                parse_string();
                break;
            case states::string_pattern: 
                parse_string_pattern();
                break;
            case states::escape: 
                {
                    escape_next_char(*p_);
                }
                ++p_;
                ++column_;
                break;
            case states::u1: 
                {
                    append_codepoint(*p_);
                    stack_.back() = states::u2;
                }
                ++p_;
                ++column_;
                break;
            case states::u2: 
                {
                    append_codepoint(*p_);
                    stack_.back() = states::u3;
                }
                ++p_;
                ++column_;
                break;
            case states::u3: 
                {
                    append_codepoint(*p_);
                    stack_.back() = states::u4;
                }
                ++p_;
                ++column_;
                break;
            case states::u4: 
                {
                    append_codepoint(*p_);
                    if (cp_ >= min_lead_surrogate && cp_ <= max_lead_surrogate)
                    {
                        stack_.back() = states::expect_surrogate_pair1;
                    }
                    else
                    {
                        json_char_traits<char_type, sizeof(char_type)>::append_codepoint_to_string(cp_, string_buffer_);
                        stack_.back() = states::string;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_surrogate_pair1: 
                {
                    switch (*p_)
                    {
                    case '\\': 
                        cp2_ = 0;
                        stack_.back() = states::expect_surrogate_pair2;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_codepoint_surrogate_pair, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_surrogate_pair2: 
                {
                    switch (*p_)
                    {
                    case 'u':
                        stack_.back() = states::u6;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_codepoint_surrogate_pair, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::u6:
                {
                    append_second_codepoint(*p_);
                    stack_.back() = states::u7;
                }
                ++p_;
                ++column_;
                break;
            case states::u7: 
                {
                    append_second_codepoint(*p_);
                    stack_.back() = states::u8;
                }
                ++p_;
                ++column_;
                break;
            case states::u8: 
                {
                    append_second_codepoint(*p_);
                    stack_.back() = states::u9;
                }
                ++p_;
                ++column_;
                break;
            case states::u9: 
                {
                    append_second_codepoint(*p_);
                    uint32_t cp = 0x10000 + ((cp_ & 0x3FF) << 10) + (cp2_ & 0x3FF);
                    json_char_traits<char_type, sizeof(char_type)>::append_codepoint_to_string(cp, string_buffer_);
                    stack_.back() = states::string;
                }
                ++p_;
                ++column_;
                break;
            case states::minus:  
                {
                    switch (*p_)
                    {
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::integer;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::zero:  
                {
                    switch (*p_)
                    {
                    case '\r':
                        stack_.push_back(states::cr);
                        break;
                    case '\n':
                        stack_.push_back(states::lf);
                        break;
                    case ' ':case '\t':
                        {
                            bool done = false;
                            while (!done && (p_ + 1) < end_input_)
                            {
                                switch (*(p_ + 1))
                                {
                                case ' ':case '\t':
                                    ++p_;
                                    ++column_;
                                    break;
                                default:
                                    done = true;
                                    break;
                                }
                            }
                        }
                        end_integer_value();
                        break; // No change
                    case '}':
                        end_integer_value();
                        do_end_object();
                        break;
                    case ']':
                        end_integer_value();
                        do_end_array();
                        break;
                    case '.':
                        stack_.back() = states::dot;
                        break;
                    case ',':
                        sequence_ = true;
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case '|':
                        sequence_ = false;
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case '0': case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        err_handler_->error(std::error_code(jcr_parser_errc::leading_zero, jcr_error_category()), *this);
                        break;
                    case '*':
                        {
                            min_repetitions_ = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                            max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                            number_buffer_.clear();
                            stack_.back() = states::expect_max_or_repeating_rule;                        
                        }
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::dot_dot:
                switch (*p_)
                {
                case '-':
                    is_negative_ = true;
                    stack_.back() = states::minus;
                    ++p_;
                    ++column_;
                    break;
                case '0': 
                    number_buffer_.push_back(static_cast<char>(*p_));
                    stack_.back() = states::zero;
                    ++p_;
                    ++column_;
                    break;
                case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    number_buffer_.push_back(static_cast<char>(*p_));
                    stack_.back() = states::integer;
                    ++p_;
                    ++column_;
                    break;
                default:
                    stack_.pop_back();
                    end_rule(sequence_,from_rule_);
                    from_rule_ = nullptr;
                    break;
                }
                break;
            case states::dot:
                switch (*p_)
                {
                case '.':
                {
                    if (is_negative_)
                    {
                        try
                        {
                            auto val = string_to_integer(is_negative_, number_buffer_.data(), number_buffer_.length());
                            from_rule_ = std::make_shared<from_rule<JsonT,int64_t>>(val);
                        }
                        catch (const std::exception&)
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        }
                    }
                    else
                    {
                        try
                        {
                            auto val = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                            from_rule_ = std::make_shared<from_rule<JsonT,int64_t>>(val);
                        }
                        catch (const std::exception&)
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        }
                    }

                    stack_.back() = states::range_value;
                    stack_.push_back(states::dot_dot);
                    number_buffer_.clear();
                    is_negative_ = false;
                }
                    break;
                default:
                    precision_ = static_cast<uint8_t>(number_buffer_.length());
                    number_buffer_.push_back(static_cast<char>(*p_));
                    stack_.back() = states::fraction;
                    break;
                }
                ++p_;
                ++column_;
                break;
            case states::integer: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        end_integer_value();
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        end_integer_value();
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        end_integer_value();
                        do_space();
                        break;
                    case ';': 
                        end_integer_value();
                        stack_.push_back(states::comment);
                        break;
                    case '}':
                        end_integer_value();
                        do_end_object();
                        break;
                    case ']':
                        end_integer_value();
                        do_end_array();
                        break;
                    case '*':
                        {
                            min_repetitions_ = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                            max_repetitions_ = std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP();
                            number_buffer_.clear();
                            stack_.back() = states::expect_max_or_repeating_rule;
                        }
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        break;
                    case '.':
                        stack_.back() = states::dot;
                        break;
                    case ',':
                        sequence_ = true;
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case '|':
                        sequence_ = false;
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case 'e':case 'E':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp1;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::max_repeat: 
                {
                    switch (*p_)
                    {
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        ++p_;
                        ++column_;
                        break;
                    default:
                        size_t max_repeat = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                        switch (parent())
                        {
                        case states::array:
                            {
                                auto repeating_rule_ptr = std::make_shared<repeat_array_item_rule<JsonT>>(min_repetitions_,max_repeat);
                                array_rule_stack_.back().second->add_rule(sequence_,repeating_rule_ptr);
                                stack_.back() = states::expect_max_or_repeating_rule;
                            }
                            break;
                        }

                        number_buffer_.clear();
                        stack_.back() = states::expect_repeating_rule;
                        break;
                    }
                }
                break;
            case states::fraction: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        end_fraction_value();
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        end_fraction_value();
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        end_fraction_value();
                        do_space();
                        break;
                    case ';': 
                        end_fraction_value();
                        stack_.push_back(states::comment);
                        break;
                    case '}':
                        end_fraction_value();
                        do_end_object();
                        break;
                    case ']':
                        end_fraction_value();
                        do_end_array();
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        ++precision_;
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::fraction;
                        break;
                    case ',':
                        sequence_ = true;
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case '|':
                        sequence_ = false;
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case 'e':case 'E':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp1;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::exp1: 
                {
                    switch (*p_)
                    {
                    case '+':
                        stack_.back() = states::exp2;
                        break;
                    case '-':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp2;
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp3;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::exp2:  
                {
                    switch (*p_)
                    {
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp3;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::exp3: 
                {
                    switch (*p_)
                    {
                    case '\r': 
                        end_fraction_value();
                        stack_.push_back(states::cr);
                        break; 
                    case '\n': 
                        end_fraction_value();
                        stack_.push_back(states::lf); 
                        break;   
                    case ' ':case '\t':
                        end_fraction_value();
                        do_space();
                        break;
                    case ';': 
                        end_fraction_value();
                        stack_.push_back(states::comment);
                        break;
                    case '}':
                        end_fraction_value();
                        do_end_object();
                        break;
                    case ']':
                        end_fraction_value();
                        do_end_array();
                        break;
                    case ',':
                        sequence_ = true;
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case '|':
                        sequence_ = false;
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        stack_.back() = states::exp3;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::comment: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        stack_.back() = states::cr;
                        break;
                    case '\n':
                        stack_.back() = states::lf;
                        break;
                    case ';':
                        stack_.pop_back();
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            default:
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad parser state");
                break;
            }
        }
        index_ += (p_-begin_input_);
    }

    void end_parse()
    {
        if (parent() == states::root)
        {
            switch (stack_.back())
            {
            case states::zero:  
            case states::integer:
                end_integer_value();
                break;
            case states::fraction:
            case states::exp3:
                end_fraction_value();
                break;
            default:
                break;
            }
        }
        if (stack_.back() != states::start)
        {
            err_handler_->error(std::error_code(jcr_parser_errc::unexpected_eof, jcr_error_category()), *this);
        }

        //handler_->end_json();
    }

    states state() const
    {
        return stack_.back();
    }

    size_t index() const
    {
        return index_;
    }
private:
    void end_fraction_value()
    {
        try
        {
            double d = float_reader_.read(number_buffer_.data(), precision_);
            if (is_negative_)
                d = -d;

            auto rule_ptr = std::make_shared<value_rule<JsonT,double>>(d);
            end_rule(sequence_,rule_ptr);
        }
        catch (...)
        {
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
        }
        number_buffer_.clear();
        is_negative_ = false;
    }

    void end_integer_value()
    {
        std::shared_ptr<rule<JsonT>> rule_ptr;
        if (is_negative_)
        {
            try
            {
                int64_t val = string_to_integer(is_negative_, number_buffer_.data(), number_buffer_.length());

                if (parent() == states::range_value)
                {
                    auto to_r = std::make_shared<to_rule<JsonT,int64_t>>(val);
                    rule_ptr = std::make_shared<composite_rule<JsonT>>(from_rule_, to_r);
                    pop_state(states::range_value);
                    //from_rule_ = nullptr;
                }
                else
                {
                    rule_ptr = std::make_shared<value_rule<JsonT,int64_t>>(val);
                }
            }
            catch (const std::exception&)
            {
                err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
            }
        }
        else
        {
            try
            {
                uint64_t val= string_to_uinteger(number_buffer_.data(), number_buffer_.length());

                if (parent() == states::range_value)
                {
                    auto to_r = std::make_shared<to_rule<JsonT,uint64_t>>(val);
                    rule_ptr = std::make_shared<composite_rule<JsonT>>(from_rule_,to_r);
                    pop_state(states::range_value);
                }
                else
                {
                    rule_ptr = std::make_shared<value_rule<JsonT,int64_t>>(val);
                }
            }
            catch (const std::exception&)
            {
                err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
            }
        }

        end_rule(sequence_,rule_ptr);

        JSONCONS_ASSERT(stack_.size() >= 2);
        number_buffer_.clear();
        is_negative_ = false;
    }

    void end_rule(bool sequence, std::shared_ptr<rule<JsonT>> rule_ptr)
    {
        switch (parent())
        {
        case states::value:
            stack_.pop_back();
            break;
        }
        switch (parent())
        {
        case states::member_name:
            {
                member_rule_stack_.back()->base_rule(rule_ptr);
                rule_ptr = member_rule_stack_.back();
                member_rule_stack_.pop_back();
                stack_.pop_back();
            }
            break;
        }

        switch (parent())
        {
        case states::array:
            {
                array_rule_stack_.back().second->add_rule(sequence,rule_ptr);
                stack_.back() = states::expect_comma_or_end;
            }
            break;
        case states::object:
            {
                object_rule_stack_.back().second->add_rule(sequence,rule_ptr);
                stack_.back() = states::expect_comma_or_end;
            }
            break;
        case states::named_rule:
            {
                handler_->named_rule(rule_name_, rule_ptr, *this);
                stack_.pop_back();
                stack_.back() = states::start;
            }
            break;
        case states::group:
            {
                group_rule_stack_.back().second->add_rule(sequence,rule_ptr);
                stack_.back() = states::expect_comma_or_end;
            }
            break;
        case states::root:
            {
                handler_->rule_definition(rule_ptr, *this);
                stack_.back() = states::start;
            }
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
            break;
        }
    }

    void append_codepoint(int c)
    {
        switch (c)
        {
        case '0': case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
            cp_ = append_to_codepoint(cp_, c);
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
            break;
        }
    }

    void append_second_codepoint(int c)
    {
        switch (c)
        {
        case '0': 
        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
            cp2_ = append_to_codepoint(cp2_, c);
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
            break;
        }
    }

    void escape_next_char(int next_input)
    {
        switch (next_input)
        {
        case '\"':
            string_buffer_.push_back('\"');
            stack_.back() = states::string;
            break;
        case '\\': 
            string_buffer_.push_back('\\');
            stack_.back() = states::string;
            break;
        case '/':
            string_buffer_.push_back('/');
            stack_.back() = states::string;
            break;
        case 'b':
            string_buffer_.push_back('\b');
            stack_.back() = states::string;
            break;
        case 'f':  
            string_buffer_.push_back('\f');
            stack_.back() = states::string;
            break;
        case 'n':
            string_buffer_.push_back('\n');
            stack_.back() = states::string;
            break;
        case 'r':
            string_buffer_.push_back('\r');
            stack_.back() = states::string;
            break;
        case 't':
            string_buffer_.push_back('\t');
            stack_.back() = states::string;
            break;
        case 'u':
            cp_ = 0;
            stack_.back() = states::u1;
            break;
        default:    
            err_handler_->error(std::error_code(jcr_parser_errc::illegal_escaped_character, jcr_error_category()), *this);
            break;
        }
    }

    void end_string_value(const char_type* s, size_t length) 
    {
        std::shared_ptr<rule<JsonT>> rule_ptr;

        switch (parent())
        {
        case states::member_name:
            {
                auto rule_ptr = std::make_shared<qstring_member_rule<JsonT>>(string_type(s, length), min_repetitions_, max_repetitions_);
                member_rule_stack_.push_back(rule_ptr);
                stack_.back() = states::value;
                stack_.push_back(states::expect_colon);
            }
            break;
        case states::value:
            {
                auto r = std::make_shared<string_rule<JsonT>>(s, length);
                end_rule(sequence_,r);
            }
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
            break;
        }

        string_buffer_.clear();
    }

    void end_string_pattern(const char_type* s, size_t length) 
    {
        std::shared_ptr<rule<JsonT>> rule_ptr;

        switch (parent())
        {
        case states::member_name:
            {
                auto rule_ptr = std::make_shared<regex_member_rule<JsonT>>(string_type(s, length), min_repetitions_, max_repetitions_);
                member_rule_stack_.push_back(rule_ptr);
                stack_.back() = states::value;
                stack_.push_back(states::expect_colon);
            }
            break;
        case states::value:
            {
                auto r = std::make_shared<string_rule<JsonT>>(s, length);
                end_rule(sequence_,r);
            }
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
            break;
        }

        string_buffer_.clear();
    }

    void begin_member_or_element() 
    {
        switch (parent())
        {
        case states::object:
            min_repetitions_ = 1;
            max_repetitions_ = 1;
            stack_.back() = states::expect_member_min_or_repeat_or_rule_or_name;
            break;
        case states::array:
            stack_.back() = states::expect_repeat_or_rule_or_value;
            break;
        case states::group:
            stack_.back() = states::expect_member_name_or_colon;
            break;
        case states::root:
            stack_.back() = states::start;
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_jcr_text, jcr_error_category()), *this);
            break;
        }
    }
 
    uint32_t append_to_codepoint(uint32_t cp, int c)
    {
        cp *= 16;
        if (c >= '0'  &&  c <= '9')
        {
            cp += c - '0';
        }
        else if (c >= 'a'  &&  c <= 'f')
        {
            cp += c - 'a' + 10;
        }
        else if (c >= 'A'  &&  c <= 'F')
        {
            cp += c - 'A' + 10;
        }
        else
        {
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_hex_escape_sequence, jcr_error_category()), *this);
        }
        return cp;
    }

    size_t do_line_number() const override
    {
        return line_;
    }

    size_t do_column_number() const override
    {
        return column_;
    }

    char_type do_current_char() const override
    {
        return p_ < end_input_? *p_ : 0;
    }
};

typedef basic_jcr_parser<json> jcr_parser;
typedef basic_jcr_parser<wjson> wjcr_parser;

}}

#endif

