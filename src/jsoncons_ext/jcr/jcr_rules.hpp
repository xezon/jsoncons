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

    virtual bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const 
    {
        return validate(val,false,rules);
    }
    virtual bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const = 0;
    virtual bool repeat() const
    {
        return false;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        if (!val.is_string())
        {
            return false;
        }
        bool result = true;
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
                    result = false;
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
                        result = false;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_object();
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return rule1_->validate(val,optional,rules) && rule2_->validate(val,optional,rules);
    }
};

template <class JsonT>
class any_integer_rule : public rule<JsonT>
{
public:
    any_integer_rule()
    {
    }

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_integer() || val.as_uinteger();
    }
};

template <class JsonT>
class true_rule : public rule<JsonT>
{
public:
    true_rule()
    {
    }

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return true;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_string() && val.as_string() == s_;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        if (!val.is_object())
        {
            return false;
        }
        auto it = val.find(name_);
        if (it == val.members().end())
        {
            return optional ? true : false;
        }
        
        return rule_->validate(it->value(), false, rules);
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return rule_->validate(val, true, rules);
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

    bool repeat() const override
    {
        return true;
    }

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return rule_->validate(val, optional, rules);
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        auto it = rules.find(name_);
        if (it == rules.end())
        {
            return false;
        }
        return it->second->validate(val,optional,rules);
    }
};

template <class JsonT>
class any_string_rule : public rule<JsonT>
{
public:
    any_string_rule()
    {
    }

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_string();
    }
};

template <class JsonT>
class null_rule : public rule<JsonT>
{
public:
    null_rule()
    {
    }

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_null();
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is<T>() && val.as<T>() == value_;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is<T>() && val.as<T>() >= from_;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is<T>() && val.as<T>() <= to_;
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        bool result = true;
        for (auto element : members_)
        {
            result = element->validate(val, optional, rules);
            if (!result)
            {
                break;
            }
        }
        return result;
    }
private:
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

    bool validate(const json_type& val, bool optional, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        if (!val.is_array())
        {
            return false;
        }
        if (val.size() < elements_.size())
        {
            return false;
        }
        bool result = true;

        bool done = false;
        for (size_t i = 0; !done && result && i < elements_.size(); ++i)
        {
            result = elements_[i]->validate(val[i], optional, rules);
            if (elements_[i]->repeat())
            {
                for (size_t j = i+1; result && j < val.size(); ++j)
                {
                    result = elements_[i]->validate(val[j], optional, rules);
                }
                done = true;
            }
        }
        return result;
    }
private:
    array_rule<JsonT>& operator=(const array_rule<JsonT>&);
};



}}

#endif
