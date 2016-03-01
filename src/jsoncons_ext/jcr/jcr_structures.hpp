// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_STRUCTURES_HPP
#define JSONCONS_JCR_JCR_STRUCTURES_HPP

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
#include "jsoncons/jsoncons.hpp"

namespace jsoncons { namespace jcr {

template <class ValT>
class rule
{
public:
    typedef typename ValT::string_type string_type;
    typedef typename ValT::json_type json_type;

    typedef typename std::vector<std::pair<string_type,ValT>>::iterator iterator;
    typedef std::move_iterator<iterator> move_iterator;

    virtual bool validate(const json_type& val) const = 0;
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

template <class ValT, class Alloc>
class array_rule : public rule<ValT>
{
public:
    typedef Alloc allocator_type;
    typedef typename ValT::json_type json_type;
    typedef ValT value_type;
    typedef typename ValT::json_type::array array;
    typedef typename std::allocator_traits<Alloc>:: template rebind_alloc<ValT> vector_allocator_type;
    typedef typename std::vector<ValT,Alloc>::reference reference;
    typedef typename std::vector<ValT,Alloc>::const_reference const_reference;
    typedef typename std::vector<ValT,Alloc>::iterator iterator;
    typedef typename std::vector<ValT,Alloc>::const_iterator const_iterator;

    array_rule()
        : elements_()
    {
    }

    explicit array_rule(const Alloc& allocator)
        : elements_(allocator)
    {
    }

    explicit array_rule(size_t n, const Alloc& allocator = Alloc())
        : elements_(n,ValT(),allocator)
    {
    }

    explicit array_rule(size_t n, const ValT& value, const Alloc& allocator = Alloc())
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

    array_rule(std::initializer_list<ValT> init, 
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
            *d = s->second;
        }
    }

    rule* clone() const override
    {
        return new array_rule(*this);
    }

    bool validate(const json_type& j) const override
    {
        const array& val = j.array_value(); 
        bool result = false;
        return result;
    }

    Alloc get_allocator() const
    {
        return elements_.get_allocator();
    }

    void swap(array_rule<ValT,Alloc>& val)
    {
        elements_.swap(val.elements_);
    }

    size_t capacity() const {return elements_.capacity();}

    void clear() {elements_.clear();}

    void reserve(size_t n) {elements_.reserve(n);}

    void resize(size_t n) {elements_.resize(n);}

    void resize(size_t n, const ValT& val) {elements_.resize(n,val);}

    void remove_range(size_t from_index, size_t to_index) 
    {
        JSONCONS_ASSERT(from_index <= to_index);
        JSONCONS_ASSERT(to_index <= elements_.size());
        elements_.erase(elements_.begin()+from_index,elements_.begin()+to_index);
    }

    void erase(iterator first, iterator last) 
    {
        elements_.erase(first,last);
    }

    ValT& operator[](size_t i) {return elements_[i];}

    const ValT& operator[](size_t i) const {return elements_[i];}

    void push_back(const ValT& value)
    {
        elements_.push_back(value);
    }

    void push_back(ValT&& value)
    {
        elements_.push_back(std::move(value));
    }

    void add(size_t index, const ValT& value)
    {
        auto position = index < elements_.size() ? elements_.begin() + index : elements_.end();
        elements_.insert(position, value);
    }

    void add(size_t index, ValT&& value)
    {
        auto it = index < elements_.size() ? elements_.begin() + index : elements_.end();
        elements_.insert(it, std::move(value));
    }

    iterator add(const_iterator pos, const ValT& value)
    {
        return elements_.insert(pos, value);
    }

    iterator add(const_iterator pos, ValT&& value)
    {
        return elements_.insert(pos, std::move(value));
    }

    iterator begin() {return elements_.begin();}

    iterator end() {return elements_.end();}

    const_iterator begin() const {return elements_.begin();}

    const_iterator end() const {return elements_.end();}

