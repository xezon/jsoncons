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
#include <regex>

namespace jsoncons { namespace jcr {

enum class status
{
    pass, fail, may_repeat, must_repeat
};

template <class JsonT>
class rule
{
public:
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;

    typedef typename std::vector<std::shared_ptr<rule<JsonT>>>::iterator iterator;
    typedef std::move_iterator<iterator> move_iterator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;
private:
    virtual status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const = 0;

public:

    virtual void base_rule(std::shared_ptr<rule<JsonT>> rule_ptr)
    {
    }

    bool validate(const JsonT& val, const name_rule_map& rules) const 
    {
        return do_validate(val,false,rules,0) == status::pass ? true : false;
    }

    status validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const 
    {
        return do_validate(val,optional,rules, index);
    }
    virtual ~rule()
    {
    }
};

template <class JsonT>
class uri_rule :  public rule<JsonT>
{
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

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
    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
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
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    any_object_rule()
    {
    }
private:
    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_object() ? status::pass : status::fail;
    }
};

template <class JsonT>
class composite_rule : public rule<JsonT>
{
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type, std::shared_ptr<rule_type>> name_rule_map;

    std::shared_ptr<rule<JsonT>> rule1_;
    std::shared_ptr<rule<JsonT>> rule2_;
public:
    composite_rule(std::shared_ptr<rule<JsonT>> rule1, 
                   std::shared_ptr<rule<JsonT>> rule2)
        : rule1_(rule1), rule2_(rule2)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return rule1_->validate(val,optional,rules, index) == status::pass
        && rule2_->validate(val,optional,rules, index) == status::pass 
        ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_integer_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    any_integer_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_integer() || val.is_uinteger() ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_float_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    any_float_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_double() ? status::pass : status::fail;
    }
};

template <class JsonT>
class any_boolean_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    any_boolean_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_bool() ? status::pass : status::fail;
    }
};

template <class JsonT>
class true_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    true_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
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
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

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

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_string() && val.as_string() == s_ ? status::pass : status::fail;
    }
};

template <class JsonT>
class string_pattern_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    string_type pattern_;
    std::regex::flag_type flags_;
public:
    string_pattern_rule(const char_type* p, size_t length, string_allocator sa)
        : pattern_(p,length,sa), flags_(std::regex_constants::ECMAScript)
    {
    }
    string_pattern_rule(const char_type* p, size_t length)
        : pattern_(p,length), flags_(std::regex_constants::ECMAScript)
    {
    }
    string_pattern_rule(const string_type& s)
        : pattern_(s), flags_(std::regex_constants::ECMAScript)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        if (!val.is_string())
        {
            return false;
        }
        std::basic_regex<char_type> pattern(pattern_,flags_);

        return std::regex_match(val.as_string(), pattern) ? status::pass : status::fail;
    }
};

template <class JsonT>
class member_rule : public rule<JsonT>
{
public:
};

template <class JsonT>
class qstring_member_rule : public member_rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    string_type name_;
    std::shared_ptr<rule<JsonT>> rule_;

    size_t min_repetitions_;
    size_t max_repetitions_;
public:
    qstring_member_rule(const string_type& name,
                        size_t min_repetitions, size_t max_repetitions)
        : name_(name), 
          min_repetitions_(min_repetitions),
          max_repetitions_(max_repetitions)
    {
    }

    void base_rule(std::shared_ptr<rule<JsonT>> rule_ptr) override
    {
        rule_ = rule_ptr;
    }
private:

    status do_validate(const JsonT& val,
                       bool optional, const name_rule_map& rules, size_t index) const override
    {
        if (!val.is_object())
        {
            return status::fail;
        }
        auto it = val.find(name_);
        if (it == val.members().end())
        {
            return optional || min_repetitions_ == 0 ? status::pass : status::fail;
        }
        
        return rule_->validate(it->value(), false,rules, index);
    }
};

