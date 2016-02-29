// Copyright 201r3 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JCR_HPP
#define JSONCONS_JCR_JCR_HPP

#include <limits>
#include <string>
#include <vector>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <memory>
#include <typeinfo>
#include "jsoncons/json.hpp"
#include "jcr_deserializer.hpp"
#include "jcr_parser.hpp"
#include "jcr_structures.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif

namespace jsoncons { namespace jcr {

template <typename CharT,class T> inline
void serialize(basic_json_output_handler<CharT>& os, const T&)
{
    os.value(null_type());
}

template <typename JsonT>
class basic_jcr_validator;

enum class value_types : uint8_t 
{
    // Simple types
    empty_object_t,
    double_t,
    integer_t,
    uinteger_t,
    bool_t,
    null_t,
    // Non simple types
    string_t,
    object_t,
    array_t,
    rule_t
};

inline
bool is_simple(value_types type)
{
    return type < value_types::string_t;
}

template <class JsonT>
class basic_jcr_validator
{
public:

    class rule
    {
    public:
        virtual bool validate(const JsonT& val) const = 0;
        virtual rule* clone() const = 0;
        virtual ~rule()
        {
        }
    };

    class any_integer_rule : public rule
    {
    public:
        any_integer_rule()
        {
        }

        rule* clone() const override
        {
            return new any_integer_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() || val.as_uinteger();
        }
    };

    class any_string_rule : public rule
    {
    public:
        any_string_rule()
        {
        }

        rule* clone() const override
        {
            return new any_string_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_string();
        }
    };

    class integer_range_rule : public rule
    {
        int64_t from_;
        int64_t to_;

    public:
        integer_range_rule(int64_t from, int64_t to)
            : from_(from), to_(to)
        {
        }

        rule* clone() const override
        {
            return new integer_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() && val.as_integer() >= from_ && val.as_integer() <= to_;
        }
    };

    class uinteger_range_rule : public rule
    {
        uint64_t from_;
        uint64_t to_;
    public:
        uinteger_range_rule(uint64_t from, uint64_t to)
            : from_(from), to_(to)
        {
        }

        rule* clone() const override
        {
            return new uinteger_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_uinteger() && val.as_uinteger() >= from_ && val.as_uinteger() <= to_;
        }
    };

    typedef JsonT json_type;

    typedef typename JsonT::allocator_type allocator_type;

    typedef typename JsonT::char_type char_type;
    typedef typename JsonT::char_traits_type char_traits_type;

    typedef typename JsonT::string_allocator string_allocator;
    typedef typename JsonT::string_type string_type;
    typedef basic_jcr_validator<JsonT> value_type;
    typedef name_value_pair<string_type,value_type> member_type;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<JsonT> array_allocator;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<member_type> object_allocator;

    typedef jcr_array_validator<value_type,array_allocator> array;
    typedef jcr_object_validator<string_type,value_type,object_allocator>  object;

    typedef jsoncons::null_type null_type;

    typedef typename object::iterator object_iterator;
    typedef typename object::const_iterator const_object_iterator;
    typedef typename array::iterator array_iterator;
    typedef typename array::const_iterator const_array_iterator;

    template <typename IteratorT>
    class range 
    {
        IteratorT first_;
        IteratorT last_;
    public:
        range(const IteratorT& first, const IteratorT& last)
            : first_(first), last_(last)
        {
        }

    public:
        friend class basic_jcr_validator<JsonT>;

        IteratorT begin()
        {
            return first_;
        }
        IteratorT end()
        {
            return last_;
        }
    };

    typedef range<object_iterator> object_range;
    typedef range<const_object_iterator> const_object_range;
    typedef range<array_iterator> array_range;
    typedef range<const_array_iterator> const_array_range;

    struct variant
    {
        variant()
            : type_(value_types::empty_object_t)
        {
        }

        variant(const allocator_type& a)
            : type_(value_types::object_t)
        {
            value_.object_val_ = create_impl<object>(a, object_allocator(a));
        }

        variant(std::initializer_list<value_type> init,
                const allocator_type& a)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(a, std::move(init), array_allocator(a));
        }

        explicit variant(variant&& var)
            : type_(value_types::null_t)
        {
            swap(var);
        }
        
        explicit variant(variant&& var, const allocator_type& a)
            : type_(value_types::null_t)
        {
            swap(var);
        }

        explicit variant(const variant& var)
        {
            init_variant(var);
        }
        explicit variant(const variant& var, const allocator_type& a)
            : type_(var.type_)
        {
            init_variant(var);
        }

        variant(const object & val)
            : type_(value_types::object_t)
        {
            value_.object_val_ = create_impl<object>(val.get_allocator(), val) ;
        }

        variant(const object & val, const allocator_type& a)
            : type_(value_types::object_t)
        {
            value_.object_val_ = create_impl<object>(a, val, object_allocator(a)) ;
        }

        variant(object&& val)
            : type_(value_types::object_t)
        {
            value_.object_val_ = create_impl<object>(val.get_allocator(), std::move(val));
        }

        variant(object&& val, const allocator_type& a)
            : type_(value_types::object_t)
        {
            value_.object_val_ = create_impl<object>(a, std::move(val), object_allocator(a));
        }

        variant(const array& val)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(val.get_allocator(), val);
        }

        variant(const array& val, const allocator_type& a)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(a, val, array_allocator(a));
        }

