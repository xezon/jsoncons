// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_RULES_HPP
#define JSONCONS_JCR_JCR_RULES_HPP

#include <string>
#include <vector>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <utility>
#include <initializer_list>
#include <map>

namespace jsoncons { namespace jcr {

template <class JsonT>
class rule
{
public:
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename JsonT json_type;

    typedef typename std::vector<std::shared_ptr<rule<JsonT>>>::iterator iterator;
    typedef std::move_iterator<iterator> move_iterator;
    typedef rule<JsonT> rule_type;

    enum class status
    {
        pass, fail, repeat
    };
private:
    virtual status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const = 0;
public:

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const 
    {
        return do_validate(val,false,rules,0) == status::pass ? true : false;
    }

    status validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const 
    {
        return do_validate(val,optional,rules, pos);
    }
    virtual ~rule()
    {
    }

    virtual void insert(move_iterator first, move_iterator last)
    {
    }
};

template <class JsonT>
class uri_rule :  public rule<JsonT>
{
    enum class states
    {
        start,
        scheme,
        expect_path
    };
public:
    typedef typename JsonT::string_type string_type;
    typedef typename JsonT::char_type char_type;
private:
    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        if (!val.is_string())
        {
            return status::fail;
        }
        status result = status::pass;
        std::string s = val.as_string();

        states state = states::scheme;

        bool done = false;
        for (const char_type* p = s.data(); !done && p < (s.data() + s.length()); ++p)
        {
            switch (state)
            {
            case states::start:
                if (('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z'))
                {
                    state = states::scheme;
                }
                else
                {
                    done = true;
                    result = status::fail;
                }
                break;
            case states::scheme:
                switch (*p)
                {
                case ':':
                    state = states::expect_path;
                    break;
                case '+': case '-': case '.':
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                    break;
                default:
                    if (!(('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z')))
                    {
                        done = true;
                        result = status::fail;
                    }
                    break;
                }
                break;
            case states::expect_path:
                switch (*p)
                {
                case '/':
                    state = states::expect_path;
                    break;
                }
                break;
            }
        }

        return result;
    }
};

template <class JsonT>
class any_object_rule : public rule<JsonT>
{
public:
    any_object_rule()
    {
    }
private:
    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_object() ? status::pass : status::fail;
    }
};

template <class JsonT>
class composite_rule : public rule<JsonT>
{
    std::shared_ptr<rule<JsonT>> rule1_;
    std::shared_ptr<rule<JsonT>> rule2_;
public:
    composite_rule(std::shared_ptr<rule<JsonT>> rule1, 
                   std::shared_ptr<rule<JsonT>> rule2)
        : rule1_(rule1), rule2_(rule2)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return rule1_->validate(val,optional,rules, pos) == status::pass
        && rule2_->validate(val,optional,rules, pos) == status::pass 
        ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_integer_rule : public rule<JsonT>
{
public:
    any_integer_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_integer() || val.is_uinteger() ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_float_rule : public rule<JsonT>
{
public:
    any_float_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_double() ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_boolean_rule : public rule<JsonT>
{
public:
    any_boolean_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_bool() ? status::pass : status::fail;
    }
};

template <class JsonT>
class true_rule : public rule<JsonT>
{
public:
    true_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return status::pass;
    }
};

template <class JsonT>
class string_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    string_type s_;
public:
    string_rule(const char_type* p, size_t length, string_allocator sa)
        : s_(p,length,sa)
    {
    }
    string_rule(const char_type* p, size_t length)
        : s_(p,length)
    {
    }
    string_rule(const string_type& s)
        : s_(s)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_string() && val.as_string() == s_ ? status::pass : status::fail;
    }
};

template <class JsonT>
class member_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    string_type name_;
    std::shared_ptr<rule<JsonT>> rule_;
public:
    member_rule(const string_type& name, std::shared_ptr<rule<JsonT>> rule)
        : name_(name),rule_(rule)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        if (!val.is_object())
        {
            return status::fail;
        }
        auto it = val.find(name_);
        if (it == val.members().end())
        {
            return optional ? status::pass : status::fail;
        }
        
        return rule_->validate(it->value(), false,rules, pos);
    }
};

template <class JsonT>
class optional_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    std::shared_ptr<rule<JsonT>> rule_;
public:
    optional_rule(std::shared_ptr<rule<JsonT>> rule)
        : rule_(rule)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return rule_->validate(val, true,rules, pos);
    }
};

template <class JsonT>
class repeating_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    std::shared_ptr<rule<JsonT>> rule_;
public:
    repeating_rule(std::shared_ptr<rule<JsonT>> rule)
        : rule_(rule)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return rule_->validate(val, optional,rules, pos) == status::fail ? status::fail : status::repeat;
    }
};

template <class JsonT>
class jcr_rule_name : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    string_type name_;
public:
    jcr_rule_name(const char_type* p, size_t length, string_allocator sa)
        : name_(p,length,sa)
    {
    }
    jcr_rule_name(const string_type& name)
        : name_(name)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        auto it = rules.find(name_);
        if (it == rules.end())
        {
            return status::fail;
        }
        return it->second->validate(val,optional,rules, pos);
    }
};

