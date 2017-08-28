// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JCR_JSON_CONTENT_RULES_HPP
#define JSONCONS_JCR_JSON_CONTENT_RULES_HPP

#include <limits>
#include <string>
#include <vector>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <memory>
#include <typeinfo>
#include <cstring>
#include <jsoncons/json.hpp>
#include <jsoncons/json_traits.hpp>
#include <jsoncons/json_structures.hpp>
#include <jsoncons_ext/jcr/jcr_decoder.hpp>
#include <jsoncons_ext/jcr/jcr_parser.hpp>
#include <jsoncons/json_type_traits.hpp>
#include <jsoncons_ext/jcr/jcr_error_category.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif

namespace jsoncons { namespace jcr {

enum class value_type : uint8_t
{
    double_t,
    integer_t,
    uinteger_t,
    bool_t,
    null_t,
    string_t,
    object_t,
    array_t
};

struct rule
{
    virtual ~rule() = default;

    value_type type_id_;

    rule(value_type id)
        : type_id_(id)
    {}
};

template <class Json>
class basic_json_content_rules
{
public:

    typedef typename Json::allocator_type allocator_type;

    typedef typename Json::json_traits_type json_traits_type;

    typedef typename Json::parse_error_handler_type parse_error_handler_type;

    typedef typename Json::char_type char_type;
    typedef typename json_traits_type::char_traits_type char_traits_type;

#if !defined(JSONCONS_HAS_STRING_VIEW)
    typedef Basic_string_view_<char_type,char_traits_type> string_view_type;
#else
    typedef std::basic_string_view<char_type,char_traits_type> string_view_type;
#endif
    // string_type is for interface only, not storage 
    typedef std::basic_string<char_type,char_traits_type> string_type;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<char_type> char_allocator_type;

    using key_storage_type = typename json_traits_type::template key_storage<char_allocator_type>;

    using string_storage_type = typename json_traits_type::template string_storage<char_allocator_type>;

    typedef basic_json_content_rules<Json> json_type;
    typedef key_value_pair<key_storage_type,json_type> key_value_pair_type;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<json_type> val_allocator_type;
    using array_storage_type = typename json_traits_type::template array_storage<json_type, val_allocator_type>;

    typedef json_array<json_type> array;

    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<key_value_pair_type> kvp_allocator_type;

    using object_storage_type = typename json_traits_type::template object_storage<key_value_pair_type , kvp_allocator_type>;
    typedef json_object<key_storage_type,json_type,json_traits_type::preserve_order> object;


    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<array> array_allocator;
    typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<object> object_allocator;

    typedef jsoncons::null_type null_type;

    typedef typename object::iterator object_iterator;
    typedef typename object::const_iterator const_object_iterator;
    typedef typename array::iterator array_iterator;
    typedef typename array::const_iterator const_array_iterator;

    struct variant
    {

        struct null_data : public rule
        {
            null_data()
                : rule(value_type::null_t)
            {
            }
        };

        struct bool_data : public rule
        {
            bool val_;

            bool_data(bool val)
                : rule(value_type::bool_t),val_(val)
            {
            }

            bool_data(const bool_data& val)
                : rule(value_type::bool_t),val_(val.val_)
            {
            }

            bool value() const
            {
                return val_;
            }

        };

        struct integer_data : public rule
        {
            int64_t val_;

            integer_data(int64_t val)
                : rule(value_type::integer_t),val_(val)
            {
            }

            integer_data(const integer_data& val)
                : rule(value_type::integer_t),val_(val.val_)
            {
            }

            int64_t value() const
            {
                return val_;
            }
        };

        struct uinteger_data : public rule
        {
            uint64_t val_;

            uinteger_data(uint64_t val)
                : rule(value_type::uinteger_t),val_(val)
            {
            }

            uinteger_data(const uinteger_data& val)
                : rule(value_type::uinteger_t),val_(val.val_)
            {
            }

            uint64_t value() const
            {
                return val_;
            }
        };

        struct double_data : public rule
        {
            uint8_t precision_;
            double val_;

            double_data(double val, uint8_t precision)
                : rule(value_type::double_t), 
                  precision_(precision), 
                  val_(val)
            {
            }

            double_data(const double_data& val)
                : rule(value_type::double_t),
                  precision_(val.precision_), 
                  val_(val.val_)
            {
            }

            double value() const
            {
                return val_;
            }

            uint8_t precision() const
            {
                return precision_;
            }
        };

