







#ifndef COMPILER_PREPROCESSOR_NUMERIC_LEX_H_
#define COMPILER_PREPROCESSOR_NUMERIC_LEX_H_

#include <sstream>

namespace pp {

inline std::ios::fmtflags numeric_base_int(const std::string& str)
{
    if ((str.size() >= 2) &&
        (str[0] == '0') &&
        (str[1] == 'x' || str[1] == 'X'))
    {
        return std::ios::hex;
    }
    else if ((str.size() >= 1) && (str[0] == '0'))
    {
        return std::ios::oct;
    }
    return std::ios::dec;
}






template<typename IntType>
bool numeric_lex_int(const std::string& str, IntType* value)
{
    std::istringstream stream(str);
    
    
    stream.setf(numeric_base_int(str), std::ios::basefield);

    stream >> (*value);
    return !stream.fail();
}

template<typename FloatType>
bool numeric_lex_float(const std::string& str, FloatType* value)
{
    std::istringstream stream(str);
    
    
    stream.imbue(std::locale::classic());

    stream >> (*value);
    return !stream.fail();
}

} 
#endif 