template <class JsonT>
class any_string_rule : public rule<JsonT>
{
public:
    any_string_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_string() ? status::pass : status::fail;
    }
};

template <class JsonT>
class null_rule : public rule<JsonT>
{
public:
    null_rule()
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is_null() ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class value_rule : public rule<JsonT>
{
    T value_;

public:
    value_rule(T value)
        : value_(value)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is<T>() && val.as<T>() == value_ ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class from_rule : public rule<JsonT>
{
    T from_;

public:
    from_rule(T from)
        : from_(from)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is<T>() && val.as<T>() >= from_ ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class to_rule : public rule<JsonT>
{
    T to_;

public:
    to_rule(T to)
        : to_(to)
    {
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        return val.is<T>() && val.as<T>() <= to_ ? status::pass : status::fail;
    }
};

template <class JsonT>
class object_rule : public rule<JsonT>
{
public:
    typedef typename JsonT json_type;
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef std::shared_ptr<rule<JsonT>> value_type;
private:
    std::vector<value_type,allocator_type> members_;
public:
    object_rule(const allocator_type& allocator = allocator_type())
        : members_(allocator)
    {
    }

    object_rule(const object_rule<JsonT>& val)
        : members_(val.members_)
    {
    }

    object_rule(object_rule&& val)
        : members_(std::move(val.members_))
    {
    }

    object_rule(const object_rule<JsonT>& val, const allocator_type& allocator) :
        members_(val.members_,allocator)
    {
    }

    object_rule(object_rule&& val,const allocator_type& allocator) :
        members_(std::move(val.members_),allocator)
    {
    }

    void add_rule(std::shared_ptr<rule<JsonT>> rule)
    {
        members_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return members_.get_allocator();
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        status result = status::pass;
        for (auto element : members_)
        {
            result = element->validate(val, optional,rules, pos);
            if (result == status::fail)
            {
                break;
            }
        }
        return result;
    }

    object_rule<JsonT>& operator=(const object_rule<JsonT>&);
};

template <class JsonT>
class array_rule : public rule<JsonT>
{
public:
    typedef typename JsonT json_type;
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef std::shared_ptr<rule<JsonT>> value_type;
private:
    std::vector<value_type,allocator_type> elements_;
public:
    array_rule(const allocator_type& allocator = allocator_type())
        : elements_(allocator)
    {
    }

    array_rule(const array_rule<JsonT>& val)
        : elements_(val.elements_)
    {
    }

    array_rule(array_rule&& val)
        : elements_(std::move(val.elements_))
    {
    }

    array_rule(const array_rule<JsonT>& val, const allocator_type& allocator) :
        elements_(val.elements_,allocator)
    {
    }

    array_rule(array_rule&& val,const allocator_type& allocator) :
        elements_(std::move(val.elements_),allocator)
    {
    }

    void add_rule(std::shared_ptr<rule<JsonT>> rule)
    {
        elements_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return elements_.get_allocator();
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        if (!val.is_array())
        {
            return status::fail;
        }
        if (val.size() < elements_.size())
        {
            return status::fail;
        }
        status result = status::pass;

        for (size_t i = 0, j = 0; result != status::fail && i < elements_.size() && j < val.size(); ++i)
        {
            size_t pos = 0;
            do
            {
                result = elements_[i]->validate(val[j], optional,rules, pos);
                ++j;
                ++pos;
            }
            while (result == status::repeat && j < val.size());
        }
        return result == status::fail ? status::fail : status::pass;
    }

    array_rule<JsonT>& operator=(const array_rule<JsonT>&);
};

template <class JsonT>
class group_rule : public rule<JsonT>
{
public:
    typedef typename JsonT json_type;
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef std::shared_ptr<rule<JsonT>> value_type;
private:
    std::vector<value_type,allocator_type> elements_;
public:
    group_rule(const allocator_type& allocator = allocator_type())
        : elements_(allocator)
    {
    }

    group_rule(const group_rule<JsonT>& val)
        : elements_(val.elements_)
    {
    }

    group_rule(group_rule&& val)
        : elements_(std::move(val.elements_))
    {
    }

    group_rule(const group_rule<JsonT>& val, const allocator_type& allocator) :
        elements_(val.elements_,allocator)
    {
    }

    group_rule(group_rule&& val,const allocator_type& allocator) :
        elements_(std::move(val.elements_),allocator)
    {
    }

    void add_rule(std::shared_ptr<rule<JsonT>> rule)
    {
        elements_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return elements_.get_allocator();
    }
private:

    status do_validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules, size_t pos) const override
    {
        status result = status::pass;
        if (pos < elements_.size())
        {
            result = elements_[pos]->validate(val,optional,rules, pos);
            if (result == status::fail)
            {
                return result;
            }
        }
        return (pos+1) < elements_.size() ? status::repeat : status::pass;
    }

    group_rule<JsonT>& operator=(const group_rule<JsonT>&);
};



}}

#endif