        struct string_data : public rule
        {
            typedef typename std::allocator_traits<allocator_type>:: template rebind_alloc<Json_string_<json_type>> string_holder_allocator_type;
            typedef typename std::allocator_traits<string_holder_allocator_type>::pointer pointer;

            pointer ptr_;

            template <typename... Args>
            void create(string_holder_allocator_type allocator, Args&& ... args)
            {
                typename std::allocator_traits<allocator_type>:: template rebind_alloc<Json_string_<json_type>> alloc(allocator);
                ptr_ = alloc.allocate(1);
                try
                {
                    std::allocator_traits<string_holder_allocator_type>:: template rebind_traits<Json_string_<json_type>>::construct(alloc, to_plain_pointer(ptr_), std::forward<Args>(args)...);
                }
                catch (...)
                {
                    alloc.deallocate(ptr_,1);
                    throw;
                }
            }

            string_data(const Json_string_<json_type>& val)
                : rule(value_type::string_t)
            {
                create(val.get_allocator(), val);
            }

            string_data(pointer ptr)
                : rule(value_type::string_t)
            {
                ptr_ = ptr;
            }

            string_data(const Json_string_<json_type>& val, const allocator_type& a)
                : rule(value_type::string_t)
            {
                create(string_holder_allocator_type(a), val, a);
            }

            string_data(const string_data & val)
                : rule(value_type::string_t)
            {
                create(val.ptr_->get_allocator(), *(val.ptr_));
            }

            string_data(const string_data & val, const allocator_type& a)
                : rule(value_type::string_t)
            {
                create(string_holder_allocator_type(a), *(val.ptr_), a);
            }

            template<class InputIterator>
            string_data(InputIterator first, InputIterator last, const allocator_type& a)
                : rule(value_type::string_t)
            {
                create(string_holder_allocator_type(a), first, last, a);
            }

            string_data(const char_type* data, size_t length, const allocator_type& a)
                : rule(value_type::string_t)
            {
                create(string_holder_allocator_type(a), data, length, a);
            }

            ~string_data()
            {
                typename std::allocator_traits<string_holder_allocator_type>:: template rebind_alloc<Json_string_<json_type>> alloc(ptr_->get_allocator());
                std::allocator_traits<string_holder_allocator_type>:: template rebind_traits<Json_string_<json_type>>::destroy(alloc, to_plain_pointer(ptr_));
                alloc.deallocate(ptr_,1);
            }

            const char_type* data() const
            {
                return ptr_->data();
            }

            const char_type* c_str() const
            {
                return ptr_->c_str();
            }

            size_t length() const
            {
                return ptr_->length();
            }

            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };

        struct object_data : public rule
        {
            typedef typename std::allocator_traits<object_allocator>::pointer pointer;
            pointer ptr_;

            template <typename... Args>
            void create(allocator_type allocator, Args&& ... args)
            {
                typename std::allocator_traits<object_allocator>:: template rebind_alloc<object> alloc(allocator);
                ptr_ = alloc.allocate(1);
                try
                {
                    std::allocator_traits<object_allocator>:: template rebind_traits<object>::construct(alloc, to_plain_pointer(ptr_), std::forward<Args>(args)...);
                }
                catch (...)
                {
                    alloc.deallocate(ptr_,1);
                    throw;
                }
            }

            explicit object_data()
                : rule(value_type::object_t)
            {
                create(allocator_type());
            }

            explicit object_data(const allocator_type& a)
                : rule(value_type::object_t)
            {
                create(a,a);
            }

            explicit object_data(pointer ptr)
                : rule(value_type::object_t)
            {
                ptr_ = ptr;
            }

            explicit object_data(const object & val)
                : rule(value_type::object_t)
            {
                create(val.get_allocator(), val);
            }

            explicit object_data(const object & val, const allocator_type& a)
                : rule(value_type::object_t)
            {
                create(object_allocator(a), val, a);
            }

            explicit object_data(const object_data & val)
                : rule(value_type::object_t)
            {
                create(val.ptr_->get_allocator(), *(val.ptr_));
            }

            explicit object_data(const object_data & val, const allocator_type& a)
                : rule(value_type::object_t)
            {
                create(object_allocator(a), *(val.ptr_), a);
            }

            ~object_data()
            {
                typename std::allocator_traits<allocator_type>:: template rebind_alloc<object> alloc(ptr_->get_allocator());
                std::allocator_traits<allocator_type>:: template rebind_traits<object>::destroy(alloc, to_plain_pointer(ptr_));
                alloc.deallocate(ptr_,1);
            }

