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

    virtual bool is_object() const
    {
        return false;
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

template <class JsonT>
class boolean_rule : public rule<JsonT>
{
    bool val_;

public:
    boolean_rule(bool val)
        : val_(val)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new boolean_rule(val_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_bool() && val.as_bool() == val_;
    }
};

template <class JsonT>
class double_rule : public rule<JsonT>
{
    double val_;
    uint8_t precision_;

public:
    double_rule(double val, uint8_t precision)
        : val_(val), precision_(precision)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new double_rule(val_,precision_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_double() && val.as_double() == val_;
    }
};

template <class JsonT>
class integer_rule : public rule<JsonT>
{
    int64_t val_;

public:
    integer_rule(int64_t val)
        : val_(val)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new integer_rule(val_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_integer() && val.as_integer() == val_;
    }
};

template <class JsonT>
class uinteger_rule : public rule<JsonT>
{
    uint64_t val_;
    uint64_t to_;

public:
    uinteger_rule(uint64_t val)
        : val_(val)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new uinteger_rule(val_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_uinteger() && val.as_uinteger() == val_;
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
class integer_range_rule : public rule<JsonT>
{
    int64_t from_;
    int64_t to_;

public:
    integer_range_rule(int64_t from, int64_t to)
        : from_(from), to_(to)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new integer_range_rule(from_,to_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_integer() && val.as_integer() >= from_ && val.as_integer() <= to_;
    }
};

template <class JsonT>
class uinteger_range_rule : public rule<JsonT>
{
    uint64_t from_;
    uint64_t to_;
public:
    uinteger_range_rule(uint64_t from, uint64_t to)
        : from_(from), to_(to)
    {
    }

    rule<JsonT>* clone() const override
    {
        return new uinteger_range_rule(from_,to_);
    }

    bool validate(const json_type& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        return val.is_uinteger() && val.as_uinteger() >= from_ && val.as_uinteger() <= to_;
    }
};

template <class JsonT, class Alloc>
class array_rule : public rule<JsonT>
{
public:
    typedef Alloc allocator_type;
    typedef typename JsonT json_type;
    typedef std::shared_ptr<rule<JsonT>> value_type;
    typedef typename JsonT::array array;
    typedef typename std::allocator_traits<Alloc>:: template rebind_alloc<JsonT> vector_allocator_type;
    typedef typename std::vector<value_type,Alloc>::reference reference;
    typedef typename std::vector<value_type,Alloc>::const_reference const_reference;
    typedef typename std::vector<value_type,Alloc>::iterator iterator;
    typedef typename std::vector<value_type,Alloc>::const_iterator const_iterator;

    array_rule()
        : elements_()
    {
    }

    explicit array_rule(const Alloc& allocator)
        : elements_(allocator)
    {
    }

    explicit array_rule(size_t n, const Alloc& allocator = Alloc())
        : elements_(n,JsonT(),allocator)
    {
    }

    explicit array_rule(size_t n, const JsonT& value, const Alloc& allocator = Alloc())
        : elements_(n,value,allocator)
    {
    }

    template <class InputIterator>
    array_rule(InputIterator begin, InputIterator end, const Alloc& allocator = Alloc())
        : elements_(begin,end,allocator)
    {
    }

    array_rule(const array_rule& val)
        : elements_(val.elements_)
    {
    }

    array_rule(const array_rule& val, const Alloc& allocator)
        : elements_(val.elements_,allocator)
    {
    }
    array_rule(array_rule&& val)
        : elements_(std::move(val.elements_))
    {
    }
    array_rule(array_rule&& val, const Alloc& allocator)
        : elements_(std::move(val.elements_),allocator)
    {
    }

    array_rule(std::initializer_list<JsonT> init, 
               const Alloc& allocator = Alloc())
        : elements_(std::move(init),allocator)
    {
    }

    void insert(move_iterator first, move_iterator last) override
    {
        size_t count = std::distance(first,last);
        size_t pos = elements_.size();
        elements_.resize(pos+count);
        auto d = elements_.begin()+pos;
        for (auto s = first; s != last; ++s, ++d)
        {
            *d = *s;
        }
    }

    rule* clone() const override
    {
        return new array_rule(*this);
    }

    bool validate(const JsonT& val, const std::map<string_type,std::shared_ptr<rule_type>>& rules) const override
    {
        bool result = false;
        return result;
    }

    Alloc get_allocator() const
    {
        return elements_.get_allocator();
    }

    void swap(array_rule<JsonT,Alloc>& val)
    {
        elements_.swap(val.elements_);
    }

    JsonT& operator[](size_t i) {return elements_[i];}

    const JsonT& operator[](size_t i) const {return elements_[i];}

    iterator begin() {return elements_.begin();}

    iterator end() {return elements_.end();}

    const_iterator begin() const {return elements_.begin();}

    const_iterator end() const {return elements_.end();}

    bool operator==(const array_rule<JsonT,Alloc>& rhs) const
    {
        if (size() != rhs.size())
        {
            return false;
        }
        for (size_t i = 0; i < size(); ++i)
        {
            if (elements_[i] != rhs.elements_[i])
            {
                return false;
            }
        }
        return true;
    }
private:
    array_rule& operator=(const array_rule<JsonT,Alloc>&);
    std::vector<value_type,Alloc> elements_;
};

template <class StringT,class JsonT,class Alloc>
class object_rule : public rule<JsonT>
{
public:
    typedef typename JsonT json_type;
    typedef Alloc allocator_type;
    typedef StringT string_type;
    typedef std::shared_ptr<rule<JsonT>> value_type;
private:
    std::vector<value_type,allocator_type> members_;
public:
    object_rule(const allocator_type& allocator = allocator_type())
        : members_(allocator)
    {
    }

    object_rule(const object_rule<StringT,JsonT,Alloc>& val)
        : members_(val.members_)
    {
    }

    object_rule(object_rule&& val)
        : members_(std::move(val.members_))
    {
    }

    object_rule(const object_rule<StringT,JsonT,Alloc>& val, const allocator_type& allocator) :
        members_(val.members_,allocator)
    {
    }

    object_rule(object_rule&& val,const allocator_type& allocator) :
        members_(std::move(val.members_),allocator)
    {
    }

    bool is_object() const override
    {
        return true;
    }

    void insert(move_iterator first, move_iterator last) override
    {
        size_t count = std::distance(first,last);
        size_t pos = members_.size();
        members_.resize(pos+count);
        auto d = members_.begin()+pos;
        for (auto s = first; s != last; ++s, ++d)
        {
            *d = *s;
        }
    }

    void add_rule(std::shared_ptr<rule<JsonT>> rule)
    {
        members_.push_back(rule);
    }

    rule* clone() const override
    {
        return new object_rule(*this);
    }

    Alloc get_allocator() const
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
    object_rule<StringT,JsonT,Alloc>& operator=(const object_rule<StringT,JsonT,Alloc>&);
};



}}

#endif
