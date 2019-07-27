



#include <ostream>
#include <istream>
#include <string>
#include <stdarg.h>
#include <stdio.h>
#include <mozilla/Assertions.h>
















#define GLIBCXX_VERSION(a, b, c) (((a) << 16) | ((b) << 8) | (c))

namespace std {
#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 9)
    
    template ostream& ostream::_M_insert(double);
    template ostream& ostream::_M_insert(long);
    template ostream& ostream::_M_insert(unsigned long);
    template ostream& ostream::_M_insert(long long);
    template ostream& ostream::_M_insert(unsigned long long);
    template ostream& ostream::_M_insert(bool);
    template ostream& ostream::_M_insert(const void*);
    template ostream& __ostream_insert(ostream&, const char*, streamsize);
    template istream& istream::_M_extract(double&);
    template istream& istream::_M_extract(float&);
    template istream& istream::_M_extract(unsigned int&);
    template istream& istream::_M_extract(unsigned long&);
    template istream& istream::_M_extract(unsigned short&);
    template istream& istream::_M_extract(unsigned long long&);
#endif
#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 14)
    

    template char *string::_S_construct_aux_2(size_type, char, allocator<char> const&);
#ifdef _GLIBCXX_USE_WCHAR_T
    template wchar_t *wstring::_S_construct_aux_2(size_type, wchar_t, allocator<wchar_t> const&);
#endif 
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    template string::basic_string(string&&);
    template string& string::operator=(string&&);
    template wstring::basic_string(wstring&&);
    template wstring& wstring::operator=(wstring&&);
    template string& string::assign(string&&);
    template wstring& wstring::assign(wstring&&);
#endif 
#endif 
#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 16)
    

    template int string::_S_compare(size_type, size_type);
#endif
}

namespace std MOZ_EXPORT {
#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 14)
    
    struct _List_node_base
    {
        void hook(_List_node_base * const __position) throw ();

        void unhook() throw ();

        void transfer(_List_node_base * const __first,
                      _List_node_base * const __last) throw();

        void reverse() throw();


#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 15)
        static void swap(_List_node_base& __x, _List_node_base& __y) throw ();
    };

    namespace __detail {

    struct _List_node_base
    {
#endif
        void _M_hook(_List_node_base * const __position) throw ();

        void _M_unhook() throw ();

        void _M_transfer(_List_node_base * const __first,
                         _List_node_base * const __last) throw();

        void _M_reverse() throw();

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 15)
        static void swap(_List_node_base& __x, _List_node_base& __y) throw ();
#endif
    };

    
    void
    _List_node_base::_M_hook(_List_node_base * const __position) throw ()
    {
        ((std::_List_node_base *)this)->hook((std::_List_node_base * const) __position);
    }

    void
    _List_node_base::_M_unhook() throw ()
    {
        ((std::_List_node_base *)this)->unhook();
    }

    void
    _List_node_base::_M_transfer(_List_node_base * const __first,
                                 _List_node_base * const __last) throw ()
    {
        ((std::_List_node_base *)this)->transfer((std::_List_node_base * const)__first,
                                                 (std::_List_node_base * const)__last);
    }

    void
    _List_node_base::_M_reverse() throw ()
    {
        ((std::_List_node_base *)this)->reverse();
    }

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 15)
    void
    _List_node_base::swap(_List_node_base& __x, _List_node_base& __y) throw ()
    {
        std::_List_node_base::swap(*((std::_List_node_base *) &__x),
                                   *((std::_List_node_base *) &__y));
    }
}
#endif

#endif

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 11)
    




    void ctype<char>::_M_widen_init() const {}
#endif

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 20)
    

    void __throw_out_of_range_fmt(char const* fmt, ...)
    {
        va_list ap;
        char buf[1024]; 

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        buf[sizeof(buf) - 1] = 0;
        va_end(ap);

        __throw_range_error(buf);
    }
#endif

}

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 20)



extern "C" void
__cxa_throw_bad_array_new_length()
{
    MOZ_CRASH();
}
#endif

#if MOZ_LIBSTDCXX_VERSION >= GLIBCXX_VERSION(3, 4, 21)




namespace std {
    runtime_error::runtime_error(char const* s)
    : runtime_error(std::string(s))
    {
    }
}
#endif
