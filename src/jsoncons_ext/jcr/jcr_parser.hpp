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

template <typename CharT>
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

enum class value_types
{
    none_t,
    integer_t,
    uinteger_t,
    double_t
};

enum class modes 
{
    initial,
    scalar,
    array_element,
    object_member_name,
    object_member_value,
    named_rule,
    rule_member_name,
    rule_member_value
};

enum class states 
{
    start, 
    slash,  
    slash_slash, 
    slash_star, 
    slash_star_star,
    expect_comma_or_end,  
    object,
    expect_member_name, 
    expect_colon,
    expect_value,
    array, 
    string,
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
    expect_rule,
    cr,
    lf,
    expect_named_rule,
    expect_rule_value
};

template<typename JsonT>
class basic_jcr_parser : private basic_parsing_context<typename JsonT::char_type>
{
    typedef typename rule<JsonT> rule_type;
    typedef typename JsonT::char_type char_type;
    typedef typename JsonT::string_type string_type;
    static const int default_depth = 100;

    states state_;
    int top_;
    std::vector<modes> stack_;
    basic_jcr_input_handler<rule_type> *handler_;
    basic_parse_error_handler<char_type> *err_handler_;
    size_t column_;
    size_t line_;
    uint32_t cp_;
    uint32_t cp2_;
    std::basic_string<char_type> string_buffer_;
    std::basic_string<char> number_buffer_;
    bool is_negative_;
    states saved_state_;
    states pre_line_break_state_;
    size_t index_;
    int depth_;
    int max_depth_;
    float_reader float_reader_;
    const char_type* begin_input_;
    const char_type* end_input_;
    const char_type* p_;
    uint8_t precision_;
    std::pair<const char_type*,size_t> literal_;
    size_t literal_index_;

public:
    basic_jcr_parser(basic_jcr_input_handler<rule_type>& handler)
       : state_(states::start), 
         top_(-1),
         stack_(default_depth),
         handler_(std::addressof(handler)),
         err_handler_(std::addressof(basic_default_parse_error_handler<char_type>::instance())),
         column_(0),
         line_(0),
         cp_(0),
         is_negative_(false),
         index_(0),
         depth_(default_depth)
    {
        max_depth_ = std::numeric_limits<int>::max JSONCONS_NO_MACRO_EXP();
    }

    basic_jcr_parser(basic_jcr_input_handler<rule_type>& handler,
                      basic_parse_error_handler<char_type>& err_handler)
       : state_(states::start), 
         top_(-1),
         stack_(default_depth),
         handler_(std::addressof(handler)),
         err_handler_(std::addressof(err_handler)),
         column_(0),
         line_(0),
         cp_(0),
         is_negative_(false),
         index_(0),
         depth_(default_depth)

    {
        max_depth_ = std::numeric_limits<int>::max JSONCONS_NO_MACRO_EXP();
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
        if (depth_ > max_depth_)
        {
            depth_ = max_depth_;
            stack_.resize(depth_);
        }
    }

    bool done() const
    {
        return state_ == states::expect_named_rule;
    }

    void begin_parse()
    {
        if (!push(modes::initial))
        {
            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
        }
        state_ = states::start;
        line_ = 1;
        column_ = 1;
    }

    string_type rule_name_;
    string_type rule_member_name_;

