// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONPATH_JSONQUERYSET_HPP
#define JSONCONS_JSONPATH_JSONQUERYSET_HPP

#include <vector>
#include "jsoncons/json.hpp"

namespace jsoncons { namespace jsonpath {

template <class Json>
class json_query_set
{
public:
    typedef typename Json::char_type char_type;
    typedef typename Json::object object;
    typedef typename Json::array array;

    json_query_set()
    {
    }
    json_query_set(const std::vector<Json*>& json_set)
        : json_set_(json_set)
    {
    }
    json_query_set(std::vector<Json*>&& json_set)
        : json_set_(std::forward<std::vector<Json*>>(json_set))
    {
    }

    //template<class T>
    //bool is() const
    //{
    //    return json_type_traits<json_type,T>::is(*this);
    //}

    size_t size() const JSONCONS_NOEXCEPT
    {
        return json_set_.size();
    }

    bool is_string() const JSONCONS_NOEXCEPT
    {
        return false;
    }


    bool is_bool() const JSONCONS_NOEXCEPT
    {
        return false;
    }

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return false;
    }

    bool is_array() const JSONCONS_NOEXCEPT
    {
        return true;
    }

    void write(std::basic_ostream<char_type>& os, const basic_output_format<char_type>& format, bool indenting) const
    {
        basic_json_serializer<char_type> serializer(os, format, indenting);
        write(serializer);
    }

    void write(basic_json_output_handler<char_type>& handler) const
    {
        handler.begin_json();
        write_body(handler);
        handler.end_json();
    }

    void write_body(basic_json_output_handler<char_type>& handler) const
    {
        handler.begin_array();
        for (const auto& p: json_set_)
        {
            p->write_body(handler);
        }
        handler.end_array();
    }
private:
    std::vector<Json*> json_set_;
};


}}

#endif