        variant(array&& val)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(val.get_allocator(), std::move(val));
        }

        variant(array&& val, const allocator_type& a)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(a, std::move(val), array_allocator(a));
        }

        explicit variant(null_type)
            : type_(value_types::null_t)
        {
        }

        explicit variant(rule* rule)
            : type_(value_types::rule_t)
        {
            value_.rule_val_ = rule;
        }

        explicit variant(bool val)
            : type_(value_types::bool_t)
        {
            value_.bool_val_ = val;
        }

        explicit variant(double val, uint8_t precision)
            : type_(value_types::double_t), length_or_precision_(precision)
        {
            value_.double_val_ = val;
        }

        explicit variant(int64_t val)
            : type_(value_types::integer_t)
        {
            value_.integer_val_ = val;
        }

        explicit variant(uint64_t val)
            : type_(value_types::uinteger_t)
        {
            value_.uinteger_val_ = val;
        }

        explicit variant(const string_type& s, const allocator_type& a)
        {
            type_ = value_types::string_t;
            value_.string_val_ = create_impl<string_type>(a, s, string_allocator(a));
            //value_.string_val_ = create_string_data(s.data(), s.length(), string_allocator(a));
        }

        explicit variant(const char_type* s, const allocator_type& a)
        {
            size_t length = std::char_traits<char_type>::length(s);
            type_ = value_types::string_t;
            value_.string_val_ = create_impl<string_type>(a, s, string_allocator(a));
            //value_.string_val_ = create_string_data(s, length, string_allocator(a));
        }

        explicit variant(const char_type* s, size_t length, const allocator_type& a)
        {
            type_ = value_types::string_t;
            value_.string_val_ = create_impl<string_type>(a, s, length, string_allocator(a));
            //value_.string_val_ = create_string_data(s, length, string_allocator(a));
        }

        template<class InputIterator>
        variant(InputIterator first, InputIterator last, const allocator_type& a)
            : type_(value_types::array_t)
        {
            value_.array_val_ = create_impl<array>(a, first, last, array_allocator(a));
        }

        void init_variant(const variant& var)
        {
            type_ = var.type_;
            switch (type_)
            {
            case value_types::null_t:
            case value_types::empty_object_t:
                break;
            case value_types::double_t:
                length_or_precision_ = 0;
                value_.double_val_ = var.value_.double_val_;
                break;
            case value_types::integer_t:
                value_.integer_val_ = var.value_.integer_val_;
                break;
            case value_types::uinteger_t:
                value_.uinteger_val_ = var.value_.uinteger_val_;
                break;
            case value_types::bool_t:
                value_.bool_val_ = var.value_.bool_val_;
                break;
            case value_types::string_t:
                value_.string_val_ = create_impl<string_type>(var.value_.string_val_->get_allocator(), *(var.value_.string_val_), string_allocator(var.value_.string_val_->get_allocator()));
                break;
            case value_types::array_t:
                value_.array_val_ = create_impl<array>(var.value_.array_val_->get_allocator(), *(var.value_.array_val_), array_allocator(var.value_.array_val_->get_allocator()));
                break;
            case value_types::object_t:
                value_.object_val_ = create_impl<object>(var.value_.object_val_->get_allocator(), *(var.value_.object_val_), object_allocator(var.value_.object_val_->get_allocator()));
                break;
            case value_types::rule_t:
                value_.rule_val_ = var.value_.rule_val_->clone();
                break;
            default:
                break;
            }
        }

        ~variant()
        {
            destroy_variant();
        }

        void destroy_variant()
        {
            switch (type_)
            {
            case value_types::string_t:
                destroy_impl(value_.string_val_->get_allocator(), value_.string_val_);
                break;
            case value_types::array_t:
                destroy_impl(value_.array_val_->get_allocator(), value_.array_val_);
                break;
            case value_types::object_t:
                destroy_impl(value_.object_val_->get_allocator(), value_.object_val_);
                break;
            case value_types::rule_t:
                delete value_.rule_val_;
                break;
            default:
                break; 
            }
        }

        variant& operator=(const variant& val)
        {
            if (this != &val)
            {
                if (is_simple(type_))
                {
                    if (is_simple(val.type_))
                    {
                        type_ = val.type_;
                        length_or_precision_ = val.length_or_precision_;
                        value_ = val.value_;
                    }
                    else
                    {
                        init_variant(val);
                    }
                }
                else
                {
                    destroy_variant();
                    init_variant(val);
                }
            }
            return *this;
        }

        variant& operator=(variant&& val)
        {
            if (this != &val)
            {
                val.swap(*this);
            }
            return *this;
        }

        void assign(const object & val)
        {
            destroy_variant();
            type_ = value_types::object_t;
            value_.object_val_ = create_impl<object>(val.get_allocator(), val, object_allocator(val.get_allocator()));
        }

        void assign(object && val)
        {
            switch (type_)
            {
            case value_types::object_t:
                value_.object_val_->swap(val);
                break;
            default:
                destroy_variant();
                type_ = value_types::object_t;
                value_.object_val_ = create_impl<object>(val.get_allocator(), std::move(val), object_allocator(val.get_allocator()));
                break;
            }
        }

        void assign(const array& val)
        {
            destroy_variant();
            type_ = value_types::array_t;
            value_.array_val_ = create_impl<array>(val.get_allocator(), val, array_allocator(val.get_allocator())) ;
        }

        void assign(array&& val)
        {
            switch (type_)
            {
            case value_types::array_t:
                value_.array_val_->swap(val);
                break;
            default:
                destroy_variant();
                type_ = value_types::array_t;
                value_.array_val_ = create_impl<array>(val.get_allocator(), std::move(val), array_allocator(val.get_allocator()));
                break;
            }
        }

        void assign(const string_type& s)
        {
            destroy_variant();
            type_ = value_types::string_t;
            value_.string_val_ = create_impl<string_type>(s.get_allocator(), s, string_allocator(s.get_allocator()));
            //value_.string_val_ = create_string_data(s.data(), s.length(), string_allocator(s.get_allocator()));
        }

        void assign_string(const char_type* s, size_t length, const allocator_type& allocator = allocator_type())
        {
            destroy_variant();
            type_ = value_types::string_t;
            value_.string_val_ = create_impl<string_type>(allocator, s, length, string_allocator(allocator));
            //value_.string_val_ = create_string_data(s, length, string_allocator(allocator));
        }

        void assign(int64_t val)
        {
            destroy_variant();
            type_ = value_types::integer_t;
            value_.integer_val_ = val;
        }

        void assign(uint64_t val)
        {
            destroy_variant();
            type_ = value_types::uinteger_t;
            value_.uinteger_val_ = val;
        }

        void assign(double val, uint8_t precision = 0)
        {
            destroy_variant();
            type_ = value_types::double_t;
            length_or_precision_ = precision;
            value_.double_val_ = val;
        }

        void assign(bool val)
        {
            destroy_variant();
            type_ = value_types::bool_t;
            value_.bool_val_ = val;
        }

        void assign(null_type)
        {
            destroy_variant();
            type_ = value_types::null_t;
        }

        bool operator!=(const variant& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator==(const variant& rhs) const
        {
            if (is_number() & rhs.is_number())
            {
                switch (type_)
                {
                case value_types::integer_t:
                    switch (rhs.type_)
                    {
                    case value_types::integer_t:
                        return value_.integer_val_ == rhs.value_.integer_val_;
                    case value_types::uinteger_t:
                        return value_.integer_val_ == rhs.value_.uinteger_val_;
                    case value_types::double_t:
                        return value_.integer_val_ == rhs.value_.double_val_;
                    default:
                        break;
                    }
                    break;
                case value_types::uinteger_t:
                    switch (rhs.type_)
                    {
                    case value_types::integer_t:
                        return value_.uinteger_val_ == rhs.value_.integer_val_;
                    case value_types::uinteger_t:
                        return value_.uinteger_val_ == rhs.value_.uinteger_val_;
                    case value_types::double_t:
                        return value_.uinteger_val_ == rhs.value_.double_val_;
                    default:
                        break;
                    }
                    break;
                case value_types::double_t:
                    switch (rhs.type_)
                    {
                    case value_types::integer_t:
                        return value_.double_val_ == rhs.value_.integer_val_;
                    case value_types::uinteger_t:
                        return value_.double_val_ == rhs.value_.uinteger_val_;
                    case value_types::double_t:
                        return value_.double_val_ == rhs.value_.double_val_;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
            }

            if (rhs.type_ != type_)
            {
                return false;
            }
            switch (type_)
            {
            case value_types::bool_t:
                return value_.bool_val_ == rhs.value_.bool_val_;
            case value_types::null_t:
            case value_types::empty_object_t:
                return true;
            case value_types::string_t:
                return *(value_.string_val_) == *(rhs.value_.string_val_);
            case value_types::array_t:
                return *(value_.array_val_) == *(rhs.value_.array_val_);
                break;
            case value_types::object_t:
                return *(value_.object_val_) == *(rhs.value_.object_val_);
                break;
            default:
                // throw
                break;
            }
            return false;
        }

        bool validate(const JsonT& val) const
        {
            if (is_number() & val.is_number())
            {
                switch (type_)
                {
                case value_types::integer_t:
                    return value_.integer_val_ == val.as_integer();
                case value_types::uinteger_t:
                    return value_.uinteger_val_ == val.as_uinteger();
                case value_types::double_t:
                    return value_.double_val_ == val.as_double();
                default:
                    break;
                }
            }

            switch (type_)
            {
            case value_types::rule_t:
                return value_.rule_val_->validate(val);
            case value_types::bool_t:
                return value_.bool_val_ == val.as_bool();
            case value_types::null_t:
                return val.is_null();
            case value_types::empty_object_t:
                return val.is_object() && val.size() == 0;
            case value_types::string_t:
                return *(value_.string_val_) == val.as_string();
            case value_types::array_t:
                return value_.array_val_->validate(val.array_value());
                break;
            case value_types::object_t:
                return value_.object_val_->validate(val.object_value());
                break;
            default:
                // throw
                break;
            }
            return false;
        }

        bool is_null() const JSONCONS_NOEXCEPT
        {
            return type_ == value_types::null_t;
        }

        bool is_bool() const JSONCONS_NOEXCEPT
        {
            return type_ == value_types::bool_t;
        }

        bool empty() const JSONCONS_NOEXCEPT
        {
            switch (type_)
            {
            case value_types::string_t:
                return value_.string_val_->length() == 0;
            case value_types::array_t:
                return value_.array_val_->size() == 0;
            case value_types::empty_object_t:
                return true;
            case value_types::object_t:
                return value_.object_val_->size() == 0;
            default:
                return false;
            }
        }

        bool is_string() const JSONCONS_NOEXCEPT
        {
            return (type_ == value_types::string_t);
        }

        bool is_number() const JSONCONS_NOEXCEPT
        {
            return type_ == value_types::double_t || type_ == value_types::integer_t || type_ == value_types::uinteger_t;
        }

        void swap(variant& rhs)
        {
            using std::swap;
            if (this == &rhs)
            {
                // same object, do nothing
            }
            else
            {
                swap(type_, rhs.type_);
                swap(length_or_precision_, rhs.length_or_precision_);
                swap(value_, rhs.value_);
            }
        }

        value_types type_;
        uint8_t length_or_precision_;
        union
        {
            double double_val_;
            int64_t integer_val_;
            uint64_t uinteger_val_;
            bool bool_val_;
            object* object_val_;
            array* array_val_;
            string_type* string_val_;
            rule* rule_val_;
        } value_;
    };

    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is);
    static basic_jcr_validator parse_stream(std::basic_istream<char_type>& is, basic_parse_error_handler<char_type>& err_handler);

    static basic_jcr_validator parse(const string_type& s)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<char_type> parser(handler);
        parser.begin_parse();
        parser.parse(s.data(),0,s.length());
        parser.end_parse();
        parser.check_done(s.data(),parser.index(),s.length());
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json string");
        }
        return handler.get_result();
    }

    static basic_jcr_validator parse(const string_type& s, basic_parse_error_handler<char_type>& err_handler)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_jcr_parser<char_type> parser(handler,err_handler);
        parser.begin_parse();
        parser.parse(s.data(),0,s.length());
        parser.end_parse();
        parser.check_done(s.data(),parser.index(),s.length());
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json string");
        }
        return handler.get_result();
    }

    static basic_jcr_validator parse_file(const std::string& s);

    static basic_jcr_validator parse_file(const std::string& s, basic_parse_error_handler<char_type>& err_handler);

    static basic_jcr_validator make_array()
    {
        return basic_jcr_validator::array();
    }

    static basic_jcr_validator make_array(size_t n, const array_allocator& allocator = array_allocator())
    {
        return basic_jcr_validator::array(n,allocator);
    }

    template <class T>
    static basic_jcr_validator make_array(size_t n, const T& val, const array_allocator& allocator = array_allocator())
    {
        return basic_jcr_validator::array(n, val,allocator);
    }

    template <size_t dim>
    static typename std::enable_if<dim==1,basic_jcr_validator>::type make_array(size_t n)
    {
        return array(n);
    }

    template <size_t dim, class T>
    static typename std::enable_if<dim==1,basic_jcr_validator>::type make_array(size_t n, const T& val, const allocator_type& allocator = allocator_type())
    {
        return array(n,val,allocator);
    }

    template <size_t dim, typename... Args>
    static typename std::enable_if<(dim>1),basic_jcr_validator>::type make_array(size_t n, Args... args)
    {
        const size_t dim1 = dim - 1;

        basic_jcr_validator val = make_array<dim1>(args...);
        val.resize(n);
        for (size_t i = 0; i < n; ++i)
        {
            val[i] = make_array<dim1>(args...);
        }
        return val;
    }

    variant var_;

    basic_jcr_validator() 
        : var_()
    {
    }

    basic_jcr_validator(const allocator_type& allocator) 
        : var_(allocator)
    {
    }

    basic_jcr_validator(std::initializer_list<value_type> init,
               const allocator_type& allocator = allocator_type()) 
        : var_(std::move(init), allocator)
    {
    }

    basic_jcr_validator(const basic_jcr_validator<JsonT>& val)
        : var_(val.var_)
    {
    }

    basic_jcr_validator(const basic_jcr_validator<JsonT>& val, const allocator_type& allocator)
        : var_(val.var_,allocator)
    {
    }

    basic_jcr_validator(JsonT&& other)
        : var_(std::move(other.var_))
    {
    }

    basic_jcr_validator(JsonT&& other, const allocator_type& allocator)
        : var_(std::move(other.var_),allocator)
    {
    }

    basic_jcr_validator(const array& val)
        : var_(val)
    {
    }

    basic_jcr_validator(array&& other)
        : var_(std::move(other))
    {
    }

    basic_jcr_validator(const object& other)
        : var_(other)
    {
    }

    basic_jcr_validator(object&& other)
        : var_(std::move(other))
    {
    }

    basic_jcr_validator(double val, uint8_t precision)
        : var_(val,precision)
    {
    }

    basic_jcr_validator(rule* rule)
        : var_(rule)
    {
    }

    template <typename T>
    basic_jcr_validator(T val, const allocator_type& allocator)
        : var_(allocator)
    {
        json_type_traits<value_type,T>::assign(*this,val);
    }

    basic_jcr_validator(const char_type *s, size_t length, const allocator_type& allocator = allocator_type())
        : var_(s, length, allocator)
    {
    }
    template<class InputIterator>
    basic_jcr_validator(InputIterator first, InputIterator last, const allocator_type& allocator = allocator_type())
        : var_(first,last,allocator)
    {
    }

    ~basic_jcr_validator()
    {
    }

    basic_jcr_validator& operator=(const JsonT& rhs)
    {
        var_ = rhs.var_;
        return *this;
    }

    basic_jcr_validator& operator=(JsonT&& rhs)
    {
        if (this != &rhs)
        {
            var_ = std::move(rhs.var_);
        }
        return *this;
    }

    template <class T>
    basic_jcr_validator<JsonT>& operator=(T val)
    {
        json_type_traits<value_type,T>::assign(*this,val);
        return *this;
    }

    bool operator!=(const basic_jcr_validator& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator==(const basic_jcr_validator& rhs) const
    {
        return var_ == rhs.var_;
    }

    size_t size() const JSONCONS_NOEXCEPT
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return 0;
        case value_types::object_t:
            return var_.value_.object_val_->size();
        case value_types::array_t:
            return var_.value_.array_val_->size();
        default:
            return 0;
        }
    }

    string_type to_string(const string_allocator& allocator=string_allocator()) const JSONCONS_NOEXCEPT
    {
        string_type s(allocator);
        std::basic_ostringstream<char_type,char_traits_type,string_allocator> os(s);
        {
            basic_json_serializer<char_type> serializer(os);
            to_stream(serializer);
        }
        return os.str();
    }

    string_type to_string(const basic_output_format<char_type>& format,
                          const string_allocator& allocator=string_allocator()) const
    {
        string_type s(allocator);
        std::basic_ostringstream<char_type> os(s);
        {
            basic_json_serializer<char_type> serializer(os, format);
            to_stream(serializer);
        }
        return os.str();
    }

    void to_stream(basic_json_output_handler<char_type>& handler) const
    {
        switch (var_.type_)
        {
        case value_types::string_t:
            handler.value(var_.value_.string_val_->data(),var_.value_.string_val_->length());
            break;
        case value_types::double_t:
            handler.value(var_.value_.double_val_, var_.length_or_precision_);
            break;
        case value_types::integer_t:
            handler.value(var_.value_.integer_val_);
            break;
        case value_types::uinteger_t:
            handler.value(var_.value_.uinteger_val_);
            break;
        case value_types::bool_t:
            handler.value(var_.value_.bool_val_);
            break;
        case value_types::null_t:
            handler.value(null_type());
            break;
        case value_types::empty_object_t:
            handler.begin_object();
            handler.end_object();
            break;
        case value_types::object_t:
            {
                handler.begin_object();
                object* o = var_.value_.object_val_;
                for (const_object_iterator it = o->begin(); it != o->end(); ++it)
                {
                    handler.name((it->name()).data(),it->name().length());
                    it->value().to_stream(handler);
                }
                handler.end_object();
            }
            break;
        case value_types::array_t:
            {
                handler.begin_array();
                array *o = var_.value_.array_val_;
                for (const_array_iterator it = o->begin(); it != o->end(); ++it)
                {
                    it->to_stream(handler);
                }
                handler.end_array();
            }
            break;
        default:
            break;
        }
    }

    void to_stream(std::basic_ostream<char_type>& os) const
    {
        basic_json_serializer<char_type> serializer(os);
        to_stream(serializer);
    }

    void to_stream(std::basic_ostream<char_type>& os, const basic_output_format<char_type>& format) const
    {
        basic_json_serializer<char_type> serializer(os, format);
        to_stream(serializer);
    }

    void to_stream(std::basic_ostream<char_type>& os, const basic_output_format<char_type>& format, bool indenting) const
    {
        basic_json_serializer<char_type> serializer(os, format, indenting);
        to_stream(serializer);
    }

    bool is_null() const JSONCONS_NOEXCEPT
    {
        return var_.is_null();
    }

    size_t count(const string_type& name) const
    {
        switch (var_.type_)
        {
        case value_types::object_t:
            {
                auto it = var_.value_.object_val_->find(name);
                if (it == members().end())
                {
                    return 0;
                }
                size_t count = 0;
                while (it != members().end() && it->name() == name)
                {
                    ++count;
                    ++it;
                }
                return count;
            }
            break;
        default:
            return 0;
        }
    }

    template<typename T>
    bool is() const
    {
        return json_type_traits<value_type,T>::is(*this);
    }

    bool is_string() const JSONCONS_NOEXCEPT
    {
        return var_.is_string();
    }


    bool is_bool() const JSONCONS_NOEXCEPT
    {
        return var_.is_bool();
    }

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::object_t || var_.type_ == value_types::empty_object_t;
    }

    bool is_array() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::array_t;
    }

    bool is_integer() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::integer_t || (var_.type_ == value_types::uinteger_t && (as_uinteger() <= static_cast<unsigned long long>(std::numeric_limits<long long>::max JSONCONS_NO_MACRO_EXP())));
    }

    bool is_uinteger() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::uinteger_t || (var_.type_ == value_types::integer_t && as_integer() >= 0);
    }

    bool is_double() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::double_t;
    }

    bool is_number() const JSONCONS_NOEXCEPT
    {
        return var_.is_number();
    }

    bool empty() const JSONCONS_NOEXCEPT
    {
        return var_.empty();
    }

    size_t capacity() const
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return var_.value_.array_val_->capacity();
        case value_types::object_t:
            return var_.value_.object_val_->capacity();
        default:
            return 0;
        }
    }

    template<class U=allocator_type,
         typename std::enable_if<std::is_default_constructible<U>::value
            >::type* = nullptr>
    void create_object_implicitly()
    {
        var_.type_ = value_types::object_t;
        var_.value_.object_val_ = create_impl<object>(allocator_type(),object_allocator(allocator_type()));
    }

    template<class U=allocator_type,
         typename std::enable_if<!std::is_default_constructible<U>::value
            >::type* = nullptr>
    void create_object_implicitly() const
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Cannot create_impl object implicitly - allocator is not default constructible.");
    }

    void reserve(size_t n)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->reserve(n);
            break;
        case value_types::empty_object_t:
        {
            create_object_implicitly();
            var_.value_.object_val_->reserve(n);
        }
        break;
        case value_types::object_t:
        {
            var_.value_.object_val_->reserve(n);
        }
            break;
        default:
            break;
        }
    }

    void resize(size_t n)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->resize(n);
            break;
        default:
            break;
        }
    }

    template <typename T>
    void resize(size_t n, T val)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->resize(n, val);
            break;
        default:
            break;
        }
    }

    basic_jcr_validator<JsonT>& at(const string_type& name)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            JSONCONS_THROW_EXCEPTION_1(std::out_of_range,"%s not found", name);
        case value_types::object_t:
            {
                auto it = var_.value_.object_val_->find(name);
                if (it == members().end())
                {
                    JSONCONS_THROW_EXCEPTION_1(std::out_of_range, "%s not found", name);
                }
                return it->value();
            }
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    basic_jcr_validator<JsonT>& evaluate() 
    {
        return *this;
    }

    basic_jcr_validator<JsonT>& evaluate_with_default() 
    {
        return *this;
    }

    const basic_jcr_validator<JsonT>& evaluate() const
    {
        return *this;
    }

    basic_jcr_validator<JsonT>& evaluate(size_t i) 
    {
        return at(i);
    }

    const basic_jcr_validator<JsonT>& evaluate(size_t i) const
    {
        return at(i);
    }

    basic_jcr_validator<JsonT>& evaluate(const string_type& name) 
    {
        return at(name);
    }

    const basic_jcr_validator<JsonT>& evaluate(const string_type& name) const
    {
        return at(name);
    }

    const basic_jcr_validator<JsonT>& at(const string_type& name) const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            JSONCONS_THROW_EXCEPTION_1(std::out_of_range,"%s not found", name);
        case value_types::object_t:
            {
                auto it = var_.value_.object_val_->find(name);
                if (it == members().end())
                {
                    JSONCONS_THROW_EXCEPTION_1(std::out_of_range, "%s not found", name);
                }
                return it->value();
            }
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    basic_jcr_validator<JsonT>& at(size_t i)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            if (i >= var_.value_.array_val_->size())
            {
                JSONCONS_THROW_EXCEPTION(std::out_of_range,"Invalid array subscript");
            }
            return var_.value_.array_val_->operator[](i);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Index on non-array value not supported");
        }
    }

    const basic_jcr_validator<JsonT>& at(size_t i) const
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            if (i >= var_.value_.array_val_->size())
            {
                JSONCONS_THROW_EXCEPTION(std::out_of_range,"Invalid array subscript");
            }
            return var_.value_.array_val_->operator[](i);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Index on non-array value not supported");
        }
    }

    object_iterator find(const string_type& name)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return members().end();
        case value_types::object_t:
            return var_.value_.object_val_->find(name);
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    const_object_iterator find(const string_type& name) const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return members().end();
        case value_types::object_t:
            return var_.value_.object_val_->find(name);
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    object_iterator find(const char_type* name)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return members().end();
        case value_types::object_t:
            return var_.value_.object_val_->find(name);
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    const_object_iterator find(const char_type* name) const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return members().end();
        case value_types::object_t:
            return var_.value_.object_val_->find(name);
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    template<typename T>
    basic_jcr_validator<JsonT> get(const string_type& name, T&& default_val) const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            {
                return JsonT(std::forward<T>(default_val));
            }
        case value_types::object_t:
            {
                const_object_iterator it = var_.value_.object_val_->find(name);
                if (it != members().end())
                {
                    return it->value();
                }
                else
                {
                    return JsonT(std::forward<T>(default_val));
                }
            }
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to get %s from a value that is not an object", name);
            }
        }
    }

    // Modifiers

    void shrink_to_fit()
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->shrink_to_fit();
            break;
        case value_types::object_t:
            var_.value_.object_val_->shrink_to_fit();
            break;
        default:
            break;
        }
    }

    void clear()
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->clear();
            break;
        case value_types::object_t:
            var_.value_.object_val_->clear();
            break;
        default:
            break;
        }
    }

    void erase(object_iterator first, object_iterator last)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            break;
        case value_types::object_t:
            var_.value_.object_val_->erase(first, last);
            break;
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an object");
            break;
        }
    }

    void erase(array_iterator first, array_iterator last)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->erase(first, last);
            break;
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an array");
            break;
        }
    }

    // Removes all elements from an array value whose index is between from_index, inclusive, and to_index, exclusive.

    void erase(const string_type& name)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            break;
        case value_types::object_t:
            var_.value_.object_val_->erase(name);
            break;
        default:
            JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object", name);
            break;
        }
    }

    void set(const string_type& name, const basic_jcr_validator<JsonT>& value)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            var_.value_.object_val_->set(name, value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object", name);
            }
        }
    }

    void set(string_type&& name, const basic_jcr_validator<JsonT>& value){
        switch (var_.type_){
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            var_.value_.object_val_->set(std::move(name),value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    }

    void set(const string_type& name, basic_jcr_validator<JsonT>&& value){
        switch (var_.type_){
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            var_.value_.object_val_->set(name,std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    }

    void set(string_type&& name, basic_jcr_validator<JsonT>&& value)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            var_.value_.object_val_->set(std::move(name),std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    }

    object_iterator set(object_iterator hint, const string_type& name, const basic_jcr_validator<JsonT>& value)
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            return var_.value_.object_val_->set(hint, name, value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object", name);
            }
        }
    }

    object_iterator set(object_iterator hint, string_type&& name, const basic_jcr_validator<JsonT>& value){
        switch (var_.type_){
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            return var_.value_.object_val_->set(hint, std::move(name),value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    }

    object_iterator set(object_iterator hint, const string_type& name, basic_jcr_validator<JsonT>&& value){
        switch (var_.type_){
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            return var_.value_.object_val_->set(hint, name,std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    } 

    object_iterator set(object_iterator hint, string_type&& name, basic_jcr_validator<JsonT>&& value){
        switch (var_.type_){
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            return var_.value_.object_val_->set(hint, std::move(name),std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Attempting to set %s on a value that is not an object",name);
            }
        }
    }

    void add(const basic_jcr_validator<JsonT>& value)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            var_.value_.array_val_->push_back(value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Attempting to insert into a value that is not an array");
            }
        }
    }

    void add(basic_jcr_validator<JsonT>&& value){
        switch (var_.type_){
        case value_types::array_t:
            var_.value_.array_val_->push_back(std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Attempting to insert into a value that is not an array");
            }
        }
    }

    array_iterator add(const_array_iterator pos, const basic_jcr_validator<JsonT>& value)
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return var_.value_.array_val_->add(pos, value);
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Attempting to insert into a value that is not an array");
            }
        }
    }

    array_iterator add(const_array_iterator pos, basic_jcr_validator<JsonT>&& value){
        switch (var_.type_){
        case value_types::array_t:
            return var_.value_.array_val_->add(pos, std::move(value));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Attempting to insert into a value that is not an array");
            }
        }
    }

    value_types type() const
    {
        return var_.type_;
    }

    uint8_t length_or_precision() const
    {
        return var_.length_or_precision_;
    }

    void swap(basic_jcr_validator& b)
    {
        var_.swap(b.var_);
    }

    template <class T>
    std::vector<T> as_vector() const
    {
        std::vector<T> v(size());
        for (size_t i = 0; i < v.size(); ++i)
        {
            v[i] = json_type_traits<value_type,T>::as(at(i));
        }
        return v;
    }

    friend void swap(JsonT& a, JsonT& b)
    {
        a.swap(b);
    }

    void assign_string(const string_type& rhs)
    {
        var_.assign(rhs);
    }

    void assign_string(const char_type* rhs, size_t length)
    {
        var_.assign_string(rhs,length);
    }

    void assign_bool(bool rhs)
    {
        var_.assign(rhs);
    }

    void assign_object(const object & rhs)
    {
        var_.assign(rhs);
    }

    void assign_array(const array& rhs)
    {
        var_.assign(rhs);
    }

    void assign_null()
    {
        var_.assign(null_type());
    }

    void assign_integer(int64_t rhs)
    {
        var_.assign(rhs);
    }

    void assign_uinteger(uint64_t rhs)
    {
        var_.assign(rhs);
    }

    void assign_double(double rhs, uint8_t precision = 0)
    {
        var_.assign(rhs,precision);
    }

    static basic_jcr_validator make_2d_array(size_t m, size_t n);

    template <typename T>
    static basic_jcr_validator make_2d_array(size_t m, size_t n, T val);

    static basic_jcr_validator make_3d_array(size_t m, size_t n, size_t k);

    template <typename T>
    static basic_jcr_validator make_3d_array(size_t m, size_t n, size_t k, T val);

    object_range members()
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return object_range(object_iterator(true),object_iterator(true));
        case value_types::object_t:
            return object_range(object_value().begin(),object_value().end());
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an object");
        }
    }

    const_object_range members() const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            return const_object_range(const_object_iterator(true),const_object_iterator(true));
        case value_types::object_t:
            return const_object_range(object_value().begin(),object_value().end());
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an object");
        }
    }

    array_range elements()
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return array_range(array_value().begin(),array_value().end());
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an array");
        }
    }

    const_array_range elements() const
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return const_array_range(array_value().begin(),array_value().end());
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Not an array");
        }
    }

    array& array_value() 
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return *(var_.value_.array_val_);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad array cast");
            break;
        }
    }

    const array& array_value() const
    {
        switch (var_.type_)
        {
        case value_types::array_t:
            return *(var_.value_.array_val_);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad array cast");
            break;
        }
    }

    object& object_value()
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            create_object_implicitly();
        case value_types::object_t:
            return *(var_.value_.object_val_);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad object cast");
            break;
        }
    }

    const object& object_value() const
    {
        switch (var_.type_)
        {
        case value_types::empty_object_t:
            const_cast<value_type*>(this)->create_object_implicitly(); // HERE
        case value_types::object_t:
            return *(var_.value_.object_val_);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad object cast");
            break;
        }
    }

    bool validate(const JsonT& val) const
    {
        return var_.validate(val);
    }