            object& value()
            {
                return *ptr_;
            }

            const object& value() const
            {
                return *ptr_;
            }

            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }
        };
    public:
        struct array_data : public rule
        {
            typedef typename std::allocator_traits<array_allocator>::pointer pointer;
            pointer ptr_;

            template <typename... Args>
            void create(array_allocator allocator, Args&& ... args)
            {
                typename std::allocator_traits<allocator_type>:: template rebind_alloc<array> alloc(allocator);
                ptr_ = alloc.allocate(1);
                try
                {
                    std::allocator_traits<array_allocator>:: template rebind_traits<array>::construct(alloc, to_plain_pointer(ptr_), std::forward<Args>(args)...);
                }
                catch (...)
                {
                    alloc.deallocate(ptr_,1);
                    throw;
                }
            }

            allocator_type get_allocator() const
            {
                return ptr_->get_allocator();
            }

            array_data(const array& val)
                : rule(value_type::array_t)
            {
                create(val.get_allocator(), val);
            }

            array_data(pointer ptr)
                : rule(value_type::array_t)
            {
                ptr_ = ptr;
            }

            array_data(const array& val, const allocator_type& a)
                : rule(value_type::array_t)
            {
                create(array_allocator(a), val, a);
            }

            array_data(const array_data & val)
                : rule(value_type::array_t)
            {
                create(val.ptr_->get_allocator(), *(val.ptr_));
            }

            array_data(const array_data & val, const allocator_type& a)
                : rule(value_type::array_t)
            {
                create(array_allocator(a), *(val.ptr_), a);
            }

            template<class InputIterator>
            array_data(InputIterator first, InputIterator last, const allocator_type& a)
                : rule(value_type::array_t)
            {
                create(array_allocator(a), first, last, a);
            }

            ~array_data()
            {
                typename std::allocator_traits<array_allocator>:: template rebind_alloc<array> alloc(ptr_->get_allocator());
                std::allocator_traits<array_allocator>:: template rebind_traits<array>::destroy(alloc, to_plain_pointer(ptr_));
                alloc.deallocate(ptr_,1);
            }

            array& value()
            {
                return *ptr_;
            }

            const array& value() const
            {
                return *ptr_;
            }
        };

    private:
        static const size_t data_size = static_max<sizeof(uinteger_data),sizeof(double_data), sizeof(string_data), sizeof(array_data), sizeof(object_data)>::value;
        static const size_t data_align = static_max<JSONCONS_ALIGNOF(uinteger_data),JSONCONS_ALIGNOF(double_data),JSONCONS_ALIGNOF(string_data),JSONCONS_ALIGNOF(array_data),JSONCONS_ALIGNOF(object_data)>::value;

        typedef typename std::aligned_storage<data_size,data_align>::type data_t;

        std::shared_ptr<rule> data_;
    public:
        variant()
        {
            data_ = std::make_shared<object_data>();
        }

        variant(const allocator_type& a)
        {
            data_ = std::make_shared<object_data>(a);
        }

        variant(const variant& val)
        {
            Init_(val);
        }

        variant(const variant& val, const allocator_type& allocator)
        {
            Init_(val,allocator);
        }

        variant(variant&& val) JSONCONS_NOEXCEPT
        {
            Init_rv_(std::forward<variant>(val));
        }

        variant(variant&& val, const allocator_type& allocator) JSONCONS_NOEXCEPT
        {
            Init_rv_(std::forward<variant>(val), allocator,
                     typename std::allocator_traits<allocator_type>::propagate_on_container_move_assignment());
        }

        explicit variant(null_type)
        {
            data_ = std::make_shared<null_data>();
        }
        explicit variant(bool val)
        {
            data_ = std::make_shared<bool_data>(val);
        }
        explicit variant(int64_t val)
        {
            data_ = std::make_shared<integer_data>(val);
        }
        explicit variant(uint64_t val, const allocator_type&)
        {
            data_ = std::make_shared<uinteger_data>(val);
        }
        explicit variant(uint64_t val)
        {
            data_ = std::make_shared<uinteger_data>(val);
        }
        variant(double val)
        {
            data_ = std::make_shared<double_data>(val,0);
        }
        variant(double val, uint8_t precision)
        {
            data_ = std::make_shared<double_data>(val,precision);
        }
        variant(const char_type* s, size_t length)
        {
            data_ = std::make_shared<string_data>(s, length, char_allocator_type());
        }
        variant(const char_type* s)
        {
            data_ = std::make_shared<string_data>(s, length, char_allocator_type());
        }

