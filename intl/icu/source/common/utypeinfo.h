








#ifndef __UTYPEINFO_H__
#define __UTYPEINFO_H__









#if defined(_MSC_VER) && _HAS_EXCEPTIONS == 0
#include <exception>
using std::exception;
#endif
#if !defined(_MSC_VER)
namespace std { class type_info; } 
#endif
#include <typeinfo>  

#endif