private:

    friend std::basic_ostream<typename string_type::value_type>& operator<<(std::basic_ostream<typename string_type::value_type>& os, const basic_jcr_validator<JsonT>& o)
    {
        o.to_stream(os);
        return os;
    }

    friend std::basic_istream<typename string_type::value_type>& operator<<(std::basic_istream<typename string_type::value_type>& is, basic_jcr_validator<JsonT>& o)
    {
        basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
        basic_json_reader<typename string_type::value_type> reader(is, handler);
        reader.read_next();
        reader.check_done();
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
        }
        o = handler.get_result();
        return is;
    }
};

template <class JsonT>
void swap(typename JsonT::member_type& a, typename JsonT::member_type& b)
{
    a.swap(b);
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_stream(std::basic_istream<char_type>& is)
{
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    basic_json_reader<char_type> reader(is, handler);
    reader.read_next();
    reader.check_done();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_stream(std::basic_istream<char_type>& is, 
                                                              basic_parse_error_handler<char_type>& err_handler)
{
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    basic_json_reader<char_type> reader(is, handler, err_handler);
    reader.read_next();
    reader.check_done();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_file(const std::string& filename)
{
    FILE* fp;

#if defined(JSONCONS_HAS_FOPEN_S)
    errno_t err = fopen_s(&fp, filename.c_str(), "rb");
    if (err != 0) 
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#else
    fp = std::fopen(filename.c_str(), "rb");
    if (fp == nullptr)
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#endif
    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    try
    {
        // obtain file size:
        std::fseek (fp , 0 , SEEK_END);
        long size = std::ftell (fp);
        std::rewind(fp);

        if (size > 0)
        {
            std::vector<char_type> buffer(size);

            // copy the file into the buffer:
            size_t result = std::fread (buffer.data(),1,size,fp);
            if (result != static_cast<unsigned long long>(size))
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Error reading file %s", filename);
            }

            basic_jcr_parser<char_type> parser(handler);
            parser.begin_parse();
            parser.parse(buffer.data(),0,buffer.size());
            parser.end_parse();
            parser.check_done(buffer.data(),parser.index(),buffer.size());
        }

        std::fclose (fp);
    }
    catch (...)
    {
        std::fclose (fp);
        throw;
    }
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json file");
    }
    return handler.get_result();
}

template<class JsonT>
basic_jcr_validator<JsonT> basic_jcr_validator<JsonT>::parse_file(const std::string& filename, 
                                                            basic_parse_error_handler<char_type>& err_handler)
{
    FILE* fp;

#if !defined(JSONCONS_HAS_FOPEN_S)
    fp = std::fopen(filename.c_str(), "rb");
    if (fp == nullptr)
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#else
    errno_t err = fopen_s(&fp, filename.c_str(), "rb");
    if (err != 0) 
    {
        JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Cannot open file %s", filename);
    }
#endif

    basic_jcr_deserializer<basic_jcr_validator<JsonT>> handler;
    try
    {
        // obtain file size:
        std::fseek (fp , 0 , SEEK_END);
        long size = std::ftell (fp);
        std::rewind(fp);

        if (size > 0)
        {
            std::vector<char_type> buffer(size);

            // copy the file into the buffer:
            size_t result = std::fread (buffer.data(),1,size,fp);
            if (result != static_cast<unsigned long long>(size))
            {
                JSONCONS_THROW_EXCEPTION_1(std::runtime_error,"Error reading file %s", filename);
            }

            basic_jcr_parser<char_type> parser(handler,err_handler);
            parser.begin_parse();
            parser.parse(buffer.data(),0,buffer.size());
            parser.end_parse();
            parser.check_done(buffer.data(),parser.index(),buffer.size());
        }

        std::fclose (fp);
    }
    catch (...)
    {
        std::fclose (fp);
        throw;
    }
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json file");
    }
    return handler.get_result();
}