    bool operator==(const array_rule<ValT,Alloc>& rhs) const
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
    array_rule& operator=(const array_rule<ValT,Alloc>&);
    std::vector<ValT,Alloc> elements_;
};

template <class StringT,class ValT,class Alloc>
class object_rule : public rule<ValT>
{
public:
    typedef typename ValT::json_type json_type;
    typedef typename ValT::json_type::object object;
    typedef Alloc allocator_type;
    typedef typename ValT::char_type char_type;
    typedef StringT string_type;
    typedef name_value_pair<StringT,ValT> value_type;
    typedef typename std::vector<value_type, allocator_type>::iterator base_iterator;
    typedef typename std::vector<value_type, allocator_type>::const_iterator const_base_iterator;

    typedef json_object_iterator<base_iterator,base_iterator> iterator;
    typedef json_object_iterator<const_base_iterator,base_iterator> const_iterator;

    static value_type move_pair(std::pair<string_type,ValT>&& val)
    {
        return value_type(std::move(val.first),std::move(val.second));
    }
private:
    std::vector<value_type,allocator_type> members_;
public:
    object_rule(const allocator_type& allocator = allocator_type())
        : members_(allocator)
    {
    }

    object_rule(const object_rule<StringT,ValT,Alloc>& val)
        : members_(val.members_)
    {
    }

    object_rule(object_rule&& val)
        : members_(std::move(val.members_))
    {
    }

    object_rule(const object_rule<StringT,ValT,Alloc>& val, const allocator_type& allocator) :
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
        insert(first,last,move_pair);
    }

    rule* clone() const override
    {
        return new object_rule(*this);
    }

    Alloc get_allocator() const
    {
        return members_.get_allocator();
    }

    bool validate(const json_type& j) const override
    {
        const object& val = j.object_value(); 

        bool result = false;
        if (val.size() > 0)
        {
            result = true;
            for (auto member : val)
            {
                auto it = find(member.name());
                if (it == end())
                {
                    result = false;
                }
                else
                {
                    result = it->value().validate(member.value());
                }
                if (!result)
                {
                    break;
                }
            }
        }
        return result;
    }

    iterator begin()
    {
        //return members_.begin();
        return iterator(members_.begin());
    }

    iterator end()
    {
        //return members_.end();
        return iterator(members_.end());
    }

    const_iterator begin() const
    {
        //return iterator(members.data());
        return const_iterator(members_.begin());
    }

    const_iterator end() const
    {
        //return members_.end();
        return const_iterator(members_.end());
    }
/*
    const_iterator cbegin() const
    {
        return members_.begin();
    }

    const_iterator cend() const
    {
        return members_.end();
    }
*/
    void swap(object_rule& val)
    {
        members_.swap(val.members_);
    }

    size_t capacity() const {return members_.capacity();}

    void clear() {members_.clear();}

    void reserve(size_t n) {members_.reserve(n);}

    iterator find(const char_type* name)
    {
        return find(name, std::char_traits<char_type>::length(name));
    }

    const_iterator find(const char_type* name) const
    {
        return find(name, std::char_traits<char_type>::length(name));
    }

    iterator find(const char_type* name, size_t length)
    {
        member_lt_string<value_type,char_type> comp(length);
        auto it = std::lower_bound(members_.begin(),members_.end(), name, comp);
        auto result = (it != members_.end() && name_eq_string(it->name(),name,length)) ? it : members_.end();
        return iterator(result);
    }

    const_iterator find(const char_type* name, size_t length) const
    {
        member_lt_string<value_type,char_type> comp(length);
        auto it = std::lower_bound(members_.begin(),members_.end(), name, comp);
        auto result = (it != members_.end() && name_eq_string(it->name(),name,length)) ? it : members_.end();
        return const_iterator(result);
    }

    iterator find(const string_type& name)
    {
        return find(name.data(), name.length());
    }
 
    const_iterator find(const string_type& name) const
    {
        return const_iterator(find(name.data(), name.length()));
    }