        variant(const char_type* s, const allocator_type& alloc)
        {
            data_ = std::make_shared<string_data>(s, length, alloc);
        }

        variant(const char_type* s, size_t length, const allocator_type& alloc)
        {
            data_ = std::make_shared<string_data>(s, length, alloc);
        }
        variant(const object& val)
        {
            data_ = std::make_shared<object_data>(val);
        }
        variant(const object& val, const allocator_type& alloc)
        {
            data_ = std::make_shared<object_data>(val, alloc);
        }
        variant(const array& val)
        {
            data_ = std::make_shared<array_data>(val);
        }
        variant(const array& val, const allocator_type& alloc)
        {
            data_ = std::make_shared<array_data>(val,alloc);
        }
        template<class InputIterator>
        variant(InputIterator first, InputIterator last, const allocator_type& a)
        {
            data_ = std::make_shared<array_data>(first, last, a);
        }

        ~variant()
        {
        }

        variant& operator=(const variant& val)
        {
            if (this != &val)
            {
                switch (val.type_id())
                {
                case value_type::null_t:
                    data_ = std::make_shared<null_data>();
                    break;
                case value_type::double_t:
                    data_ = std::make_shared<double_data>(*(val.double_data_cast()));
                    break;
                case value_type::integer_t:
                    data_ = std::make_shared<integer_data>(*(val.integer_data_cast()));
                    break;
                case value_type::uinteger_t:
                    data_ = std::make_shared<uinteger_data>(*(val.uinteger_data_cast()));
                    break;
                case value_type::bool_t:
                    data_ = std::make_shared<bool_data>(*(val.bool_data_cast()));
                    break;
                case value_type::string_t:
                    data_ = std::make_shared<string_data>(*(val.string_data_cast()));
                    break;
                case value_type::object_t:
                    data_ = std::make_shared<object_data>(*(val.object_data_cast()));
                    break;
                case value_type::array_t:
                    data_ = std::make_shared<array_data>(*(val.array_data_cast()));
                    break;
                default:
                    break;
                }
            }
            return *this;
        }

        variant& operator=(variant&& val) JSONCONS_NOEXCEPT
        {
            if (this != &val)
            {
                data_ = std::make_shared<null_data>();
                swap(val);
            }
            return *this;
        }

        value_type type_id() const
        {
            return data_->type_id_;
        }

        const null_data* null_data_cast() const
        {
            return static_cast<const null_data*>(data_.get());
        }

        const bool_data* bool_data_cast() const
        {
            return static_cast<const bool_data*>(data_.get());
        }

        const integer_data* integer_data_cast() const
        {
            return static_cast<const integer_data*>(data_.get());
        }

        const uinteger_data* uinteger_data_cast() const
        {
            return static_cast<const uinteger_data*>(data_.get());
        }

        const double_data* double_data_cast() const
        {
            return static_cast<const double_data*>(data_.get());
        }

        string_data* string_data_cast()
        {
            return static_cast<string_data*>(data_.get());
        }

        const string_data* string_data_cast() const
        {
            return reinterpret_cast<const string_data*>(&data_);
        }

        object_data* object_data_cast()
        {
            return static_cast<object_data*>(data_.get());
        }

        const object_data* object_data_cast() const
        {
            return static_cast<const object_data*>(data_.get());
        }

        array_data* array_data_cast()
        {
            return static_cast<array_data*>(data_.get());
        }

        const array_data* array_data_cast() const
        {
            return static_cast<const array_data*>(data_.get());
        }

