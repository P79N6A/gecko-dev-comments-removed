




































#include <ostream>
#include <istream>
#ifdef DEBUG
#include <string>
#endif

namespace std {
#if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 2)
    
    template ostream& ostream::_M_insert(double);
    template ostream& ostream::_M_insert(long);
    template ostream& ostream::_M_insert(unsigned long);
    template ostream& __ostream_insert(ostream&, const char*, streamsize);
    template istream& istream::_M_extract(double&);
#endif
#ifdef DEBUG
#if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)
    

    template char *basic_string<char, char_traits<char>, allocator<char> >::_S_construct_aux_2(size_type, char, allocator<char> const&);
    template wchar_t *basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >::_S_construct_aux_2(size_type, wchar_t, allocator<wchar_t> const&);
#endif
#endif
}

namespace std __attribute__((visibility("default"))) {

#if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)
    
    struct _List_node_base
    {
        void hook(_List_node_base * const __position) throw ();

        void unhook() throw ();

        void transfer(_List_node_base * const __first,
                      _List_node_base * const __last) throw();

        void _M_hook(_List_node_base * const __position) throw ();

        void _M_unhook() throw ();

        void _M_transfer(_List_node_base * const __first,
                         _List_node_base * const __last) throw();
    };

    
    void
    _List_node_base::_M_hook(_List_node_base * const __position) throw ()
    {
        hook(__position);
    }

    void
    _List_node_base::_M_unhook() throw ()
    {
        unhook();
    }

    void
    _List_node_base::_M_transfer(_List_node_base * const __first,
                                 _List_node_base * const __last) throw ()
    {
        transfer(__first, __last);
    }
#endif

#if (__GNUC__ == 4) && (__GNUC_MINOR__ >= 4)
    




    void ctype<char>::_M_widen_init() const {}
#endif

}
