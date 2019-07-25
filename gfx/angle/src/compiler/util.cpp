





#include <math.h>
#include <stdlib.h>

#include "util.h"

#ifdef _MSC_VER
    #include <locale.h>
#else
    #include <sstream>
#endif

double atof_dot(const char *str)
{
#ifdef _MSC_VER
    return _atof_l(str, _create_locale(LC_NUMERIC, "C"));
#else
    double result;
    std::istringstream s(str);
    std::locale l("C");
    s.imbue(l);
    s >> result;
    return result;
#endif
}