        void swap(variant& rhs) JSONCONS_NOEXCEPT
        {
            if (this != &rhs)
            {
                switch (type_id())
                {
                case value_type::string_t:
                    {
                        auto ptr = string_data_cast()->ptr_;
                        switch (rhs.type_id())
                        {
                        case value_type::object_t:
                            data_ = std::make_shared<object_data>(rhs.object_data_cast()->ptr_);
                            break;
                        case value_type::array_t:
                            data_ = std::make_shared<array_data>(rhs.array_data_cast()->ptr_);
                            break;
                        case value_type::string_t:
                            data_ = std::make_shared<string_data>(rhs.string_data_cast()->ptr_);
                            break;
                        case value_type::null_t:
                            data_ = std::make_shared<null_data>();
                            break;
                        case value_type::double_t:
                            data_ = std::make_shared<double_data>(*(rhs.double_data_cast()));
                            break;
                        case value_type::integer_t:
                            data_ = std::make_shared<integer_data>(*(rhs.integer_data_cast()));
                            break;
                        case value_type::uinteger_t:
                            data_ = std::make_shared<uinteger_data>(*(rhs.uinteger_data_cast()));
                            break;
                        case value_type::bool_t:
                            data_ = std::make_shared<bool_data>(*(rhs.bool_data_cast()));
                            break;
                        default:
                            break;
                        }
                        new(reinterpret_cast<void*>(&(rhs.data_)))string_data(ptr);
                    }
                    break;
                case value_type::object_t:
                    {
                        auto ptr = object_data_cast()->ptr_;
                        switch (rhs.type_id())
                        {
                        case value_type::object_t:
                            data_ = std::make_shared<object_data>(rhs.object_data_cast()->ptr_);
                            break;
                        case value_type::array_t:
                            data_ = std::make_shared<array_data>(rhs.array_data_cast()->ptr_);
                            break;
                        case value_type::string_t:
                            data_ = std::make_shared<string_data>(rhs.string_data_cast()->ptr_);
                            break;
                        case value_type::null_t:
                            data_ = std::make_shared<null_data>();
                            break;
                        case value_type::double_t:
                            data_ = std::make_shared<double_data>(*(rhs.double_data_cast()));
                            break;
                        case value_type::integer_t:
                            data_ = std::make_shared<integer_data>(*(rhs.integer_data_cast()));
                            break;
                        case value_type::uinteger_t:
                            data_ = std::make_shared<uinteger_data>(*(rhs.uinteger_data_cast()));
                            break;
                        case value_type::bool_t:
                            data_ = std::make_shared<bool_data>(*(rhs.bool_data_cast()));
                            break;
                        default:
                            break;
                        }
                        new(reinterpret_cast<void*>(&(rhs.data_)))object_data(ptr);
                    }
                    break;
                case value_type::array_t:
                    {
                        auto ptr = array_data_cast()->ptr_;
                        switch (rhs.type_id())
                        {
                        case value_type::object_t:
                            data_ = std::make_shared<object_data>(rhs.object_data_cast()->ptr_);
                            break;
                        case value_type::array_t:
                            data_ = std::make_shared<array_data>(rhs.array_data_cast()->ptr_);
                            break;
                        case value_type::string_t:
                            data_ = std::make_shared<string_data>(rhs.string_data_cast()->ptr_);
                            break;
                        case value_type::null_t:
                            data_ = std::make_shared<null_data>();
                            break;
                        case value_type::double_t:
                            data_ = std::make_shared<double_data>(*(rhs.double_data_cast()));
                            break;
                        case value_type::integer_t:
                            data_ = std::make_shared<integer_data>(*(rhs.integer_data_cast()));
                            break;
                        case value_type::uinteger_t:
                            data_ = std::make_shared<uinteger_data>(*(rhs.uinteger_data_cast()));
                            break;
                        case value_type::bool_t:
                            data_ = std::make_shared<bool_data>(*(rhs.bool_data_cast()));
                            break;
                        default:
                            break;
                        }
                        new(reinterpret_cast<void*>(&(rhs.data_)))array_data(ptr);
                    }
                    break;
                default:
                    switch (rhs.type_id())
                    {
                    case value_type::string_t:
                        {
                            auto ptr = rhs.string_data_cast()->ptr_;
                            switch (type_id())
                            {
                            case value_type::null_t:
                                new(reinterpret_cast<void*>(&rhs.data_))null_data();
                                break;
                            case value_type::double_t:
                                new(reinterpret_cast<void*>(&rhs.data_))double_data(*(double_data_cast()));
                                break;
                            case value_type::integer_t:
                                new(reinterpret_cast<void*>(&rhs.data_))integer_data(*(integer_data_cast()));
                                break;
                            case value_type::uinteger_t:
                                new(reinterpret_cast<void*>(&rhs.data_))uinteger_data(*(uinteger_data_cast()));
                                break;
                            case value_type::bool_t:
                                new(reinterpret_cast<void*>(&rhs.data_))bool_data(*(bool_data_cast()));
                                break;
                            default:
                                break;
                            }
                            data_ = std::make_shared<string_data>(ptr);
                        }
                        break;
                    case value_type::object_t:
                        {
                            auto ptr = rhs.object_data_cast()->ptr_;
                            switch (type_id())
                            {
                            case value_type::null_t:
                                new(reinterpret_cast<void*>(&rhs.data_))null_data();
                                break;
                            case value_type::double_t:
                                new(reinterpret_cast<void*>(&rhs.data_))double_data(*(double_data_cast()));
                                break;
                            case value_type::integer_t:
                                new(reinterpret_cast<void*>(&rhs.data_))integer_data(*(integer_data_cast()));
                                break;
                            case value_type::uinteger_t:
                                new(reinterpret_cast<void*>(&rhs.data_))uinteger_data(*(uinteger_data_cast()));
                                break;
                            case value_type::bool_t:
                                new(reinterpret_cast<void*>(&rhs.data_))bool_data(*(bool_data_cast()));
                                break;
                            default:
                                break;
                            }
                            data_ = std::make_shared<object_data>(ptr);
                        }
                        break;
                    case value_type::array_t:
                        {
                            auto ptr = rhs.array_data_cast()->ptr_;
                            switch (type_id())
                            {
                            case value_type::null_t:
                                new(reinterpret_cast<void*>(&rhs.data_))null_data();
                                break;
                            case value_type::double_t:
                                new(reinterpret_cast<void*>(&rhs.data_))double_data(*(double_data_cast()));
                                break;
                            case value_type::integer_t:
                                new(reinterpret_cast<void*>(&rhs.data_))integer_data(*(integer_data_cast()));
                                break;
                            case value_type::uinteger_t:
                                new(reinterpret_cast<void*>(&rhs.data_))uinteger_data(*(uinteger_data_cast()));
                                break;
                            case value_type::bool_t:
                                new(reinterpret_cast<void*>(&rhs.data_))bool_data(*(bool_data_cast()));
                                break;
                            default:
                                break;
                            }
                            data_ = std::make_shared<array_data>(ptr);
                        }
                        break;
                    default:
                        {
                            std::swap(data_,rhs.data_);
                        }
                        break;
                    }
                }
            }
        }
    private:

