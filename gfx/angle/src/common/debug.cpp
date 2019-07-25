







#include "common/debug.h"

#include <stdio.h>
#include <stdarg.h>

static bool trace_on = true;

namespace gl
{
void trace(const char *format, ...)
{
    if (trace_on)
    {
        if (format)
        {
            FILE *file = fopen("debug.txt", "a");

            if (file)
            {
                va_list vararg;
                va_start(vararg, format);
                vfprintf(file, format, vararg);
                va_end(vararg);

                fclose(file);
            }
        }
    }
}
}
