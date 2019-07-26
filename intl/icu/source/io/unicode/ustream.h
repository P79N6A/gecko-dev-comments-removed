













#ifndef USTREAM_H
#define USTREAM_H

#include "unicode/unistr.h"










#if U_IOSTREAM_SOURCE >= 199711
#if (__GNUC__ == 2)
#include <iostream>
#else
#include <istream>
#include <ostream>
#endif

U_NAMESPACE_BEGIN








U_IO_API std::ostream & U_EXPORT2 operator<<(std::ostream& stream, const UnicodeString& s);







U_IO_API std::istream & U_EXPORT2 operator>>(std::istream& stream, UnicodeString& s);
U_NAMESPACE_END

#endif



#endif