        void Init_(const variant& val)
        {
            switch (val.type_id())
            {
            case value_type::null_t:
                data_ = std::make_shared<null_data>();
                break;
            case value_type::double_t:
                data_ = std::make_shared<double_data>(*(val.double_data_cast()));
                break;
            case value_type::integer_t:
                data_ = std::make_shared<integer_data>(*(val.integer_data_cast()));
                break;
            case value_type::uinteger_t:
                data_ = std::make_shared<uinteger_data>(*(val.uinteger_data_cast()));
                break;
            case value_type::bool_t:
                data_ = std::make_shared<bool_data>(*(val.bool_data_cast()));
                break;
            case value_type::string_t:
                data_ = std::make_shared<string_data>(*(val.string_data_cast()));
                break;
            case value_type::object_t:
                data_ = std::make_shared<object_data>(*(val.object_data_cast()));
                break;
            case value_type::array_t:
                data_ = std::make_shared<array_data>(*(val.array_data_cast()));
                break;
            default:
                break;
            }
        }

        void Init_(const variant& val, const allocator_type& a)
        {
            switch (val.type_id())
            {
            case value_type::null_t:
            case value_type::double_t:
            case value_type::integer_t:
            case value_type::uinteger_t:
            case value_type::bool_t:
            case value_type::string_t:
                data_ = std::make_shared<string_data>(*(val.string_data_cast()),a);
                break;
            case value_type::object_t:
                data_ = std::make_shared<object_data>(*(val.object_data_cast()),a);
                break;
            case value_type::array_t:
                data_ = std::make_shared<array_data>(*(val.array_data_cast()),a);
                break;
            default:
                break;
            }
        }

        void Init_rv_(variant&& val) JSONCONS_NOEXCEPT
        {
            switch (val.type_id())
            {
            case value_type::null_t:
            case value_type::double_t:
            case value_type::integer_t:
            case value_type::uinteger_t:
            case value_type::bool_t:
            case value_type::string_t:
                {
                    data_ = std::make_shared<string_data>(val.string_data_cast()->ptr_);
                    val.string_data_cast()->type_id_ = value_type::null_t;
                }
                break;
            case value_type::object_t:
                {
                    data_ = std::make_shared<object_data>(val.object_data_cast()->ptr_);
                    val.object_data_cast()->type_id_ = value_type::null_t;
                }
                break;
            case value_type::array_t:
                {
                    data_ = std::make_shared<array_data>(val.array_data_cast()->ptr_);
                    val.array_data_cast()->type_id_ = value_type::null_t;
                }
                break;
            default:
                break;
            }
        }

