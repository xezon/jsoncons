/// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_ERROR_CATEGORY_HPP
#define JSONCONS_JCR_JCR_ERROR_CATEGORY_HPP

#include "jsoncons/jsoncons.hpp"
#include <system_error>

namespace jsoncons { namespace jcr {

namespace jcr_parser_errc 
{
    const int unexpected_eof = 0;
    const int invalid_jcr_text = 1;
    const int extra_character = 2;
    const int max_depth_exceeded = 3;
    const int single_quote = 4;
    const int illegal_character_in_string = 5;
    const int extra_comma = 6;
    const int expected_name = 7;
    const int expected_value = 8;
    const int invalid_value = 9;
    const int expected_colon = 10;
    const int illegal_control_character = 11;
    const int illegal_escaped_character = 12;
    const int expected_codepoint_surrogate_pair = 13;
    const int invalid_hex_escape_sequence = 14;
    const int invalid_unicode_escape_sequence = 15;
    const int leading_zero = 16;
    const int invalid_number = 17;
    const int expected_comma_or_right_brace = 18;
    const int expected_comma_or_right_bracket = 19;
    const int unexpected_right_bracket = 20;
    const int unexpected_right_brace = 21;
    const int expected_rule_or_value = 22;
    const int expected_star = 23;
}

class jcr_error_category_impl
   : public std::error_category
{
public:
    virtual const char* name() const JSONCONS_NOEXCEPT
    {
        return "jcr";
    }
    virtual std::string message(int ev) const
    {
        switch (ev)
        {
        case jcr_parser_errc::unexpected_eof:
            return "Unexpected end of file";
        case jcr_parser_errc::invalid_jcr_text:
            return "Invalid JCR text";
        case jcr_parser_errc::extra_character:
            return "Unexpected non-whitespace character after JSON text";
        case jcr_parser_errc::max_depth_exceeded:
            return "Maximum JSON depth exceeded";
        case jcr_parser_errc::single_quote:
            return "JSON strings cannot be quoted with single quotes";
        case jcr_parser_errc::illegal_character_in_string:
            return "Illegal character in string";
        case jcr_parser_errc::extra_comma:
            return "Extra comma";
        case jcr_parser_errc::expected_name:
            return "Expected object member name";
        case jcr_parser_errc::expected_value:
            return "Expected value";
        case jcr_parser_errc::invalid_value:
            return "Invalid value";
        case jcr_parser_errc::expected_colon:
            return "Expected name separator ':'";
        case jcr_parser_errc::illegal_control_character:
            return "Illegal control character in string";
        case jcr_parser_errc::illegal_escaped_character:
            return "Illegal escaped character in string";
        case jcr_parser_errc::expected_codepoint_surrogate_pair:
            return "Invalid codepoint, expected another \\u token to begin the second half of a codepoint surrogate pair.";
        case jcr_parser_errc::invalid_hex_escape_sequence:
            return "Invalid codepoint, expected hexadecimal digit.";
        case jcr_parser_errc::invalid_unicode_escape_sequence:
            return "Invalid codepoint, expected four hexadecimal digits.";
        case jcr_parser_errc::leading_zero:
            return "A number cannot have a leading zero";
        case jcr_parser_errc::invalid_number:
            return "Invalid number";
        case jcr_parser_errc::expected_comma_or_right_brace:
            return "Expected comma or right brace ']'";
        case jcr_parser_errc::expected_comma_or_right_bracket:
            return "Expected comma or right bracket ']'";
        case jcr_parser_errc::unexpected_right_brace:
            return "Unexpected right brace '}'";
        case jcr_parser_errc::unexpected_right_bracket:
            return "Unexpected right bracket ']'";
        case jcr_parser_errc::expected_rule_or_value:
            return "Expected rule name or value";
        case jcr_parser_errc::expected_star:
            return "Expected '*'";
        default:
            return "Unknown JCR parser error";
        }
    }
};

inline
const std::error_category& jcr_error_category()
{
  static jcr_error_category_impl instance;
  return instance;
}

}}
#endif