    ValT& at(const string_type& name) 
    {
        auto it = find(name);
        if (it == members_.end())
        {
            JSONCONS_THROW_EXCEPTION_1(std::out_of_range,"Member %s not found.",name);
        }
        return it->value();
    }

    const ValT& at(const string_type& name) const
    {
        auto it = find(name);
        if (it == members_.end())
        {
            JSONCONS_THROW_EXCEPTION_1(std::out_of_range,"Member %s not found.",name);
        }
        return it->value();
    }

    void erase(iterator first, iterator last) 
    {
        members_.erase(first.get(),last.get());
    }

    void erase(const char_type* name) 
    {
        erase(name, std::char_traits<char_type>::length(name));
    }

    void erase(const char_type* name, size_t length) 
    {
        member_lt_string<value_type,char_type> comp(length);
        auto it = std::lower_bound(members_.begin(),members_.end(), name, comp);
        if (it != members_.end() && name_eq_string(it->name(),name,length))
        {
            members_.erase(it);
        }
    }

    void erase(const string_type& name) 
    {
        return erase(name.data(),name.length());
    }

    template<class InputIt, class UnaryPredicate>
    void insert(InputIt first, InputIt last, UnaryPredicate pred)
    {
        size_t count = std::distance(first,last);
        size_t pos = members_.size();
        members_.resize(pos+count);
        auto d = members_.begin()+pos;
        for (auto s = first; s != last; ++s, ++d)
        {
            *d = pred(*s);
        }
        std::sort(members_.begin(),members_.end(),member_lt_member<value_type>());
    }

    void set(const char_type* s, size_t length, const ValT& value)
    {
        auto it = std::lower_bound(members_.begin(),members_.end(),s,member_lt_string<value_type,char_type>(length));
        if (it == members_.end())
        {
            members_.push_back(value_type(string_type(s,length),value));
        }
        else if (name_eq_string(it->name(),s,length))
        {
            it->value(value);
        }
        else
        {
            members_.insert(it,value_type(string_type(s,length),value));
        }
    }

    void set(const char_type* s, size_t length, ValT&& value)
    {
        auto it = std::lower_bound(members_.begin(),members_.end(),s,member_lt_string<value_type,char_type>(length));
        if (it == members_.end())
        {
            members_.push_back(value_type(string_type(s,length),std::move(value)));
        }
        else if (name_eq_string(it->name(),s,length))
        {
            it->value(std::move(value));
        }
        else
        {
            members_.insert(it,value_type(string_type(s,length),std::move(value)));
        }
    }