template <class JsonT>
class regex_member_rule : public member_rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    string_type name_pattern_;
    std::regex::flag_type flags_;
    std::shared_ptr<rule<JsonT>> rule_;

    size_t min_repetitions_;
    size_t max_repetitions_;
public:
    regex_member_rule(const string_type& name_pattern,
                      size_t min_repetitions, size_t max_repetitions)
        : name_pattern_(name_pattern), 
          min_repetitions_(min_repetitions),
          max_repetitions_(max_repetitions),
          flags_(std::regex_constants::ECMAScript)
    {
    }

    void base_rule(std::shared_ptr<rule<JsonT>> rule_ptr) override
    {
        rule_ = rule_ptr;
    }
private:

    status do_validate(const JsonT& val,
                       bool optional, const name_rule_map& rules, size_t index) const override
    {
        if (!val.is_object())
        {
            return status::fail;
        }
        std::basic_regex<char_type> pattern(name_pattern_,flags_);

        auto it = val.members().begin();
        auto end = val.members().end();

        status result = status::pass;
        size_t count = 0;
        if (val.size() > 0)
        {
            result = status::fail;
            while (it != end && count < max_repetitions_)
            {
                if (std::regex_match(it->name(), pattern))
                {
                    result = rule_->validate(it->value(),optional,rules,index);
                    if (result != status::fail)
                    {
                        ++count;
                    }
                }
                ++it;
            }
        }
        
        return count < min_repetitions_ ? status::fail : status::pass;
    }
};

template <class JsonT>
class optional_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    std::shared_ptr<rule<JsonT>> rule_;
public:
    optional_rule(std::shared_ptr<rule<JsonT>> rule)
        : rule_(rule)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return rule_->validate(val, true,rules, index);
    }
};

template <class JsonT>
class repeat_array_item_rule : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    std::shared_ptr<rule<JsonT>> rule_;
    size_t min_;
    size_t max_;
public:
    repeat_array_item_rule()
        : min_(0), 
          max_(std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP())
    {
    }
    repeat_array_item_rule(size_t min_repitition)
        : min_(min_repitition), 
          max_(std::numeric_limits<size_t>::max JSONCONS_NO_MACRO_EXP())
    {
    }
    repeat_array_item_rule(size_t min_repitition, size_t max_repitition)
        : min_(min_repitition), 
          max_(max_repitition)
    {
    }

    repeat_array_item_rule(std::shared_ptr<rule<JsonT>> rule, size_t min, size_t max)
        : rule_(rule),
          min_(min), 
          max_(max)
    {
    }

    void base_rule(std::shared_ptr<rule<JsonT>> rule_ptr) override
    {
        rule_ = rule_ptr;
    }

private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        if (index >= max_)
        {
            return status::fail;
        }
        status result = rule_->validate(val, optional,rules, index);
        if (result == status::fail)
        {
            return status::fail;
        }

        return index + 1 < min_ ? status::must_repeat : status::may_repeat;
    }
};

template <class JsonT>
class jcr_rule_name : public rule<JsonT>
{
    typedef typename JsonT::string_type string_type;
    typedef typename string_type::value_type char_type;
    typedef typename string_type::allocator_type string_allocator;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

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

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        auto it = rules.find(name_);
        if (it == rules.end())
        {
            return status::fail;
        }
        return it->second->validate(val,optional,rules, index);
    }
};

template <class JsonT>
class any_string_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    any_string_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_string() ? status::pass : status::fail;
    }
};

template <class JsonT>
class null_rule : public rule<JsonT>
{
public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    null_rule()
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.is_null() ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class value_rule : public rule<JsonT>
{
    T value_;

public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    value_rule(T value)
        : value_(value)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.template is<T>() && val.template as<T>() == value_ ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class from_rule : public rule<JsonT>
{
    T from_;

public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    from_rule(T from)
        : from_(from)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.template is<T>() && val.template as<T>() >= from_ ? status::pass : status::fail;
    }
};

template <class JsonT, typename T>
class to_rule : public rule<JsonT>
{
    T to_;

public:
    typedef rule<JsonT> rule_type;
    typedef typename JsonT::string_type string_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;