        void Init_rv_(variant&& val, const allocator_type& a, std::true_type) JSONCONS_NOEXCEPT
        {
            Init_rv_(std::forward<variant>(val));
        }

        void Init_rv_(variant&& val, const allocator_type& a, std::false_type) JSONCONS_NOEXCEPT
        {
            switch (val.type_id())
            {
            case value_type::null_t:
            case value_type::double_t:
            case value_type::integer_t:
            case value_type::uinteger_t:
            case value_type::bool_t:
            case value_type::string_t:
                {
                    if (a == val.string_data_cast()->get_allocator())
                    {
                        Init_rv_(std::forward<variant>(val), a, std::true_type());
                    }
                    else
                    {
                        Init_(val,a);
                    }
                }
                break;
            case value_type::object_t:
                {
                    if (a == val.object_data_cast()->get_allocator())
                    {
                        Init_rv_(std::forward<variant>(val), a, std::true_type());
                    }
                    else
                    {
                        Init_(val,a);
                    }
                }
                break;
            case value_type::array_t:
                {
                    if (a == val.array_data_cast()->get_allocator())
                    {
                        Init_rv_(std::forward<variant>(val), a, std::true_type());
                    }
                    else
                    {
                        Init_(val,a);
                    }
                }
                break;
            default:
                break;
            }
        }
    };

    bool is_object() const JSONCONS_NOEXCEPT
    {
        return var_.type_id() == value_type::object_t;
    }

    bool is_array() const JSONCONS_NOEXCEPT
    {
        return var_.type_id() == value_type::array_t;
    }

    static basic_json_content_rules parse(string_view_type s)
    {
        parse_error_handler_type err_handler;
        return parse(s,err_handler);
    }

    static basic_json_content_rules parse(const char_type* s, size_t length)
    {
        parse_error_handler_type err_handler;
        return parse(s,length,err_handler);
    }

    static basic_json_content_rules parse(const char_type* s, size_t length, basic_parse_error_handler<char_type>& err_handler)
    {
        return parse(string_view_type(s,length),err_handler);
    }

    static basic_json_content_rules parse(string_view_type s, basic_parse_error_handler<char_type>& err_handler)
    {
        jcr_decoder<json_type> handler;
        basic_jcr_parser<char_type> parser(handler,err_handler);
        parser.set_source(s.data(),s.length());
        parser.skip_bom();
        parser.parse();
        parser.end_parse();
        parser.check_done();
        if (!handler.is_valid())
        {
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Failed to parse json string");
        }
        return handler.get_result();
    }

    static const json_type& null()
    {
        static json_type a_null = json_type(variant(null_type()));
        return a_null;
    }

    variant var_;

    basic_json_content_rules() 
        : var_()
    {
    }

    explicit basic_json_content_rules(const allocator_type& allocator) 
        : var_(allocator)
    {
    }

    basic_json_content_rules(const json_type& val)
        : var_(val.var_)
    {
    }

    basic_json_content_rules(const json_type& val, const allocator_type& allocator)
        : var_(val.var_,allocator)
    {
    }

    basic_json_content_rules(json_type&& other) JSONCONS_NOEXCEPT
        : var_(std::move(other.var_))
    {
    }

    basic_json_content_rules(json_type&& other, const allocator_type& allocator) JSONCONS_NOEXCEPT
        : var_(std::move(other.var_) /*,allocator*/ )
    {
    }

    basic_json_content_rules(const variant& val)
        : var_(val)
    {
    }

    basic_json_content_rules(variant&& other)
        : var_(std::forward<variant>(other))
    {
    }

    basic_json_content_rules(const array& val)
        : var_(val)
    {
    }

    basic_json_content_rules(array&& other)
        : var_(std::forward<array>(other))
    {
    }

    basic_json_content_rules(const object& other)
        : var_(other)
    {
    }

    basic_json_content_rules(object&& other)
        : var_(std::forward<object>(other))
    {
    }

    template <class T>
    basic_json_content_rules(const T& val)
        : var_(json_type_traits<json_type,T>::to_json(val).var_)
    {
    }

    template <class T>
    basic_json_content_rules(const T& val, const allocator_type& allocator)
        : var_(json_type_traits<json_type,T>::to_json(val,allocator).var_)
    {
    }