template <typename JsonT>
std::basic_istream<typename JsonT::char_type>& operator>>(std::basic_istream<typename JsonT::char_type>& is, JsonT& o)
{
    basic_jcr_deserializer<JsonT> handler;
    basic_json_reader<typename JsonT::char_type> reader(is, handler);
    reader.read_next();
    reader.check_done();
    if (!handler.is_valid())
    {
        JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json stream");
    }
    o = handler.get_result();
    return is;
}

template<typename JsonT>
class json_printable
{
public:
    typedef typename JsonT::char_type char_type;

    json_printable(const JsonT& o,
                   bool is_pretty_print)
       : o_(&o), is_pretty_print_(is_pretty_print)
    {
    }

    json_printable(const JsonT& o,
                   bool is_pretty_print,
                   const basic_output_format<char_type>& format)
       : o_(&o), is_pretty_print_(is_pretty_print), format_(format)
    {
        ;
    }

    void to_stream(std::basic_ostream<char_type>& os) const
    {
        o_->to_stream(os, format_, is_pretty_print_);
    }

    friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const json_printable<JsonT>& o)
    {
        o.to_stream(os);
        return os;
    }

    const JsonT *o_;
    bool is_pretty_print_;
    basic_output_format<char_type> format_;
private:
    json_printable();
};

template<typename JsonT>
json_printable<JsonT> print(const JsonT& val)
{
    return json_printable<JsonT>(val,false);
}

template<class JsonT>
json_printable<JsonT> print(const JsonT& val,
                            const basic_output_format<typename JsonT::char_type>& format)
{
    return json_printable<JsonT>(val, false, format);
}

template<class JsonT>
json_printable<JsonT> pretty_print(const JsonT& val)
{
    return json_printable<JsonT>(val,true);
}

template<typename JsonT>
json_printable<JsonT> pretty_print(const JsonT& val,
                                   const basic_output_format<typename JsonT::char_type>& format)
{
    return json_printable<JsonT>(val, true, format);
}

typedef basic_jcr_validator<json> jcr_validator;
typedef basic_jcr_validator<wjson> wjcr_validator;

typedef basic_jcr_deserializer<jcr_validator> jcr_deserializer;
typedef basic_jcr_deserializer<wjcr_validator> wjcr_deserializer;

}}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
