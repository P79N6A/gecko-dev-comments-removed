





#include "compiler/translator/util.h"

#include <limits>

#include "compiler/preprocessor/numeric_lex.h"

bool atof_clamp(const char *str, float *value)
{
    bool success = pp::numeric_lex_float(str, value);
    if (!success)
        *value = std::numeric_limits<float>::max();
    return success;
}

bool atoi_clamp(const char *str, int *value)
{
    bool success = pp::numeric_lex_int(str, value);
    if (!success)
        *value = std::numeric_limits<int>::max();
    return success;
}