    to_rule(T to)
        : to_(to)
    {
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        return val.template is<T>() && val.template as<T>() <= to_ ? status::pass : status::fail;
    }
};

template <class JsonT>
class object_rule : public rule<JsonT>
{
public:
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef std::shared_ptr<rule<JsonT>> rule_ptr_type;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;
private:
    bool sequence_;
    std::vector<rule_ptr_type,allocator_type> members_;
public:
    object_rule(const allocator_type& allocator = allocator_type())
        : sequence_(true), members_(allocator)
    {
    }

    void add_rule(bool sequence, std::shared_ptr<rule<JsonT>> rule)
    {
        if (members_.size() > 0)
        {
            sequence_ = sequence;
        }
        members_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return members_.get_allocator();
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        status result = status::pass;
        for (auto element : members_)
        {
            result = element->validate(val, optional,rules, index);
            if (sequence_ && result == status::fail)
            {
                return result;
            }
            else if (!sequence_ && result == status::pass)
            {
                return result;
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
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef std::shared_ptr<rule<JsonT>> rule_ptr_type;
    typedef rule<JsonT> rule_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;
private:
    bool sequence_;
    std::vector<rule_ptr_type,allocator_type> elements_;
public:
    array_rule(const allocator_type& allocator = allocator_type())
        : sequence_(true), elements_(allocator)
    {
    }

    void add_rule(bool sequence, std::shared_ptr<rule<JsonT>> rule)
    {
        if (elements_.size() > 0)
        {
            sequence_ = sequence;
        }
        elements_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return elements_.get_allocator();
    }

    void base_rule(std::shared_ptr<rule<JsonT>> rule_ptr) override
    {
        if (elements_.size() > 0)
        {
            elements_.back()->base_rule(rule_ptr);
        }
    }

private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
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

        for (size_t i = 0, j = 0; i < elements_.size() && j < val.size(); ++i)
        {
            size_t index = 0;
            do
            {
                result = elements_[i]->validate(val[j], optional,rules, index);
                if (sequence_ && result == status::fail)
                {
                    return result;
                }
                else if (!sequence_ && result == status::pass)
                {
                    return result;
                }
                ++j;
                ++index;
            }
            while ((result == status::may_repeat || result == status::must_repeat) && j < val.size());
        }
        return (result == status::fail || result == status::must_repeat) ? status::fail : status::pass;
    }

    array_rule<JsonT>& operator=(const array_rule<JsonT>&);
};

template <class JsonT>
class group_rule : public rule<JsonT>
{
public:
    typedef typename JsonT::object_allocator allocator_type;
    typedef typename JsonT::string_type string_type;
    typedef rule<JsonT> rule_type;
    typedef std::shared_ptr<rule<JsonT>> rule_ptr_type;
    typedef std::map<string_type,std::shared_ptr<rule_type>> name_rule_map;
private:
    bool sequence_;
    std::vector<rule_ptr_type,allocator_type> elements_;
public:
    group_rule(const allocator_type& allocator = allocator_type())
        : sequence_(true), elements_(allocator)
    {
    }

    void add_rule(bool sequence, std::shared_ptr<rule<JsonT>> rule)
    {
        if (elements_.size() > 0)
        {
            sequence_ = sequence;
        }
        elements_.push_back(rule);
    }

    allocator_type get_allocator() const
    {
        return elements_.get_allocator();
    }
private:

    status do_validate(const JsonT& val, bool optional, const name_rule_map& rules, size_t index) const override
    {
        status result = status::pass;
        if (index < elements_.size())
        {
            result = elements_[index]->validate(val,optional,rules, index);
            if (sequence_ && result == status::fail)
            {
                return result;
            }
            else if (!sequence_ && result == status::pass)
            {
                return result;
            }
        }
        return (index+1) < elements_.size() ? status::may_repeat : status::pass;
    }

    group_rule<JsonT>& operator=(const group_rule<JsonT>&);
};



}}

#endif
