







#include "common/debug.h"

#include <stdio.h>
#include <stdarg.h>

#ifndef TRACE_OUTPUT_FILE
#define TRACE_OUTPUT_FILE "debug.txt"
#endif

static bool trace_on = true;

namespace gl
{
void trace(const char *format, ...)
{
    if (trace_on)
    {
        if (format)
        {
            FILE *file = fopen(TRACE_OUTPUT_FILE, "a");

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
