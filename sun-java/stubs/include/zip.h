



































#ifndef _ZIP_H_
#define _ZIP_H_

#include "bool.h"
#include "typedefs.h"
#include "jri.h"

typedef void zip_t;

struct stat;

JRI_PUBLIC_API(void)
zip_close(zip_t *zip);

JRI_PUBLIC_API(bool_t)
zip_get(zip_t *zip, const char *fn, void HUGEP *buf, int32_t len);

JRI_PUBLIC_API(zip_t *)
zip_open(const char *fn);

JRI_PUBLIC_API(bool_t)
zip_stat(zip_t *zip, const char *fn, struct stat *sbuf);

#endif  