    basic_json_content_rules(const char_type* s)
        : var_(s)
    {
    }

    basic_json_content_rules(const char_type* s, const allocator_type& allocator)
        : var_(s,allocator)
    {
    }

    basic_json_content_rules(double val, uint8_t precision)
        : var_(val,precision)
    {
    }

    basic_json_content_rules(const char_type *s, size_t length, const allocator_type& allocator = allocator_type())
        : var_(s, length, allocator)
    {
    }
    template<class InputIterator>
    basic_json_content_rules(InputIterator first, InputIterator last, const allocator_type& allocator = allocator_type())
        : var_(first,last,allocator)
    {
    }

    ~basic_json_content_rules()
    {
    }

    basic_json_content_rules& operator=(const json_type& rhs)
    {
        var_ = rhs.var_;
        return *this;
    }

    basic_json_content_rules& operator=(json_type&& rhs) JSONCONS_NOEXCEPT
    {
        if (this != &rhs)
        {
            var_ = std::move(rhs.var_);
        }
        return *this;
    }

    template <class T>
    json_type& operator=(const T& val)
    {
        var_ = json_type_traits<json_type,T>::to_json(val).var_;
        return *this;
    }

    json_type& operator=(const char_type* s)
    {
        var_ = variant(s);
        return *this;
    }

    void reserve(size_t n)
    {
        switch (var_.type_id())
        {
        case value_type::array_t:
            array_value().reserve(n);
            break;
        case value_type::object_t:
        {
            object_value().reserve(n);
        }
            break;
        default:
            break;
        }
    }

    bool validate(const Json& j) const
    {
        return true;
    }

    // Modifiers

    template <class T>
    void add(T&& val)
    {
        switch (var_.type_id())
        {
        case value_type::array_t:
            array_value().add(std::forward<T>(val));
            break;
        default:
            {
                JSONCONS_THROW_EXCEPTION(std::runtime_error,"Attempting to insert into a value that is not an array");
            }
        }
    }

    value_type type_id() const
    {
        return var_.type_id();
    }

    void swap(json_type& b)
    {
        var_.swap(b.var_);
    }

    friend void swap(json_type& a, json_type& b)
    {
        a.swap(b);
    }

    static json_type make_string(string_view_type s)
    {
        return json_type(variant(s.data(),s.length()));
    }

    static json_type make_string(const char_type* rhs, size_t length)
    {
        return json_type(variant(rhs,length));
    }

    static json_type make_string(string_view_type s, allocator_type allocator)
    {
        return json_type(variant(s.data(),s.length(),allocator));
    }

    static json_type from_integer(int64_t val)
    {
        return json_type(variant(val));
    }

    static json_type from_integer(int64_t val, allocator_type)
    {
        return json_type(variant(val));
    }

    static json_type from_uinteger(uint64_t val)
    {
        return json_type(variant(val));
    }

    static json_type from_uinteger(uint64_t val, allocator_type)
    {
        return json_type(variant(val));
    }

    static json_type from_floating_point(double val)
    {
        return json_type(variant(val));
    }

    static json_type from_floating_point(double val, allocator_type)
    {
        return json_type(variant(val));
    }

    static json_type from_bool(bool val)
    {
        return json_type(variant(val));
    }

    static json_type make_object(const object& o)
    {
        return json_type(variant(o));
    }

    static json_type make_object(const object& o, allocator_type allocator)
    {
        return json_type(variant(o,allocator));
    }

    array& array_value() 
    {
        switch (var_.type_id())
        {
        case value_type::array_t:
            return var_.array_data_cast()->value();
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad array cast");
            break;
        }
    }

    const array& array_value() const
    {
        switch (var_.type_id())
        {
        case value_type::array_t:
            return var_.array_data_cast()->value();
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad array cast");
            break;
        }
    }

    object& object_value()
    {
        switch (var_.type_id())
        {
        case value_type::object_t:
            return var_.object_data_cast()->value();
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad object cast");
            break;
        }
    }

    const object& object_value() const
    {
        switch (var_.type_id())
        {
        case value_type::object_t:
            return var_.object_data_cast()->value();
        default:
            JSONCONS_THROW_EXCEPTION(std::runtime_error,"Bad object cast");
            break;
        }
    }

private:
};

template <class Json>
void swap(typename Json::key_value_pair_type & a, typename Json::key_value_pair_type & b)
{
    a.swap(b);
}

typedef basic_json_content_rules<json> json_content_rules;

}}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif
