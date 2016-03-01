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
    null_t,
    // Non simple types
    rule_t,
    object_t,
    array_t
};

inline
bool is_simple(value_types type)
{
    return type < value_types::rule_t;
}

template <class JsonT>
class basic_jcr_validator
{
public:

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

    class any_object_rule : public rule<value_type>
    {
    public:
        any_object_rule()
        {
        }

        rule<value_type>* clone() const override
        {
            return new any_object_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_object();
        }
    };

    class any_integer_rule : public rule<value_type>
    {
    public:
        any_integer_rule()
        {
        }

        rule<value_type>* clone() const override
        {
            return new any_integer_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() || val.as_uinteger();
        }
    };

    class string_rule : public rule<value_type>
    {
        string_type s_;
    public:
        string_rule(const char_type* p, size_t length, string_allocator sa)
            : s_(p,length,sa)
        {
        }
        string_rule(const string_type& s)
            : s_(s)
        {
        }

        rule<value_type>* clone() const override
        {
            return new string_rule(s_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_string() && val.as_string() == s_;
        }
    };

    class any_string_rule : public rule<value_type>
    {
    public:
        any_string_rule()
        {
        }

        rule<value_type>* clone() const override
        {
            return new any_string_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_string();
        }
    };

    class null_rule : public rule<value_type>
    {
    public:
        null_rule()
        {
        }

        rule<value_type>* clone() const override
        {
            return new null_rule();
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_null();
        }
    };

    class bool_rule : public rule<value_type>
    {
        bool val_;

    public:
        bool_rule(bool val)
            : val_(val)
        {
        }

        rule<value_type>* clone() const override
        {
            return new bool_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_bool() && val.as_bool() == val_;
        }
    };

    class double_rule : public rule<value_type>
    {
        double val_;
        uint8_t precision_;

    public:
        double_rule(double val, uint8_t precision)
            : val_(val), precision_(precision)
        {
        }

        rule<value_type>* clone() const override
        {
            return new double_rule(val_,precision_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_double() && val.as_double() == val_;
        }
    };

    class integer_rule : public rule<value_type>
    {
        int64_t val_;

    public:
        integer_rule(int64_t val)
            : val_(val)
        {
        }

        rule<value_type>* clone() const override
        {
            return new integer_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() && val.as_integer() == val_;
        }
    };

    class uinteger_rule : public rule<value_type>
    {
        uint64_t val_;
        uint64_t to_;

    public:
        uinteger_rule(uint64_t val)
            : val_(val)
        {
        }

        rule<value_type>* clone() const override
        {
            return new uinteger_rule(val_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_uinteger() && val.as_uinteger() == val_;
        }
    };

    class integer_range_rule : public rule<value_type>
    {
        int64_t from_;
        int64_t to_;

    public:
        integer_range_rule(int64_t from, int64_t to)
            : from_(from), to_(to)
        {
        }

        rule<value_type>* clone() const override
        {
            return new integer_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_integer() && val.as_integer() >= from_ && val.as_integer() <= to_;
        }
    };

    class uinteger_range_rule : public rule<value_type>
    {
        uint64_t from_;
        uint64_t to_;
    public:
        uinteger_range_rule(uint64_t from, uint64_t to)
            : from_(from), to_(to)
        {
        }

        rule<value_type>* clone() const override
        {
            return new uinteger_range_rule(from_,to_);
        }

        bool validate(const JsonT& val) const override
        {
            return val.is_uinteger() && val.as_uinteger() >= from_ && val.as_uinteger() <= to_;
        }
    };

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

        explicit variant(rule<value_type>* rule)
            : type_(value_types::rule_t)
        {
            value_.rule_val_ = rule;
        }

        void init_variant(const variant& var)
        {
            type_ = var.type_;
            switch (type_)
            {
            case value_types::null_t:
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

        void assign(rule<value_type>* val)
        {
            destroy_variant();
            type_ = value_types::rule_t;
            value_.rule_val_ = val;
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

        bool validate(const JsonT& val) const
        {

            switch (type_)
            {
            case value_types::rule_t:
                return value_.rule_val_->validate(val);
            case value_types::array_t:
                return value_.array_val_->validate(val);
            case value_types::object_t:
                return value_.object_val_->validate(val);
            default:
                // throw
                break;
            }
            return false;
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
            object* object_val_;
            array* array_val_;
            rule<value_type>* rule_val_;
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

    basic_jcr_validator(rule<value_type>* rule)
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

    basic_jcr_validator& operator=(const value_type& rhs)
    {
        var_ = rhs.var_;
        return *this;
    }

    basic_jcr_validator& operator=(value_type&& rhs)
    {
        if (this != &rhs)
        {
            var_ = std::move(rhs.var_);
        }
        return *this;
    }

    basic_jcr_validator<JsonT>& operator=(rule<value_type>* val)
    {
        var_.assign(val);
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

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::object_t || var_.type_ == value_types::empty_object_t;
    }

    bool is_array() const JSONCONS_NOEXCEPT
    {
        return var_.type_ == value_types::array_t;
    }

    void swap(basic_jcr_validator& b)
    {
        var_.swap(b.var_);
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

    rule<value_type>& array_value() 
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

    const rule<value_type>& array_value() const
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

    rule<value_type>& object_value()
    {
        switch (var_.type_)
        {
        case value_types::object_t:
            return *(var_.value_.object_val_);
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad object cast");
            break;
        }
    }

    const rule<value_type>& object_value() const
    {
        switch (var_.type_)
        {
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

typedef basic_jcr_validator<json> jcr_validator;
typedef basic_jcr_validator<wjson> wjcr_validator;

typedef basic_jcr_deserializer<jcr_validator> jcr_deserializer;
typedef basic_jcr_deserializer<wjcr_validator> wjcr_deserializer;

}}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