    std::shared_ptr<rule<JsonT>> from_rule_;

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
                    pre_line_break_state_ = state_;
                    state_ = states::cr;
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
                    pre_line_break_state_ = state_;
                    state_ = states::lf;
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
                state_ = states::escape;
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
            //++p_;
        }
        if (!done)
        {
            string_buffer_.append(sb,p_-sb);
            column_ += (p_ - sb + 1);
        }
    }

    void parse(const char_type* const input, size_t start, size_t length)
    {
        begin_input_ = input + start;
        end_input_ = input + length;
        p_ = begin_input_;

        index_ = start;
        while ((p_ < end_input_))
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

            switch (state_)
            {
            case states::cr:
                ++line_;
                column_ = 1;
                switch (*p_)
                {
                case '\n':
                    state_ = pre_line_break_state_;
                    ++p_;
                    break;
                default:
                    state_ = pre_line_break_state_;
                    break;
                }
                break;
            case states::lf:
                ++line_;
                column_ = 1;
                state_ = pre_line_break_state_;
                break;
            case states::start: 
                {
                    switch (*p_)
                    {
                        case '\r':
                            pre_line_break_state_ = state_;
                            state_ = states::cr;
                            break;
                        case '\n':
                            pre_line_break_state_ = state_;
                            state_ = states::lf;
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
                        break; 
                    case '{':
                        handler_->begin_json();
                        if (!push(modes::object_member_name))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::object;
                        handler_->begin_object(*this);
                        break;
                    case '[':
                        handler_->begin_json();
                        if (!push(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::array;
                        handler_->begin_array(*this);
                        break;
                    case '\"':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::string;
                        break;
                    case '-':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        is_negative_ = true;
                        state_ = states::minus;
                        break;
                    case '0': 
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
                        break;
                    case 'f':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::f;
                        literal_ = json_literals<char_type>::false_literal();
                        literal_index_ = 1;
                        break;
                    case 'i':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::any_integer;
                        literal_ = jcr_char_traits<char_type>::integer_literal();
                        literal_index_ = 1;
                        break;
                    case 's':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::any_string;
                        literal_ = jcr_char_traits<char_type>::string_literal();
                        literal_index_ = 1;
                        break;
                    case 'n':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::n;
                        literal_ = json_literals<char_type>::null_literal();
                        literal_index_ = 1;
                        break;
                    case 't':
                        handler_->begin_json();
                        flip(modes::initial, modes::scalar);
                        state_ = states::t;
                        literal_ = json_literals<char_type>::true_literal();
                        literal_index_ = 1;
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    case '}':
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_brace, jcr_error_category()), *this);
                        break;
                    case ']':
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_bracket, jcr_error_category()), *this);
                        break;
                    default:
                        err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
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
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break; 
                    case '}':
                        if (peek() == modes::object_member_value)
                        {
                            pop(modes::object_member_value);                        
                            handler_->end_object(*this);
                            if (peek() == modes::initial)
                            {
                                flip(modes::initial, modes::named_rule);
                                state_ = states::expect_named_rule;
                                handler_->end_json();
                            }
                            else
                            {
                                state_ = states::expect_comma_or_end;
                            }
                        }
                        else if (peek() == modes::array_element)
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_comma_or_right_bracket, jcr_error_category()), *this);
                        }
                        else
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_brace, jcr_error_category()), *this);
                        }
                        break;
                    case ']':
                        if (peek() == modes::array_element)
                        {
                            pop(modes::array_element);                        
                            handler_->end_array(*this);
                            if (peek() == modes::initial)
                            {
                                flip(modes::initial, modes::named_rule);
                                state_ = states::expect_named_rule;
                                handler_->end_json();
                            }
                            else
                            {
                                state_ = states::expect_comma_or_end;
                            }
                        }
                        else if (peek() == modes::object_member_value)
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::expected_comma_or_right_brace, jcr_error_category()), *this);
                        }
                        else
                        {
                            err_handler_->fatal_error(std::error_code(jcr_parser_errc::unexpected_right_bracket, jcr_error_category()), *this);
                        }
                        break;
                    case ',':
                        begin_member_or_element();
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    default:
                        if (peek() == modes::array_element)
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_comma_or_right_bracket, jcr_error_category()), *this);
                        }
                        else if (peek() == modes::object_member_value)
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::expected_comma_or_right_brace, jcr_error_category()), *this);
                        }
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::object: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case '}':
                        if (!pop(modes::object_member_name))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_object(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case '\"':
                        state_ = states::string;
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            flip(modes::object_member_name, modes::object_member_value);
                            string_buffer_.push_back(*p_);
                            state_ = states::rule_name;
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
            case states::rule_name:
                if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z') || ('0' <=*p_ && *p_ <= '9') || *p_ == '-' || *p_ == '_')
                {
                    string_buffer_.push_back(*p_);
                    ++p_;
                }
                else if (stack_[top_] == modes::named_rule)
                {
                    flip(modes::named_rule,modes::rule_member_name);
                    rule_name_ = string_buffer_;
                    string_buffer_.clear();
                    state_ = states::expect_member_name;
                }
                else
                {
                    handler_->rule_name(string_buffer_.data(),string_buffer_.length(),*this);
                    string_buffer_.clear();
                    state_ = states::expect_comma_or_end;
                }
                break;
            case states::expect_member_name: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case '\"':
                        state_ = states::string;
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    case '}':
                        err_handler_->error(std::error_code(jcr_parser_errc::extra_comma, jcr_error_category()), *this);
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                        {
                            flip(modes::object_member_name, modes::object_member_value);
                            string_buffer_.push_back(*p_);
                            state_ = states::rule_name;
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
            case states::expect_colon: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case ':':
                        switch (stack_[top_])
                        {
                        case modes::object_member_name:
                            flip(modes::object_member_name, modes::object_member_value);
                            state_ = states::expect_value;
                            break;
                        case modes::rule_member_name:
                            flip(modes::rule_member_name, modes::rule_member_value);
                            state_ = states::expect_rule_value;
                            break;
                        }
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
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
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case '{':
                        if (!push(modes::object_member_name))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::object;
                        handler_->begin_object(*this);
                        break;
                    case '[':
                        if (!push(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::array;
                        handler_->begin_array(*this);
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;

                    case '\"':
                        state_ = states::string;
                        break;
                    case '-':
                        is_negative_ = true;
                        state_ = states::minus;
                        break;
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
                        break;
                    case 'f':
                        state_ = states::f;
                        literal_ = json_literals<char_type>::false_literal();
                        literal_index_ = 1;
                        break;
                    case 'i':
                        state_ = states::any_integer;
                        literal_ = jcr_char_traits<char_type>::integer_literal();
                        literal_index_ = 1;
                        break;
                    case 's':
                        state_ = states::any_string;
                        literal_ = jcr_char_traits<char_type>::string_literal();
                        literal_index_ = 1;
                        break;
                    case 'n':
                        state_ = states::n;
                        literal_ = json_literals<char_type>::null_literal();
                        literal_index_ = 1;
                        break;
                    case 't':
                        state_ = states::t;
                        literal_ = json_literals<char_type>::true_literal();
                        literal_index_ = 1;
                        break;
                    case ']':
                        if (peek() == modes::array_element)
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
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::array: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case '{':
                        if (!push(modes::object_member_name))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::object;
                        handler_->begin_object(*this);
                        break;
                    case '[':
                        if (!push(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::max_depth_exceeded, jcr_error_category()), *this);
                        }
                        state_ = states::array;
                        handler_->begin_array(*this);
                        break;
                    case ']':
                        if (!pop(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_array(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case '\"':
                        state_ = states::string;
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    case '-':
                        is_negative_ = true;
                        state_ = states::minus;
                        break;
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
                        break;
                    case 'f':
                        state_ = states::f;
                        literal_ = json_literals<char_type>::false_literal();
                        literal_index_ = 1;
                        break;
                    case 'i':
                        state_ = states::any_integer;
                        literal_ = jcr_char_traits<char_type>::integer_literal();
                        literal_index_ = 1;
                        break;
                    case 's':
                        state_ = states::any_string;
                        literal_ = jcr_char_traits<char_type>::string_literal();
                        literal_index_ = 1;
                        break;
                    case 'n':
                        state_ = states::n;
                        literal_ = json_literals<char_type>::null_literal();
                        literal_index_ = 1;
                        break;
                    case 't':
                        state_ = states::t;
                        literal_ = json_literals<char_type>::true_literal();
                        literal_index_ = 1;
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::string: 
                parse_string();
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
                    state_ = states::u2;
                }
                ++p_;
                ++column_;
                break;
            case states::u2: 
                {
                    append_codepoint(*p_);
                    state_ = states::u3;
                }
                ++p_;
                ++column_;
                break;
            case states::u3: 
                {
                    append_codepoint(*p_);
                    state_ = states::u4;
                }
                ++p_;
                ++column_;
                break;
            case states::u4: 
                {
                    append_codepoint(*p_);
                    if (cp_ >= min_lead_surrogate && cp_ <= max_lead_surrogate)
                    {
                        state_ = states::expect_surrogate_pair1;
                    }
                    else
                    {
                        json_char_traits<char_type, sizeof(char_type)>::append_codepoint_to_string(cp_, string_buffer_);
                        state_ = states::string;
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
                        state_ = states::expect_surrogate_pair2;
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
                        state_ = states::u6;
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
                    state_ = states::u7;
                }
                ++p_;
                ++column_;
                break;
            case states::u7: 
                {
                    append_second_codepoint(*p_);
                    state_ = states::u8;
                }
                ++p_;
                ++column_;
                break;
            case states::u8: 
                {
                    append_second_codepoint(*p_);
                    state_ = states::u9;
                }
                ++p_;
                ++column_;
                break;
            case states::u9: 
                {
                    append_second_codepoint(*p_);
                    uint32_t cp = 0x10000 + ((cp_ & 0x3FF) << 10) + (cp2_ & 0x3FF);
                    json_char_traits<char_type, sizeof(char_type)>::append_codepoint_to_string(cp, string_buffer_);
                    state_ = states::string;
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
                        state_ = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
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
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        if (!pop(modes::object_member_value))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_object(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case ']':
                        end_integer_value();
                        if (!pop(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_array(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case '.':
                        state_ = states::dot;
                        break;
                    case ',':
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case '0': case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        err_handler_->error(std::error_code(jcr_parser_errc::leading_zero, jcr_error_category()), *this);
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
                    state_ = states::minus;
                    break;
                case '0': 
                    number_buffer_.push_back(static_cast<char>(*p_));
                    state_ = states::zero;
                    break;
                case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                    number_buffer_.push_back(static_cast<char>(*p_));
                    state_ = states::integer;
                    break;
                default:
                    switch (stack_[top_])
                    {
                    case modes::array_element:
                    case modes::object_member_value:
                        handler_->rule_definition(from_rule_, *this);
                        state_ = states::expect_comma_or_end;
                        break;
                    case modes::rule_member_value:
                        handler_->named_rule(rule_name_,
                            std::make_shared<member_rule<JsonT>>(rule_member_name_, from_rule_),
                            *this);
                        flip(modes::rule_member_value, modes::named_rule);
                        state_ = states::expect_named_rule;
                        break;
                    case modes::scalar:
                        handler_->rule_definition(from_rule_, *this);
                        flip(modes::scalar, modes::named_rule);
                        state_ = states::expect_named_rule;
                        handler_->end_json();
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        break;
                    }
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

                    state_ = states::dot_dot;
                    number_buffer_.clear();
                    is_negative_ = false;
                }
                    break;
                default:
                    precision_ = static_cast<uint8_t>(number_buffer_.length());
                    number_buffer_.push_back(static_cast<char>(*p_));
                    state_ = states::fraction;
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
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break; 
                    case '}':
                        end_integer_value();
                        if (!pop(modes::object_member_value))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_object(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case ']':
                        end_integer_value();
                        if (!pop(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_array(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
                        break;
                    case '.':
                        state_ = states::dot;
                        //precision_ = static_cast<uint8_t>(number_buffer_.length());
                        //number_buffer_.push_back(static_cast<char>(*p_));
                        //state_ = states::fraction;
                        break;
                    case ',':
                        end_integer_value();
                        begin_member_or_element();
                        break;
                    case 'e':case 'E':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::exp1;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::fraction: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        end_fraction_value();
                        break; 
                    case '}':
                        end_fraction_value();
                        if (!pop(modes::object_member_value))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_object(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case ']':
                        end_fraction_value();
                        if (!pop(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_array(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        ++precision_;
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::fraction;
                        break;
                    case ',':
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case 'e':case 'E':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::exp1;
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
                        state_ = states::exp2;
                        break;
                    case '-':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::exp2;
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::exp3;
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
                        state_ = states::exp3;
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
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        end_fraction_value();
                        break; 
                    case '}':
                        end_fraction_value();
                        if (!pop(modes::object_member_value))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_object(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case ']':
                        end_fraction_value();
                        if (!pop(modes::array_element))
                        {
                            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        }
                        handler_->end_array(*this);
                        if (peek() == modes::initial)
                        {
                            flip(modes::initial, modes::named_rule);
                            state_ = states::expect_named_rule;
                            handler_->end_json();
                        }
                        else
                        {
                            state_ = states::expect_comma_or_end;
                        }
                        break;
                    case ',':
                        end_fraction_value();
                        begin_member_or_element();
                        break;
                    case '0': 
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::exp3;
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::t: 
                while (p_ < end_input_ && literal_index_ < literal_.second)
                {
                    if (*p_ != literal_.first[literal_index_])
                    {
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_value, jcr_error_category()), *this);
                    }
                    ++p_;
                    ++literal_index_;
                    ++column_;
                }
                if (literal_index_ == literal_.second)
                {
                    handler_->value(true, *this);
                    if (peek() == modes::scalar)
                    {
                        flip(modes::scalar,modes::named_rule);
                        state_ = states::expect_named_rule;
                        handler_->end_json();
                    }
                    else
                    {
                        state_ = states::expect_comma_or_end;
                    }
                }
                break;
            case states::f:  
                while (p_ < end_input_ && literal_index_ < literal_.second)
                {
                    if (*p_ != literal_.first[literal_index_])
                    {
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_value, jcr_error_category()), *this);
                    }
                    ++p_;
                    ++literal_index_;
                    ++column_;
                }
                if (literal_index_ == literal_.second)
                {
                    handler_->value(false, *this);
                    if (peek() == modes::scalar)
                    {
                        flip(modes::scalar,modes::named_rule);
                        state_ = states::expect_named_rule;
                        handler_->end_json();
                    }
                    else
                    {
                        state_ = states::expect_comma_or_end;
                    }
                }
                break;
            case states::any_integer:  
            case states::any_string:  
                while (p_ < end_input_ && literal_index_ < literal_.second)
                {
                    if (*p_ != literal_.first[literal_index_])
                    {
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_value, jcr_error_category()), *this);
                    }
                    ++p_;
                    ++literal_index_;
                    ++column_;
                }
                if (literal_index_ == literal_.second)
                {
                    std::shared_ptr<rule<JsonT>> rule;
                    switch (state_)
                    {
                    case states::any_integer:  
                        rule = std::make_shared<any_integer_rule<JsonT>>();
                        break;
                    case states::any_string:  
                        rule = std::make_shared<any_string_rule<JsonT>>();
                        break;
                    }
                    switch (peek())
                    {
                    case modes::rule_member_value:
                        handler_->named_rule(rule_name_,
                                             std::make_shared<member_rule<JsonT>>(rule_member_name_,rule),*this);
                        flip(modes::rule_member_value,modes::named_rule);
                        state_ = states::expect_named_rule;
                        break;
                    case modes::scalar:
                        handler_->value(literal_.first,literal_.second, *this);
                        flip(modes::scalar,modes::named_rule);
                        state_ = states::expect_named_rule;
                        handler_->end_json();
                        break;
                    default:
                        handler_->value(literal_.first,literal_.second, *this);
                        state_ = states::expect_comma_or_end;
                        break;
                    }
                }
                break;
            case states::n: 
                while (p_ < end_input_ && literal_index_ < literal_.second)
                {
                    if (*p_ != literal_.first[literal_index_])
                    {
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_value, jcr_error_category()), *this);
                    }
                    ++p_;
                    ++literal_index_;
                    ++column_;
                }
                if (literal_index_ == literal_.second)
                {
                    handler_->value(null_type(), *this);
                    if (peek() == modes::scalar)
                    {
                        flip(modes::scalar,modes::named_rule);
                        state_ = states::expect_named_rule;
                        handler_->end_json();
                    }
                    else
                    {
                        state_ = states::expect_comma_or_end;
                    }
                }
                break;
            case states::slash: 
                {
                    switch (*p_)
                    {
                    case '*':
                        state_ = states::slash_star;
                        break;
                    case '/':
                        state_ = states::slash_slash;
                        break;
                    default:    
                        err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::slash_star:  
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
                        break;
                    case '*':
                        state_ = states::slash_star_star;
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::slash_slash: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = saved_state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = saved_state_;
                        state_ = states::lf;
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::slash_star_star: 
                {
                    switch (*p_)
                    {
                    case '/':
                        state_ = saved_state_;
                        break;
                    default:    
                        state_ = states::slash_star;
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_rule_value: 
                {
                    switch (*p_)
                    {
                    case '\r':
                        pre_line_break_state_ = state_;
                        state_ = states::cr;
                        break;
                    case '\n':
                        pre_line_break_state_ = state_;
                        state_ = states::lf;
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
                        break;
                    case '/':
                        saved_state_ = state_;
                        state_ = states::slash;
                        break;
                    case '\"':
                        state_ = states::string;
                        break;
                    case '-':
                        is_negative_ = true;
                        state_ = states::minus;
                        break;
                    case '0': 
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8': case '9':
                        number_buffer_.push_back(static_cast<char>(*p_));
                        state_ = states::integer;
                        break;
                    case 'f':
                        state_ = states::f;
                        literal_ = json_literals<char_type>::false_literal();
                        literal_index_ = 1;
                        break;
                    case 'i':
                        state_ = states::any_integer;
                        literal_ = jcr_char_traits<char_type>::integer_literal();
                        literal_index_ = 1;
                        break;
                    case 's':
                        state_ = states::any_string;
                        literal_ = jcr_char_traits<char_type>::string_literal();
                        literal_index_ = 1;
                        break;
                    case 'n':
                        state_ = states::n;
                        literal_ = json_literals<char_type>::null_literal();
                        literal_index_ = 1;
                        break;
                    case 't':
                        state_ = states::t;
                        literal_ = json_literals<char_type>::true_literal();
                        literal_index_ = 1;
                        break;
                    case '\'':
                        err_handler_->error(std::error_code(jcr_parser_errc::single_quote, jcr_error_category()), *this);
                        break;
                    default:
                        err_handler_->error(std::error_code(jcr_parser_errc::expected_value, jcr_error_category()), *this);
                        break;
                    }
                }
                ++p_;
                ++column_;
                break;
            case states::expect_named_rule:
                switch (*p_)
                {
                case '\r':
                    pre_line_break_state_ = state_;
                    state_ = states::cr;
                    break;
                case '\n':
                    pre_line_break_state_ = state_;
                    state_ = states::lf;
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
                    break;
                default:
                    if (('a' <=*p_ && *p_ <= 'z') || ('A' <=*p_ && *p_ <= 'Z'))
                    {
                        //flip(modes::object_member_name, modes::object_member_value);
                        string_buffer_.push_back(*p_);
                        state_ = states::rule_name;
                    }
                    else
                    {
                        //err_handler_->error(std::error_code(jcr_parser_errc::expected_name, jcr_error_category()), *this);
                    }
                    break;
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
        if (peek() == modes::scalar)
        {
            switch (state_)
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
        if (!pop(modes::named_rule))
        {
            err_handler_->error(std::error_code(jcr_parser_errc::unexpected_eof, jcr_error_category()), *this);
        }
    }

    states state() const
    {
        return state_;
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
            handler_->value(d, static_cast<uint8_t>(precision_), *this);
        }
        catch (...)
        {
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
            handler_->value(null_type(), *this); // recovery
        }
        number_buffer_.clear();
        is_negative_ = false;
        switch (stack_[top_])
        {
        case modes::array_element:
        case modes::object_member_value:
            state_ = states::expect_comma_or_end;
            break;
        case modes::scalar:
            flip(modes::scalar,modes::named_rule);
            state_ = states::expect_named_rule;
            handler_->end_json();
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
            break;
        }
    }

    void end_integer_value()
    {
        std::shared_ptr<rule<JsonT>> rule;
        if (is_negative_)
        {
            try
            {
                if (from_rule_)
                {
                }
                else
                {
                    int64_t val = string_to_integer(is_negative_, number_buffer_.data(), number_buffer_.length());
                    rule = std::make_shared<integer_rule<JsonT>>(val);
                }
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
                if (from_rule_)
                {
                }
                else
                {
                    uint64_t val = string_to_uinteger(number_buffer_.data(), number_buffer_.length());
                    rule = std::make_shared<uinteger_rule<JsonT>>(val);
                }
            }
            catch (const std::exception&)
            {
                err_handler_->fatal_error(std::error_code(jcr_parser_errc::invalid_number, jcr_error_category()), *this);
            }
        }

        switch (stack_[top_])
        {
        case modes::array_element:
        case modes::object_member_value:
            handler_->rule_definition(rule, *this);
            state_ = states::expect_comma_or_end;
            break;
        case modes::rule_member_value:
            handler_->named_rule(rule_name_,
                std::make_shared<member_rule<JsonT>>(rule_member_name_, rule),
                *this);
            flip(modes::rule_member_value, modes::named_rule);
            state_ = states::expect_named_rule;
            break;
        case modes::scalar:
            handler_->rule_definition(rule, *this);
            flip(modes::scalar, modes::named_rule);
            state_ = states::expect_named_rule;
            handler_->end_json();
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
            break;
        }
        number_buffer_.clear();
        is_negative_ = false;
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
            state_ = states::string;
            break;
        case '\\': 
            string_buffer_.push_back('\\');
            state_ = states::string;
            break;
        case '/':
            string_buffer_.push_back('/');
            state_ = states::string;
            break;
        case 'b':
            string_buffer_.push_back('\b');
            state_ = states::string;
            break;
        case 'f':  
            string_buffer_.push_back('\f');
            state_ = states::string;
            break;
        case 'n':
            string_buffer_.push_back('\n');
            state_ = states::string;
            break;
        case 'r':
            string_buffer_.push_back('\r');
            state_ = states::string;
            break;
        case 't':
            string_buffer_.push_back('\t');
            state_ = states::string;
            break;
        case 'u':
            cp_ = 0;
            state_ = states::u1;
            break;
        default:    
            err_handler_->error(std::error_code(jcr_parser_errc::illegal_escaped_character, jcr_error_category()), *this);
            break;
        }
    }

    void end_string_value(const char_type* s, size_t length) 
    {
        switch (stack_[top_])
        {
        case modes::object_member_name:
            handler_->name(s, length, *this);
            state_ = states::expect_colon;
            break;
        case modes::rule_member_name:
            rule_member_name_ = string_type(s, length);
            state_ = states::expect_colon;
            break;
        case modes::array_element:
        case modes::object_member_value:
            {
                auto rule = std::make_shared<string_rule<JsonT>>(s,length);
                handler_->rule_definition(rule, *this);
                state_ = states::expect_comma_or_end;
            }
            break;
        case modes::rule_member_value:
            {
                auto rule = std::make_shared<string_rule<JsonT>>(s,length);
                handler_->named_rule(rule_name_,
                                     std::make_shared<member_rule<JsonT>>(rule_member_name_,rule),*this);
                flip(modes::rule_member_value,modes::named_rule);
                state_ = states::expect_named_rule;
            }
            break;
        case modes::scalar:
            {
                auto rule = std::make_shared<string_rule<JsonT>>(s,length);
                handler_->rule_definition(rule, *this);
            }
            flip(modes::scalar,modes::named_rule);
            state_ = states::expect_named_rule;
            handler_->end_json();
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
            break;
        }
    }

    void begin_member_or_element() 
    {
        switch (stack_[top_])
        {
        case modes::object_member_value:
            // A comma causes a flip from object_member_value modes to object_member_name modes.
            flip(modes::object_member_value, modes::object_member_name);
            state_ = states::expect_member_name;
            break;
        case modes::array_element:
            state_ = states::expect_value;
            break;
        case modes::initial:
            break;
        default:
            err_handler_->error(std::error_code(jcr_parser_errc::invalid_json_text, jcr_error_category()), *this);
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

    bool push(modes mode)
    {
        ++top_;
        if (top_ >= depth_)
        {
            if (top_ >= max_depth_)
            {
                return false;
            }
            depth_ *= 2;
            stack_.resize(depth_);
        }
        stack_[top_] = mode;
        return true;
    }

    modes peek()
    {
        return stack_[top_];
    }

    bool peek(modes mode)
    {
        return stack_[top_] == mode;
    }

    void flip(modes mode1, modes mode2)
    {
        JSONCONS_ASSERT((top_ >= 0) && (stack_[top_] == mode1))
        stack_[top_] = mode2;
    }

    bool pop(modes mode)
    {
        if (top_ < 0 || stack_[top_] != mode)
        {
            return false;
        }
        --top_;
        return true;
    }
};

typedef basic_jcr_parser<json> jcr_parser;
typedef basic_jcr_parser<wjson> wjcr_parser;

}}

#endif

