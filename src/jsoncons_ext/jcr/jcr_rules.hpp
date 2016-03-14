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

    virtual bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const = 0;
    virtual rule* clone() const = 0;
    virtual ~rule()
    {
    }

    virtual void insert(move_iterator first, move_iterator last)
    {
    }
};

template <class JsonT>
class any_object_rule : public rule<JsonT>
{
public:
    any_object_rule()
    {
    }

    rule<JsonT>* clone() const override
    {
        return new any_object_rule();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new composite_rule(rule1_,rule2_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return rule1_->validate(val,rules) && rule2_->validate(val,rules);
    }
};

template <class JsonT>
class any_integer_rule : public rule<JsonT>
{
public:
    any_integer_rule()
    {
    }

    rule<JsonT>* clone() const override
    {
        return new any_integer_rule();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new true_rule();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new string_rule(s_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new member_rule(name_,rule_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        if (!val.is_object())
        {
            return false;
        }
        auto it = val.find(name_);
        if (it == val.members().end())
        {
            return false;
        }
        
        return rule_->validate(it->value(), rules);
    }
};

template <class JsonT>
class optional_member_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;

    string_type name_;
    std::shared_ptr<rule<JsonT>> rule_;
public:
    optional_member_rule(const string_type& name, std::shared_ptr<rule<JsonT>> rule)
        : name_(name),rule_(rule)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new optional_member_rule(name_,rule_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        if (!val.is_object())
        {
            return false;
        }
        auto it = val.find(name_);
        if (it == val.members().end())
        {
            return true;
        }
        
        return rule_->validate(it->value(), rules);
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

    rule<JsonT>* clone() const override
    {
        return new jcr_rule_name(name_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        auto it = rules.find(name_);
        if (it == rules.end())
        {
            return false;
        }
        return it->second->validate(val,rules);
    }
};

template <class JsonT>
class any_string_rule : public rule<JsonT>
{
public:
    any_string_rule()
    {
    }

    rule<JsonT>* clone() const override
    {
        return new any_string_rule();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new null_rule();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new value_rule(value_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new from_rule(from_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
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

    rule<JsonT>* clone() const override
    {
        return new to_rule(to_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is<T>() && val.as<T>() <= to_;
    }
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
    std::vector<value_type,allocator_type> members_;
public:
    group_rule(const allocator_type& allocator = allocator_type())
        : members_(allocator)
    {
    }

    group_rule(const group_rule<JsonT>& val)
        : members_(val.members_)
    {
    }

    group_rule(group_rule&& val)
        : members_(std::move(val.members_))
    {
    }

    group_rule(const group_rule<JsonT>& val, const allocator_type& allocator) :
        members_(val.members_,allocator)
    {
    }

    group_rule(group_rule&& val,const allocator_type& allocator) :
        members_(std::move(val.members_),allocator)
    {
    }

    void add_rule(std::shared_ptr<rule<JsonT>> rule)
    {
        members_.push_back(rule);
    }

    rule* clone() const override
    {
        return new group_rule(*this);
    }

    allocator_type get_allocator() const
    {
        return members_.get_allocator();
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        bool result = true;
        for (auto element : members_)
        {
            result = element->validate(val, rules);
            if (!result)
            {
                break;
            }
        }
        return result;
    }
private:
    group_rule<JsonT>& operator=(const group_rule<JsonT>&);
};



}}

#endif