    void set(string_type&& name, const ValT& value)
    {
        auto it = std::lower_bound(members_.begin(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        if (it == members_.end())
        {
            members_.push_back(value_type(std::move(name), value));
        }
        else if (it->name() == name)
        {
            it->value(value);
        }
        else
        {
            members_.insert(it,value_type(std::move(name),value));
        }
    }

    void set(const string_type& name, const ValT& value)
    {
        set(name.data(),name.length(),value);
    }

    void set(const string_type& name, ValT&& value)
    {
        set(name.data(),name.length(),std::move(value));
    }

    void set(string_type&& name, ValT&& value)
    {
        auto it = std::lower_bound(members_.begin(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        if (it == members_.end())
        {
            members_.push_back(value_type(std::move(name), std::move(value)));
        }
        else if (it->name() == name)
        {
            it->value(std::move(value));
        }
        else
        {
            members_.insert(it,value_type(std::move(name),std::move(value)));
        }
    }

    iterator set(iterator hint, const char_type* name, const ValT& value)
    {
        return set(hint, name, std::char_traits<char_type>::length(name), value);
    }

    iterator set(iterator hint, const char_type* name, ValT&& value)
    {
        return set(hint, name, std::char_traits<char_type>::length(name), std::move(value));
    }

    iterator set(iterator hint, const char_type* s, size_t length, const ValT& value)
    {
        base_iterator it;
        if (hint.get() != members_.end() && name_le_string(hint.get()->name(), s, length))
        {
            it = std::lower_bound(hint.get(),members_.end(),s,member_lt_string<value_type,char_type>(length));
        }
        else
        {
            it = std::lower_bound(members_.begin(),members_.end(),s, member_lt_string<value_type,char_type>(length));
        }

        if (it == members_.end())
        {
            members_.push_back(value_type(string_type(s, length), value));
            it = members_.end();
        }
        else if (name_eq_string(it->name(),s,length))
        {
            it->value(value);
        }
        else
        {
           it = members_.insert(it,value_type(string_type(s,length),value));
        }
        return iterator(it);
    }

    iterator set(iterator hint, const char_type* s, size_t length, ValT&& value)
    {
        base_iterator it;
        if (hint.get() != members_.end() && name_le_string(hint.get()->name(), s, length))
        {
            it = std::lower_bound(hint.get(),members_.end(),s,member_lt_string<value_type,char_type>(length));
        }
        else
        {
            it = std::lower_bound(members_.begin(),members_.end(),s, member_lt_string<value_type,char_type>(length));
        }

        if (it == members_.end())
        {
            members_.push_back(value_type(string_type(s, length), std::move(value)));
            it = members_.end();
        }
        else if (name_eq_string(it->name(),s,length))
        {
            it->value(std::move(value));
        }
        else
        {
           it = members_.insert(it,value_type(string_type(s,length),std::move(value)));
        }
        return iterator(it);
    }

    iterator set(iterator hint, const string_type& name, const ValT& value)
    {
        return set(hint,name.data(),name.length(),value);
    }

    iterator set(iterator hint, string_type&& name, const ValT& value)
    {
        base_iterator it;
        if (hint.get() != members_.end() && hint.get()->name() <= name)
        {
            it = std::lower_bound(hint.get(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        }
        else
        {
            it = std::lower_bound(members_.begin(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        }

        if (it == members_.end())
        {
            members_.push_back(value_type(std::move(name), value));
            it = members_.end();
        }
        else if (it->name() == name)
        {
            it->value(value);
        }
        else
        {
            it = members_.insert(it,value_type(std::move(name),value));
        }
        return iterator(it);
    }

    iterator set(iterator hint, const string_type& name, ValT&& value)
    {
        return set(hint,name.data(),name.length(),std::move(value));
    }

    iterator set(iterator hint, string_type&& name, ValT&& value)
    {
        typename std::vector<value_type,allocator_type>::iterator it;
        if (hint.get() != members_.end() && hint.get()->name() <= name)
        {
            it = std::lower_bound(hint.get(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        }
        else
        {
            it = std::lower_bound(members_.begin(),members_.end(),name.data() ,member_lt_string<value_type,char_type>(name.length()));
        }

        if (it == members_.end())
        {
            members_.push_back(value_type(std::move(name), std::move(value)));
            it = members_.end();
        }
        else if (it->name() == name)
        {
            it->value(std::move(value));
        }
        else
        {
            it = members_.insert(it,value_type(std::move(name),std::move(value)));
        }
        return iterator(it);
    }

    bool operator==(const object_rule<StringT,ValT,Alloc>& rhs) const
    {
        if (size() != rhs.size())
        {
            return false;
        }
        for (auto it = members_.begin(); it != members_.end(); ++it)
        {

            auto rhs_it = std::lower_bound(rhs.members_.begin(), rhs.members_.end(), *it, member_lt_member<value_type>());
            // member_lt_member actually only compares keys, so we need to check the value separately
            if (rhs_it == rhs.members_.end() || rhs_it->name() != it->name() || rhs_it->value() != it->value())
            {
                return false;
            }
        }
        return true;
    }
private:
    object_rule<StringT,ValT,Alloc>& operator=(const object_rule<StringT,ValT,Alloc>&);
};



}}

#endif
